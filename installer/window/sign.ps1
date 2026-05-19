#Requires -Version 5.1
<#
.SYNOPSIS
    Sign dri-install.exe with a self-signed Authenticode certificate.
    Publisher shown in UAC: "drewdrew0414"
    Note: self-signed = yellow UAC warning ("unverified publisher").
    For green UAC, purchase an EV code-signing cert from DigiCert / Sectigo.
#>
param(
    [string]$ExePath  = "$PSScriptRoot\dri-install.exe",
    [string]$CertName = "drewdrew0414"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step { param($m) Write-Host "  ► $m" -ForegroundColor Cyan }
function Write-OK   { param($m) Write-Host "  ✓ $m" -ForegroundColor Green }
function Write-Warn { param($m) Write-Host "  ⚠ $m" -ForegroundColor Yellow }
function Write-Err  { param($m) Write-Host "  ✗ $m" -ForegroundColor Red }

# ── Locate or create self-signed certificate ─────────────────────────────────
Write-Step "Looking for existing certificate: CN=$CertName ..."

$cert = Get-ChildItem Cert:\CurrentUser\My |
        Where-Object { $_.Subject -eq "CN=$CertName" -and $_.EnhancedKeyUsageList.ObjectId -contains "1.3.6.1.5.5.7.3.3" } |
        Sort-Object NotAfter -Descending |
        Select-Object -First 1

if ($cert) {
    Write-OK "Found: $($cert.Thumbprint)  expires $($cert.NotAfter.ToString('yyyy-MM-dd'))"
} else {
    Write-Step "Creating self-signed code-signing certificate..."

    # New-SelfSignedCertificate requires PowerShell 5.1+ and Windows 8.1+
    $cert = New-SelfSignedCertificate `
        -Subject        "CN=$CertName" `
        -Type           CodeSigning `
        -KeyUsage       DigitalSignature `
        -FriendlyName   "dri installer signing key" `
        -CertStoreLocation Cert:\CurrentUser\My `
        -NotAfter       (Get-Date).AddYears(5)

    Write-OK "Created: $($cert.Thumbprint)  expires $($cert.NotAfter.ToString('yyyy-MM-dd'))"

    # Trust the certificate locally so Windows doesn't flag it as completely unknown
    Write-Step "Adding to Trusted Root store (requires admin prompt)..."
    try {
        $rootStore = New-Object System.Security.Cryptography.X509Certificates.X509Store("Root","LocalMachine")
        $rootStore.Open("ReadWrite")
        $rootStore.Add($cert)
        $rootStore.Close()
        Write-OK "Added to LocalMachine\Root"
    } catch {
        Write-Warn "Could not add to LocalMachine\Root (no admin rights) — UAC will still show yellow warning"
        Write-Warn "Run this script as Administrator once to trust the certificate."
    }
}

# ── Sign the exe ─────────────────────────────────────────────────────────────
if (-not (Test-Path $ExePath)) {
    Write-Err "Exe not found: $ExePath"
    Write-Warn "Run build-installer.bat first."
    exit 1
}

Write-Step "Signing: $ExePath ..."

$result = Set-AuthenticodeSignature -FilePath $ExePath -Certificate $cert -TimestampServer "http://timestamp.digicert.com"

if ($result.Status -eq "Valid") {
    Write-OK "Signed successfully  (status: $($result.Status))"
} elseif ($result.Status -eq "UnknownError" -or $result.Status -eq "NotSigned") {
    # Retry without timestamp server (no internet needed)
    Write-Warn "Timestamp server unreachable — signing without timestamp..."
    $result = Set-AuthenticodeSignature -FilePath $ExePath -Certificate $cert
    if ($result.Status -ne "NotSigned") {
        Write-OK "Signed (no timestamp)  status: $($result.Status)"
    } else {
        Write-Err "Signing failed: $($result.StatusMessage)"
        exit 1
    }
} else {
    Write-OK "Signed  status: $($result.Status)"
}

# ── Verify ───────────────────────────────────────────────────────────────────
Write-Step "Verifying signature..."
$check = Get-AuthenticodeSignature $ExePath
Write-Host "  Publisher : $($check.SignerCertificate.Subject)"
Write-Host "  Status    : $($check.Status)"
Write-Host "  Thumbprint: $($check.SignerCertificate.Thumbprint)"

Write-Host ""
Write-OK "Done. UAC will now show publisher: $CertName"
Write-Warn "Note: self-signed cert = yellow UAC warning."
Write-Warn "For green (trusted) UAC, use an EV code-signing cert from a commercial CA."
