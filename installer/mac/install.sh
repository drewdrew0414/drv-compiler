#!/usr/bin/env bash
# ──────────────────────────────────────────────────────────────────────────────
#  dri Language Compiler — Linux/macOS installer
#  Path: https://drvpm-registry.cloud/dist/v0.1.0/linux/install.sh
#
#  Usage:
#    curl -fsSL https://drvpm-registry.cloud/dist/v0.1.0/linux/install.sh | bash
#    ./install.sh [--install-dir <path>] [--no-vscode] [--uninstall]
# ──────────────────────────────────────────────────────────────────────────────

set -euo pipefail

# ── 1. Constants / environment ────────────────────────────────────────────────
VERSION="0.1.0"
REGISTRY_URL="https://drvpm-registry.cloud"
RELEASE_URL="${REGISTRY_URL}/dist/v${VERSION}"
VSIX_NAME="dri-lang-${VERSION}.vsix"

OS_TYPE="$(uname -s | tr '[:upper:]' '[:lower:]')"
ARCH_TYPE="$(uname -m)"

DEFAULT_INSTALL_DIR="${HOME}/.local/share/dri"
INSTALL_DIR="${DEFAULT_INSTALL_DIR}"
SKIP_VSCODE=false
UNINSTALL=false

# Detect OS and supported architecture early
case "${OS_TYPE}" in
    darwin)
        COMPILER_NAME="dri"
        SHELL_RC_FILES=("${HOME}/.zshrc" "${HOME}/.bash_profile" "${HOME}/.bashrc")
        ;;
    linux)
        COMPILER_NAME="dri"
        SHELL_RC_FILES=("${HOME}/.bashrc" "${HOME}/.profile")
        ;;
    *)
        printf '\033[31m✗ Unsupported OS: %s\033[0m\n' "$(uname -s)" >&2
        exit 1
        ;;
esac

case "${ARCH_TYPE}" in
    x86_64|amd64|arm64|aarch64) ;;  # supported
    *)
        printf '\033[33m⚠ Unverified architecture: %s — proceeding anyway\033[0m\n' \
            "${ARCH_TYPE}" >&2
        ;;
esac

# ── 2. Argument parsing ───────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --install-dir)
            if [[ $# -lt 2 ]]; then
                printf '✗ --install-dir requires a path argument\n' >&2
                exit 2
            fi
            INSTALL_DIR="$2"; shift 2 ;;
        --no-vscode|-NoVSCode)   SKIP_VSCODE=true; shift ;;
        --uninstall|-Uninstall)  UNINSTALL=true;   shift ;;
        -h|--help)
            cat <<EOF
Usage: $0 [options]
Options:
  --install-dir <path>    Custom install directory (default: ${DEFAULT_INSTALL_DIR})
  --no-vscode             Skip installing the VSCode extension
  --uninstall             Remove an existing dri install
  -h, --help              Show this help
EOF
            exit 0 ;;
        *) printf '✗ Unknown option: %s\n' "$1" >&2; exit 1 ;;
    esac
done

# ── 3. Output helpers ─────────────────────────────────────────────────────────
write_step() { printf '  \033[36m►\033[0m %s\n' "$1"; }
write_ok()   { printf '  \033[32m✓\033[0m %s\n' "$1"; }
write_warn() { printf '  \033[33m⚠\033[0m %s\n' "$1"; }
write_err()  { printf '  \033[31m✗\033[0m %s\n' "$1" >&2; }

# ── 4. EULA prompt (Zsh/Bash safe, also works in piped curl|bash flows) ───────
check_eula() {
    cat <<'EOF'
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  dri Language Compiler — End User License Agreement
EOF
    printf '  Version %s\n' "${VERSION}"
    cat <<'EOF'
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  This software downloads the compiler binary from
  drvpm-registry.cloud. By installing and using it you
  accept the license terms (personal & commercial use).
  Reverse engineering and unauthorised redistribution
  as an independent product are prohibited.
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

EOF

    if [[ -t 0 ]]; then
        printf ' Accept the agreement and continue? (y/N): '
        read -r EULA_AGREE
    elif [[ -e /dev/tty ]]; then
        printf ' Accept the agreement and continue? (y/N): '
        read -r EULA_AGREE < /dev/tty
    else
        write_err "Non-interactive shell with no /dev/tty — re-run with a terminal."
        exit 1
    fi

    case "${EULA_AGREE}" in
        [yY][eE][sS]|[yY]) printf '\n' ;;
        *) write_err "EULA not accepted — aborting installation."; exit 1 ;;
    esac
}

