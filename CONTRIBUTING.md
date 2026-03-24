# Contributing to macOS X Security Patches

Thank you for your interest in contributing to the macOS X Security Patches project! This document provides guidelines and instructions for contributing.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [How Can I Contribute?](#how-can-i-contribute)
- [Development Setup](#development-setup)
- [Submitting Changes](#submitting-changes)
- [Style Guidelines](#style-guidelines)
- [Community](#community)

## Code of Conduct

This project and everyone participating in it is governed by our commitment to:

- Be respectful and inclusive
- Welcome newcomers and help them learn
- Focus on constructive feedback
- Respect different viewpoints and experiences

## Getting Started

### Prerequisites

To contribute to this project, you'll need:

- A Mac running macOS X (10.4 Tiger or later recommended)
- Basic knowledge of shell scripting
- Understanding of macOS system architecture
- Familiarity with security concepts

### Repository Structure

```
macosx-security-patches/
├── patches/          # Security patch scripts
├── docs/             # Documentation
├── tests/            # Test suites
├── tools/            # Utility scripts
└── README.md         # Project overview
```

## How Can I Contribute?

### Reporting Security Vulnerabilities

**⚠️ IMPORTANT:** Do NOT open public issues for security vulnerabilities.

Instead:
1. Email security concerns to the maintainers privately
2. Allow time for assessment and patch development
3. Coordinate disclosure timeline

### Reporting Bugs

When reporting bugs, please include:

- **macOS Version:** (e.g., 10.4.11 Tiger)
- **Hardware:** (e.g., PowerBook G4, iMac G5)
- **Patch Version:** The specific patch or script affected
- **Steps to Reproduce:** Clear steps to reproduce the issue
- **Expected Behavior:** What you expected to happen
- **Actual Behavior:** What actually happened
- **Logs:** Relevant system logs or error messages

### Suggesting Enhancements

Enhancement suggestions are welcome! Please provide:

- Clear description of the enhancement
- Rationale for why it would be useful
- Potential implementation approach
- Any security implications

### Contributing Patches

#### Types of Patches We Accept

1. **Security Fixes:** Patches addressing known vulnerabilities
2. **Hardening Scripts:** Configuration improvements for security
3. **Documentation:** Security best practices and guides
4. **Testing Tools:** Scripts to verify security settings

#### Patch Requirements

All patches must:

- Include clear documentation of what vulnerability/issue they address
- Provide rollback instructions
- Include test cases where applicable
- Follow the shell scripting style guidelines
- Be tested on relevant macOS X versions

## Development Setup

### Setting Up Your Development Environment

1. **Fork the Repository**
   ```bash
   # Fork via GitHub UI, then clone your fork
   git clone https://github.com/YOUR_USERNAME/macosx-security-patches.git
   cd macosx-security-patches
   ```

2. **Create a Branch**
   ```bash
   git checkout -b feature/your-patch-name
   ```

3. **Test Environment Setup**
   
   For safe testing, we recommend:
   - Using a spare Mac or virtual machine
   - Creating system backups before testing patches
   - Testing on non-production systems first

### Testing Patches

Before submitting:

1. **Test on Clean System**
   ```bash
   # Document your test environment
   sw_vers -productVersion  # macOS version
   uname -m                 # Architecture
   ```

2. **Verify Patch Application**
   ```bash
   # Apply the patch
   sudo ./patches/your-patch.sh
   
   # Verify the change
   # (document verification steps in your PR)
   ```

3. **Test Rollback**
   ```bash
   # Verify rollback works
   sudo ./patches/your-patch.sh --rollback
   ```

## Submitting Changes

### Pull Request Process

1. **Update Documentation**
   - Update README.md if adding new patches
   - Update relevant documentation files
   - Add entries to CHANGELOG.md

2. **Commit Your Changes**
   ```bash
   git add .
   git commit -m "security: add patch for [vulnerability/issue]"
   ```

3. **Push to Your Fork**
   ```bash
   git push origin feature/your-patch-name
   ```

4. **Open a Pull Request**
   - Use the PR template
   - Reference any related issues
   - Describe testing performed
   - Include macOS versions tested

### Commit Message Guidelines

We follow conventional commits:

- `security:` - Security patches and fixes
- `docs:` - Documentation changes
- `test:` - Adding or updating tests
- `tool:` - Utility script changes
- `chore:` - Maintenance tasks

Examples:
```
security: add patch for CVE-2024-XXXX on Tiger
security: harden SSH configuration for PowerPC Macs
docs: add guide for securing 10.4 Tiger installations
test: add verification script for firewall settings
```

## Style Guidelines

### Shell Script Style

All shell scripts should follow these conventions:

```bash
#!/bin/bash
set -euo pipefail

# Script header with description
# Author: Your Name
# Date: YYYY-MM-DD
# Description: Brief description of what this script does

# Configuration
BACKUP_DIR="/var/backups/security-patches"
LOG_FILE="/var/log/security-patch.log"

# Functions
log_message() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

create_backup() {
    local file="$1"
    local backup_name="${BACKUP_DIR}/$(basename "$file").$(date +%s).bak"
    cp "$file" "$backup_name"
    log_message "Created backup: $backup_name"
}

# Main execution
main() {
    log_message "Starting security patch..."
    # Patch logic here
    log_message "Security patch completed successfully"
}

# Rollback function
rollback() {
    log_message "Rolling back changes..."
    # Rollback logic here
    log_message "Rollback completed"
}

# Handle arguments
case "${1:-}" in
    --rollback)
        rollback
        ;;
    *)
        main
        ;;
esac
```

### Documentation Style

- Use clear, concise language
- Include code examples
- Provide context for security implications
- Reference official Apple documentation when applicable

## Community

### Getting Help

- **GitHub Issues:** For bug reports and feature requests
- **Discussions:** For questions and general discussion
- **Wiki:** For community-contributed guides

### Recognition

Contributors will be:
- Listed in CONTRIBUTORS.md
- Mentioned in release notes for significant contributions
- Credited in patch documentation

## Security Considerations

### For Contributors

- Never commit sensitive information (passwords, API keys)
- Test patches thoroughly before submitting
- Consider edge cases and potential system impacts
- Document any privileged operations clearly

### For Users

- Always review patches before applying
- Create system backups
- Test in non-production environments first
- Understand rollback procedures

## Platform-Specific Notes

### macOS X Tiger (10.4)

- Limited modern security features
- Focus on network-level protections
- SSH hardening is particularly important
- Consider firewall configuration

### macOS X Leopard (10.5)

- Improved security features over Tiger
- Leopard Firewall provides better control
- FileVault available for home directory encryption

### PowerPC vs Intel

Some patches may need platform-specific versions:
- Check `uname -m` for architecture
- PowerPC: `ppc`, `ppc64`, `Power Macintosh`
- Intel: `i386`, `x86_64`

## Thank You!

Your contributions help keep vintage Mac systems secure. Whether you're fixing a bug, adding a feature, or improving documentation, your efforts are appreciated.

---

*Last updated: 2026-03-24*
