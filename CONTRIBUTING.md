# Contributing to Mac OS X Security Patches

Thank you for contributing to Mac OS X Security Patches, which provides security patches for legacy Mac OS X versions that Apple no longer supports.

## Project Overview

This project backports security fixes from newer macOS versions to legacy PowerPC and Intel-based Macs running older OS X versions. **Warning: These patches are experimental and modify system files.**

## ⚠️ Important Warnings

- These patches modify system files — **always back up before applying**
- Test on non-critical machines first
- Some patches may break functionality or void warranties
- Not all patches are suitable for all systems

## Development Setup

### Prerequisites

- Legacy Mac with PowerPC (G4/G5) or Intel CPU
- Mac OS X 10.4-10.6 installed
- Xcode Command Line Tools (for building patches)
- sudo/root access

### Building Patches

```bash
git clone https://github.com/Scottcjn/macosx-security-patches.git
cd macosx-security-patches

# List available patches
ls patches/

# Build a specific patch
cd patches/CVE-XXXX-XXXXX
make

# Test in safe mode first
sudo make test
```

## Testing

```bash
# Verify patch integrity
shasum -a 256 patches/*/*.patch

# Test patch application
sudo make test DRY_RUN=1

# Check system integrity after patch
./scripts/verify-system.sh
```

## Submitting Changes

1. Fork the repository
2. Create a branch: `git checkout -b patch/CVE-XXXX-XXXXX`
3. Follow the patch submission guidelines in `docs/SUBMISSION.md`
4. Test on real hardware
5. Submit a pull request

## Ideas for Contributions

- Backport additional CVE patches
- Support for more OS X versions
- Automated build/CI for patches
- Documentation improvements
- Hardware compatibility reports