# ── 5. Uninstall ──────────────────────────────────────────────────────────────
invoke_uninstall() {
    write_step "Removing dri compiler…"
    if [[ -d "${INSTALL_DIR}" ]]; then
        rm -rf -- "${INSTALL_DIR}"
        write_ok "Install directory removed: ${INSTALL_DIR}"
    else
        write_warn "Install directory not found: ${INSTALL_DIR}"
    fi
    write_warn "PATH entries in shell rc files should be cleaned up manually."
    exit 0
}

# ── 6. Local-source fallback build ────────────────────────────────────────────
build_from_source() {
    local out_path="$1"
    write_step "Remote binary unavailable — attempting local source build…"

    local cxx=""
    if   command -v clang++ >/dev/null 2>&1; then cxx="clang++"
    elif command -v g++     >/dev/null 2>&1; then cxx="g++"
    fi

    if [[ -z "${cxx}" ]]; then
        write_warn "No C++ compiler found (need clang++ or g++)."
        return 1
    fi

    if [[ ! -f ./main.cpp ]]; then
        write_warn "Cannot find ./main.cpp for source build (run from repo root)."
        return 1
    fi

    local src_files=(
        ./main.cpp
        ./compiler/src/lexer.cpp
        ./compiler/src/parser.cpp
        ./compiler/src/codegen.cpp
        ./compiler/src/compiler.cpp
        ./compiler/src/analyzer.cpp
        ./compiler/src/incremental.cpp
    )

    write_step "Compiling with ${cxx}…"
    if "${cxx}" -std=c++20 -O2 -I. "${src_files[@]}" -o "${out_path}"; then
        write_ok "Local build succeeded: ${out_path}"
        return 0
    fi
    write_err "Local build failed."
    return 1
}

# ── VS Code installation path detection ──────────────────────────────────────
# Returns the bin/ directory inside the VS Code app bundle, or an empty string.
find_vscode_bin() {
    # 1. If 'code' is already on PATH, resolve it back to the bin dir
    if command -v code >/dev/null 2>&1; then
        local resolved
        resolved="$(dirname "$(command -v code)")"
        echo "${resolved}"
        return 0
    fi

    if [[ "${OS_TYPE}" == "darwin" ]]; then
        # 2a. macOS: use Spotlight (mdfind) to locate the app bundle
        if command -v mdfind >/dev/null 2>&1; then
            local app
            app="$(mdfind "kMDItemCFBundleIdentifier == 'com.microsoft.VSCode'" \
                   2>/dev/null | head -1)"
            if [[ -n "${app}" && -d "${app}/Contents/Resources/app/bin" ]]; then
                echo "${app}/Contents/Resources/app/bin"
                return 0
            fi
        fi
        # 2b. Fall back to the two most common fixed locations
        local mac_candidates=(
            "${HOME}/Applications/Visual Studio Code.app/Contents/Resources/app/bin"
            "/Applications/Visual Studio Code.app/Contents/Resources/app/bin"
        )
        for c in "${mac_candidates[@]}"; do
            if [[ -d "${c}" ]]; then echo "${c}"; return 0; fi
        done
    else
        # 2c. Linux: check common install locations
        local linux_candidates=(
            "/usr/bin"
            "/usr/share/code/bin"
            "/usr/lib/code/bin"
            "/snap/bin"
            "${HOME}/.local/share/code/bin"
        )
        for c in "${linux_candidates[@]}"; do
            if [[ -x "${c}/code" ]]; then echo "${c}"; return 0; fi
        done
    fi

    # Not found
    return 1
}

# Detect content type of a downloaded file — distinguish HTML 404 page from
# real binary even when the server returned HTTP 200.
looks_like_html() {
    local file="$1"
    [[ -s "${file}" ]] || return 0
    # Read first 256 bytes and look for telltale HTML markers
    local sample
    sample=$(head -c 256 -- "${file}" 2>/dev/null | tr '[:upper:]' '[:lower:]')
    case "${sample}" in
        *"<!doctype html"*|*"<html"*|*"<head"*|*"<body"*) return 0 ;;
    esac
    return 1
}

# ── 7. Main install flow ──────────────────────────────────────────────────────
if [[ "${UNINSTALL}" == "true" ]]; then
    invoke_uninstall
fi

check_eula

write_step "Preparing install directory…"
mkdir -p -- "${INSTALL_DIR}"

compiler_path="${INSTALL_DIR}/${COMPILER_NAME}"
downloaded=false

