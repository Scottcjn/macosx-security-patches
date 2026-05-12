# Contributing to macosx-security-patches

Thanks for helping improve the Mac OS X Tiger and Leopard security patch
collection. These changes affect legacy systems and security-sensitive install
paths, so contributions should be conservative, reversible, and well documented.

## Setup

1. Fork the repository and create a branch:

   ```sh
   git checkout -b fix/short-description
   ```

2. Read the root `README.md`, `README_KERNEL_PATCHES.md`, and
   `leopard/README.md` before changing installer or patch behavior.

3. On a compatible Mac OS X test system, build the relevant package:

   ```sh
   ./BUILD_KERNEL_PKG.sh
   cd leopard
   ./BUILD_LEOPARD_PKG.sh
   ```

4. For individual Tiger kernel patch work, use the scripts under `scripts/` and
   the relevant `kernel_patches/CVE-*` directory.

5. Always test on a non-production machine or VM snapshot first. Keep a full
   backup and document the hardware model, OS version, and build number.

## Pull Request Guidelines

- Keep one CVE, installer behavior change, or documentation update per PR.
- Explain the safety impact and rollback path.
- Include exact build and verification commands.
- Update affected README files when install, uninstall, or compatibility details
  change.
- Do not commit generated DMGs, package outputs, build directories, logs, or
  machine-specific paths unless they are intentionally tracked release assets.

## Code Style

- Shell scripts should quote paths and fail clearly on missing tools or unsafe
  privileges.
- C patch code should keep CVE-specific behavior isolated in its existing
  directory.
- Prefer explicit warnings for experimental or hardware-dependent behavior.
- Avoid silent changes to system-wide launchd, DYLD, or installer paths.
- Keep security claims tied to documented tests or clearly mark untested areas.

## Validation Checklist

- [ ] Relevant package or patch build was run on compatible Mac OS X hardware,
  or the limitation is documented in the PR.
- [ ] Install and uninstall steps were reviewed for reversibility.
- [ ] Security behavior is tied to a CVE-specific test or documented evidence.
- [ ] README files were updated for changed commands or risks.
- [ ] Generated packages, DMGs, logs, and local build artifacts are excluded.
