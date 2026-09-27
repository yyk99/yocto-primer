#!/usr/bin/env bash
# Report whether KVM acceleration is usable in this environment, and why not
# if it isn't. Codespaces base images have been inconsistent about this
# (see devcontainers/images#884), so check instead of assuming.
set -euo pipefail

if [ ! -e /dev/kvm ]; then
    echo "no-kvm: /dev/kvm does not exist (host has no nested virtualization exposed here)"
    exit 1
fi

if [ ! -r /dev/kvm ] || [ ! -w /dev/kvm ]; then
    echo "no-kvm: /dev/kvm exists but is not read/writable by $(whoami) (are you in the 'kvm' group? try: newgrp kvm)"
    exit 1
fi

# kvm-ok gives a more thorough CPU-level check, but needs root; skip it
# silently if sudo would prompt (e.g. non-interactive use) rather than
# failing on that alone - direct /dev/kvm access above is the real gate.
if command -v kvm-ok >/dev/null 2>&1 && sudo -n true 2>/dev/null; then
    if ! sudo -n kvm-ok >/tmp/kvm-ok.$$ 2>&1; then
        cat /tmp/kvm-ok.$$
        rm -f /tmp/kvm-ok.$$
        echo "no-kvm: kvm-ok reported KVM unusable"
        exit 1
    fi
    rm -f /tmp/kvm-ok.$$
fi

echo "kvm-ok: /dev/kvm is present and accessible"