# Build OS-specific download URL
if [[ "${OS_TYPE}" == "darwin" ]]; then
    DOWNLOAD_URL="${RELEASE_URL}/mac/${COMPILER_NAME}"
else
    DOWNLOAD_URL="${RELEASE_URL}/linux/${COMPILER_NAME}"
fi

write_step "Downloading binary from ${DOWNLOAD_URL}…"

# Use `-f` (curl) or `--server-response` (wget) so that 4xx/5xx HTML responses
# are treated as failures and never written to disk.
if command -v curl >/dev/null 2>&1; then
    if curl -fsSL --connect-timeout 10 --max-time 300 \
            -o "${compiler_path}" "${DOWNLOAD_URL}"; then
        downloaded=true
    fi
elif command -v wget >/dev/null 2>&1; then
    if wget --quiet --tries=2 --timeout=30 \
            -O "${compiler_path}" "${DOWNLOAD_URL}"; then
        downloaded=true
    fi
else
    write_warn "Neither curl nor wget found — skipping remote download."
fi

# Validate downloaded payload: drop empty / HTML-error responses before chmod.
if [[ "${downloaded}" != "true" ]] || \
   [[ ! -s "${compiler_path}" ]]   || \
   looks_like_html "${compiler_path}"; then
    rm -f -- "${compiler_path}"
    if ! build_from_source "${compiler_path}"; then
        write_err "Install failed: remote binary at ${DOWNLOAD_URL} unavailable"
        write_err "and local source build did not succeed."
        exit 1
    fi
fi

chmod +x "${compiler_path}"
write_ok "Compiler installed: ${compiler_path}"

# ── PATH registration ─────────────────────────────────────────────────────────
write_step "Registering PATH…"
PATH_LINE="export PATH=\"\$PATH:${INSTALL_DIR}\""
for rc_file in "${SHELL_RC_FILES[@]}"; do
    if [[ -f "${rc_file}" ]]; then
        if ! grep -qsF "${INSTALL_DIR}" "${rc_file}"; then
            {
                printf '\n# dri Language Compiler PATH\n'
                printf '%s\n' "${PATH_LINE}"
            } >> "${rc_file}"
            write_ok "PATH appended to: ${rc_file}"
        fi
    fi
done

