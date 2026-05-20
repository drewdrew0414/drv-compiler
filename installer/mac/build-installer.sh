#!/usr/bin/env bash
# ──────────────────────────────────────────────────────────────────────────────
#  dri Language Compiler — Linux/macOS installer packager
#
#  Produces a distributable installer bundle:
#    dist/dri-installer-<version>-<os>-<arch>.tar.gz
#    dist/dri-installer-<version>-<os>-<arch>.tar.gz.sha256
#
#  The bundle contains:
#    - install.sh    (the installer script)
#    - VERSION       (release version string)
#    - dri           (compiler binary; only when --bundle-binary is passed AND
#                     a freshly built dri executable is available)
#
#  This is the Linux/macOS counterpart of installer/window/build-installer.bat,
#  which compiles launcher.cs into dri-install.exe. Since Linux/macOS use a
#  plain shell installer, "building" here means packaging it into a versioned
#  archive that can be hosted at the release URL.
#
#  Usage:
#    ./build-installer.sh                  # ship install.sh only
#    ./build-installer.sh --bundle-binary  # also embed the compiled compiler
#    ./build-installer.sh --version 0.2.0  # override version label
#    ./build-installer.sh --out dist/      # custom output directory
# ──────────────────────────────────────────────────────────────────────────────

set -euo pipefail

# ── Locate repo root (this script lives at <repo>/installer/linux/) ───────────
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/../.." && pwd)"

# ── Defaults ──────────────────────────────────────────────────────────────────
VERSION="0.1.0"
OUT_DIR="${REPO_ROOT}/dist"
BUNDLE_BINARY=false

OS_TYPE="$(uname -s | tr '[:upper:]' '[:lower:]')"
ARCH_TYPE="$(uname -m)"

# Normalise OS label (darwin → mac)
case "${OS_TYPE}" in
    darwin) OS_LABEL="mac" ;;
    linux)  OS_LABEL="linux" ;;
    *)      OS_LABEL="${OS_TYPE}" ;;
esac

# ── Output helpers ────────────────────────────────────────────────────────────
write_step() { printf '  \033[36m►\033[0m %s\n' "$1"; }
write_ok()   { printf '  \033[32m✓\033[0m %s\n' "$1"; }
write_warn() { printf '  \033[33m⚠\033[0m %s\n' "$1"; }
write_err()  { printf '  \033[31m✗\033[0m %s\n' "$1" >&2; }

# ── Argument parsing ──────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --version)
            [[ $# -ge 2 ]] || { write_err "--version requires a value"; exit 2; }
            VERSION="$2"; shift 2 ;;
        --out)
            [[ $# -ge 2 ]] || { write_err "--out requires a path"; exit 2; }
            OUT_DIR="$2"; shift 2 ;;
        --bundle-binary)
            BUNDLE_BINARY=true; shift ;;
        -h|--help)
            sed -n '2,28p' -- "${BASH_SOURCE[0]}"
            exit 0 ;;
        *)
            write_err "Unknown option: $1"; exit 1 ;;
    esac
done

write_step "Packaging dri installer for ${OS_LABEL}/${ARCH_TYPE} (v${VERSION})…"

# ── Validate installer source ─────────────────────────────────────────────────
INSTALL_SH="${SCRIPT_DIR}/install.sh"
if [[ ! -f "${INSTALL_SH}" ]]; then
    write_err "install.sh not found at ${INSTALL_SH}"
    exit 1
fi

if ! bash -n -- "${INSTALL_SH}"; then
    write_err "install.sh has syntax errors — aborting packaging."
    exit 1
fi
write_ok "install.sh syntax check passed."

# Optional: run shellcheck if available — non-fatal
if command -v shellcheck >/dev/null 2>&1; then
    if shellcheck --severity=error --shell=bash -- "${INSTALL_SH}"; then
        write_ok "shellcheck (errors only) passed."
    else
        write_warn "shellcheck reported issues — see above."
    fi
fi

# ── Stage files into a temp dir ───────────────────────────────────────────────
STAGE_DIR="$(mktemp -d -t dri-installer-stage.XXXXXX)"
cleanup() { rm -rf -- "${STAGE_DIR}"; }
trap cleanup EXIT

BUNDLE_NAME="dri-installer-${VERSION}-${OS_LABEL}-${ARCH_TYPE}"
BUNDLE_ROOT="${STAGE_DIR}/${BUNDLE_NAME}"
mkdir -p -- "${BUNDLE_ROOT}"

cp -- "${INSTALL_SH}" "${BUNDLE_ROOT}/install.sh"
chmod 0755 "${BUNDLE_ROOT}/install.sh"

printf '%s\n' "${VERSION}" > "${BUNDLE_ROOT}/VERSION"

if [[ "${BUNDLE_BINARY}" == "true" ]]; then
    write_step "Locating compiled dri binary…"
    DRI_BIN=""
    for candidate in \
        "${REPO_ROOT}/cmake-build-release/dri" \
        "${REPO_ROOT}/cmake-build-debug/dri"   \
        "${REPO_ROOT}/build/dri"               \
        "${REPO_ROOT}/dri"                     ; do
        if [[ -x "${candidate}" ]]; then
            DRI_BIN="${candidate}"; break
        fi
    done

    if [[ -z "${DRI_BIN}" ]]; then
        write_err "Cannot find a built dri binary — build first or drop --bundle-binary."
        exit 1
    fi

    cp -- "${DRI_BIN}" "${BUNDLE_ROOT}/dri"
    chmod 0755 "${BUNDLE_ROOT}/dri"
    write_ok "Bundled binary: ${DRI_BIN}"
fi

# ── Build the tarball ─────────────────────────────────────────────────────────
mkdir -p -- "${OUT_DIR}"
TAR_NAME="${BUNDLE_NAME}.tar.gz"
TAR_PATH="${OUT_DIR}/${TAR_NAME}"

write_step "Creating ${TAR_PATH}…"
# Use deterministic flags so reruns produce identical archives where possible.
tar -C "${STAGE_DIR}" \
    --owner=0 --group=0 \
    -czf "${TAR_PATH}" "${BUNDLE_NAME}"

# ── Checksum ──────────────────────────────────────────────────────────────────
write_step "Computing SHA-256…"
if command -v shasum >/dev/null 2>&1; then
    (cd "${OUT_DIR}" && shasum -a 256 "${TAR_NAME}" > "${TAR_NAME}.sha256")
elif command -v sha256sum >/dev/null 2>&1; then
    (cd "${OUT_DIR}" && sha256sum "${TAR_NAME}" > "${TAR_NAME}.sha256")
else
    write_warn "Neither shasum nor sha256sum found — skipping checksum file."
fi

write_ok "Build complete:"
printf '    %s\n' "${TAR_PATH}"
[[ -f "${TAR_PATH}.sha256" ]] && printf '    %s\n' "${TAR_PATH}.sha256"

printf '\n  Test install with:\n'
printf '    tar -xzf %s\n' "${TAR_NAME}"
printf '    ./%s/install.sh\n\n' "${BUNDLE_NAME}"
