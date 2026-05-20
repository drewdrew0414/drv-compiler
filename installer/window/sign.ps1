#Requires -Version 5.1
<#
.SYNOPSIS
    Sign dri-install.exe with Authenticode certificate.
    EV certificates (OID 2.23.140.1.3) are detected and used preferentially.
    Publisher shown in UAC: "drewdrew0414"
    Note: self-signed = yellow UAC warning ("unverified publisher").
    For green UAC, purchase an EV code-signing cert from DigiCert / Sectigo.
.PARAMETER ExePath
    Path to the executable to sign (default: .\dri-install.exe)
.PARAMETER CertName
    CN name for self-signed certificate (default: drewdrew0414)
.PARAMETER TimestampServer
    RFC 3161 timestamp server URL (default: http://timestamp.digicert.com)
#>
param(
    [string]$ExePath         = "$PSScriptRoot\dri-install.exe",
    [string]$CertName        = "drewdrew0414",
    [string]$TimestampServer = "http://timestamp.digicert.com"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step { param($m) Write-Host "  >> $m" -ForegroundColor Cyan }
function Write-OK   { param($m) Write-Host "  OK $m" -ForegroundColor Green }
function Write-Warn { param($m) Write-Host "  !! $m" -ForegroundColor Yellow }
function Write-Err  { param($m) Write-Host "  XX $m" -ForegroundColor Red }

# EV Code Signing OID (CA/Browser Forum Baseline Requirements)
$EV_OID = "2.23.140.1.3"

Write-Host ""
Write-Host "  dri Installer Code Signing" -ForegroundColor DarkCyan
Write-Host "  Timestamp server: $TimestampServer" -ForegroundColor Gray
Write-Host ""

# ── Step 1: Look for EV certificate first ────────────────────────────────────
Write-Step "Scanning for EV code-signing certificate (OID $EV_OID)..."

$evCert = Get-ChildItem Cert:\CurrentUser\My |
    Where-Object {
        $_.EnhancedKeyUsageList.ObjectId -contains "1.3.6.1.5.5.7.3.3" -and
        ($_.Extensions | Where-Object {
            $_.Oid.Value -eq "2.5.29.32" -and
            $_.Format($false) -match $EV_OID
        })
    } |
    Sort-Object NotAfter -Descending |
    Select-Object -First 1

if ($evCert) {
    Write-OK "EV certificate found — using it preferentially:"
    Write-Host "    Subject   : $($evCert.Subject)"       -ForegroundColor Green
    Write-Host "    Thumbprint: $($evCert.Thumbprint)"    -ForegroundColor Green
    Write-Host "    Expires   : $($evCert.NotAfter.ToString('yyyy-MM-dd'))" -ForegroundColor Green
    $cert = $evCert
    $certType = "EV"
} else {
    Write-Warn "No EV certificate found"

    # ── Step 2: Look for existing standard code-signing cert ─────────────────
    Write-Step "Looking for existing certificate: CN=$CertName ..."

    $cert = Get-ChildItem Cert:\CurrentUser\My |
        Where-Object {
            $_.Subject -eq "CN=$CertName" -and
            $_.EnhancedKeyUsageList.ObjectId -contains "1.3.6.1.5.5.7.3.3"
        } |
        Sort-Object NotAfter -Descending |
        Select-Object -First 1

    if ($cert) {
        Write-OK "Found existing certificate:"
        Write-Host "    Thumbprint: $($cert.Thumbprint)"                        -ForegroundColor Green
        Write-Host "    Expires   : $($cert.NotAfter.ToString('yyyy-MM-dd'))"   -ForegroundColor Green
        $certType = "SelfSigned"
    } else {
        # ── Step 3: Create self-signed certificate ────────────────────────────
        Write-Step "No certificate found — creating self-signed code-signing certificate..."

        $cert = New-SelfSignedCertificate `
            -Subject           "CN=$CertName" `
            -Type              CodeSigning `
            -KeyUsage          DigitalSignature `
            -FriendlyName      "dri installer signing key" `
            -CertStoreLocation Cert:\CurrentUser\My `
            -NotAfter          (Get-Date).AddYears(5)

        Write-OK "Created self-signed certificate:"
        Write-Host "    Thumbprint: $($cert.Thumbprint)"                        -ForegroundColor Green
        Write-Host "    Expires   : $($cert.NotAfter.ToString('yyyy-MM-dd'))"   -ForegroundColor Green
        $certType = "SelfSigned"

        # Trust locally so Windows doesn't flag it as completely unknown
        Write-Step "Adding to Trusted Root store (may require admin prompt)..."
        try {
            $rootStore = New-Object System.Security.Cryptography.X509Certificates.X509Store("Root", "LocalMachine")
            $rootStore.Open("ReadWrite")
            $rootStore.Add($cert)
            $rootStore.Close()
            Write-OK "Added to LocalMachine\Root"
        } catch {
            Write-Warn "Could not add to LocalMachine\Root (no admin rights)"
            Write-Warn "Run this script as Administrator once to trust the certificate locally."
        }
    }
}

# ── Sign the exe ──────────────────────────────────────────────────────────────
if (-not (Test-Path $ExePath)) {
    Write-Err "Executable not found: $ExePath"
    Write-Warn "Run build-installer.bat first."
    exit 1
}

Write-Host ""
Write-Step "Signing: $ExePath"
Write-Step "Certificate type: $certType"

$signResult = $null

# Attempt with timestamp server
try {
    $signResult = Set-AuthenticodeSignature `
        -FilePath        $ExePath `
        -Certificate     $cert `
        -TimestampServer $TimestampServer `
        -HashAlgorithm   SHA256

    if ($signResult.Status -eq "Valid") {
        Write-OK "Signed with timestamp: $TimestampServer"
    } elseif ($signResult.Status -notin @("Valid", "NotSigned")) {
        Write-OK "Signed (status: $($signResult.Status))"
    } else {
        throw "Signing status: $($signResult.Status) — $($signResult.StatusMessage)"
    }
} catch {
    Write-Warn "Timestamp signing failed: $($_.Exception.Message)"
    Write-Warn "Retrying without timestamp server..."

    try {
        $signResult = Set-AuthenticodeSignature `
            -FilePath    $ExePath `
            -Certificate $cert `
            -HashAlgorithm SHA256

        if ($signResult.Status -ne "NotSigned") {
            Write-OK "Signed without timestamp (status: $($signResult.Status))"
        } else {
            Write-Err "Signing failed: $($signResult.StatusMessage)"
            exit 1
        }
    } catch {
        Write-Err "Signing failed: $($_.Exception.Message)"
        exit 1
    }
}

# ── File hash display after signing ──────────────────────────────────────────
Write-Host ""
Write-Step "File hash after signing:"
$fileHash = Get-FileHash -Path $ExePath -Algorithm SHA256
Write-Host "    SHA-256 : $($fileHash.Hash)" -ForegroundColor White
$md5Hash  = Get-FileHash -Path $ExePath -Algorithm MD5
Write-Host "    MD5     : $($md5Hash.Hash)"  -ForegroundColor White

# ── Verify signature ──────────────────────────────────────────────────────────
Write-Host ""
Write-Step "Verifying signature..."
$check = Get-AuthenticodeSignature $ExePath
Write-Host "    Publisher : $($check.SignerCertificate.Subject)"              -ForegroundColor White
Write-Host "    Status    : $($check.Status)"                                  -ForegroundColor White
Write-Host "    Thumbprint: $($check.SignerCertificate.Thumbprint)"           -ForegroundColor White
if ($check.TimeStamperCertificate) {
    Write-Host "    Timestamp : $($check.TimeStamperCertificate.Subject)"     -ForegroundColor White
} else {
    Write-Host "    Timestamp : (none — binary will appear unsigned after cert expiry)" -ForegroundColor Yellow
}

Write-Host ""
Write-OK "Signing complete. Publisher: $CertName"

if ($certType -eq "SelfSigned") {
    Write-Warn "Self-signed certificate = yellow UAC warning."
    Write-Warn "For green (trusted) UAC, purchase an EV code-signing cert from DigiCert / Sectigo."
} else {
    Write-OK "EV certificate — UAC will show green 'verified publisher' prompt."
}