# ── VSCode extension (optional) ───────────────────────────────────────────────
# VSIX URL: https://drvpm-registry.cloud/dist/v<VERSION>/dri-lang-<VERSION>.vsix
if [[ "${SKIP_VSCODE}" != "true" ]]; then
    vsix_path="${INSTALL_DIR}/${VSIX_NAME}"
    vsix_url="${RELEASE_URL}/${VSIX_NAME}"

    write_step "Downloading VSCode extension from ${vsix_url}…"

    vsix_ok=false
    if command -v curl >/dev/null 2>&1; then
        if curl -fsSL --connect-timeout 10 --max-time 120 \
                -o "${vsix_path}" "${vsix_url}" 2>/dev/null; then
            vsix_ok=true
        fi
    elif command -v wget >/dev/null 2>&1; then
        if wget --quiet --tries=2 --timeout=30 \
                -O "${vsix_path}" "${vsix_url}" 2>/dev/null; then
            vsix_ok=true
        fi
    fi

    if [[ "${vsix_ok}" == "true" ]] && [[ -s "${vsix_path}" ]] && \
       ! looks_like_html "${vsix_path}"; then
        write_ok "Extension downloaded: ${vsix_path}"

        # Try every known VS Code–compatible CLI in priority order:
        #   code           → Visual Studio Code (stable)
        #   code-insiders  → VS Code Insiders
        #   cursor         → Cursor IDE
        #   codium         → VSCodium
        installed_ext=false
        for editor_cmd in code code-insiders cursor codium; do
            if command -v "${editor_cmd}" >/dev/null 2>&1; then
                write_step "Installing extension into ${editor_cmd}…"
                if "${editor_cmd}" --install-extension "${vsix_path}" --force \
                        >/dev/null 2>&1; then
                    write_ok "Extension installed into ${editor_cmd}."
                    installed_ext=true
                    break
                else
                    write_warn "${editor_cmd} --install-extension failed."
                fi
            fi
        done

        if [[ "${installed_ext}" == "false" ]]; then
            write_warn "No supported editor CLI found (code / cursor / codium)."
            # Keep the VSIX on disk so the user can install it manually

            printf '\n'
            printf '  \033[33m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n'
            printf '  \033[33m  How to enable the VS Code "code" CLI\033[0m\n'
            printf '  \033[33m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n'
            printf '\n'

            # Try to locate VS Code on this machine right now
            detected_bin="$(find_vscode_bin || true)"

            if [[ "${OS_TYPE}" == "darwin" ]]; then
                printf '  \033[36m[macOS]\033[0m The "code" command is not on PATH yet.\n'
                printf '  Due to macOS sandboxing, one-time setup is required.\n'
                printf '\n'
                printf '  \033[1mStep 1 — Move VS Code to Applications\033[0m\n'
                printf '     Drag "Visual Studio Code.app" from Downloads into\n'
                printf '     /Applications, then launch it from there.\n'
                printf '\n'
                printf '  \033[1mStep 2 — Register the CLI (one-time)\033[0m\n'
                printf '     Inside VS Code press  \033[32mCmd + Shift + P\033[0m\n'
                printf '     Type  \033[32mshell command\033[0m  and select:\n'
                printf '     \033[32mShell Command: Install '"'"'code'"'"' command in PATH\033[0m\n'
                printf '\n'
                printf '  \033[1mStep 3 — Restart your terminal\033[0m\n'
                printf '     Quit the terminal app fully and reopen it.\n'
                printf '\n'
                printf '  \033[1m⚠  If the command stops working after a reboot:\033[0m\n'
                if [[ -n "${detected_bin}" ]]; then
                    # Show the exact path found on this machine
                    printf '     VS Code was found at: \033[32m%s\033[0m\n' "${detected_bin}"
                    printf '     Run these two commands once:\n'
                    printf '\n'
                    printf '     \033[32mecho '"'"'export PATH="$PATH:%s"'"'"' >> ~/.zshrc\033[0m\n' \
                           "${detected_bin}"
                    printf '     \033[32msource ~/.zshrc\033[0m\n'
                else
                    # VS Code not found yet — give a generic dynamic snippet
                    printf '     VS Code was not found at a standard location.\n'
                    printf '     Once you install VS Code, run:\n'
                    printf '\n'
                    printf '     \033[32mVSC_BIN=$(mdfind "kMDItemCFBundleIdentifier == '"'"'com.microsoft.VSCode'"'"'" | head -1)\033[0m\n'
                    printf '     \033[32mecho "export PATH=\\"\\$PATH:\\${VSC_BIN}/Contents/Resources/app/bin\\"" >> ~/.zshrc\033[0m\n'
                    printf '     \033[32msource ~/.zshrc\033[0m\n'
                fi
                printf '\n'
            else
                printf '  \033[36m[Linux]\033[0m Add the VS Code binary directory to PATH:\n'
                printf '\n'
                if [[ -n "${detected_bin}" ]]; then
                    printf '     VS Code was found at: \033[32m%s\033[0m\n' "${detected_bin}"
                    printf '\n'
                    printf '     \033[32mecho '"'"'export PATH="$PATH:%s"'"'"' >> ~/.bashrc\033[0m\n' \
                           "${detected_bin}"
                    printf '     \033[32msource ~/.bashrc\033[0m\n'
                else
                    printf '     VS Code was not found at a standard location.\n'
                    printf '     After installing VS Code, find its binary and run:\n'
                    printf '\n'
                    printf '     \033[32mecho "export PATH=\\"\\$PATH:\\$(dirname \\$(which code))\\"" >> ~/.bashrc\033[0m\n'
                    printf '     \033[32msource ~/.bashrc\033[0m\n'
                fi
                printf '\n'
                printf '  Alternatively, install VS Code via your package manager\n'
                printf '  (snap / apt / rpm) — the "code" command is added automatically.\n'
                printf '\n'
            fi

            printf '  \033[1mVerify the setup:\033[0m\n'
            printf '     \033[32mcode --version\033[0m\n'
            printf '\n'
            printf '  \033[1mThen install the dri extension manually:\033[0m\n'
            printf '     \033[32mcode --install-extension %s\033[0m\n' "${vsix_path}"
            printf '\n'
            printf '  \033[33m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n'
            printf '\n'
        fi
    else
        write_warn "Could not download the VSCode extension (skipping)."
        rm -f -- "${vsix_path}"
    fi
fi

# ── 8. Inject PATH into current shell session ─────────────────────────────────
export PATH="${PATH}:${INSTALL_DIR}"

printf '\n'
write_ok "dri compiler v${VERSION} installed successfully."
write_step "PATH has been updated for the current shell."
printf '\n  [Verify install]\n'
printf '    \033[32mdri --version\033[0m\n\n'
