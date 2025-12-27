# Leopard Security Patches (Mac OS X 10.5.x)

Security patches for Mac OS X Leopard that Apple no longer supports.

## WARNING - EXPERIMENTAL

**These patches are EXPERIMENTAL and have NOT been tested on all hardware configurations.**

- Use at your own risk
- Make a FULL BACKUP before installing
- Test on non-production machines first
- Some patches may cause issues on certain hardware

## Supported Configurations

| Hardware | Status | Notes |
|----------|--------|-------|
| Power Mac G4 | Planned | Building/testing |
| Power Mac G5 | Planned | Primary target |
| PowerBook G4 | Planned | Should work |
| iMac G4/G5 | Planned | Should work |
| MacBook (Intel) | Planned | Universal binary |
| Mac mini (Intel/PPC) | Planned | Should work |

## CVEs Addressed

| CVE | Description | Severity | Method |
|-----|-------------|----------|--------|
| CVE-2008-1447 | DNS cache poisoning (Kaminsky) | High | Port randomization via DYLD interpose |
| CVE-2009-2414 | TCP ISN hijacking | Medium | ISN entropy enhancement |
| CVE-2010-0036 | HFS+ integer overflow | High | Userspace bounds checking |
| CVE-2011-0182 | Font parsing RCE | Critical | Font header validation |
| CVE-2014-4377 | IOKit privilege escalation | High | ioctl bounds checking |

## How It Works

These patches use **DYLD_INSERT_LIBRARIES** to inject security checks into running processes.
This is a userspace mitigation approach that:

- Intercepts dangerous system calls (bind, open, ioctl, etc.)
- Validates input before passing to kernel
- Blocks obviously malicious requests
- Logs blocked attempts (in debug mode)

**Limitations:**
- Cannot fix actual kernel bugs (would require XNU rebuild)
- Processes that clear DYLD_* variables bypass protection
- Some system daemons may not load the libraries
- Performance overhead is minimal but exists

## Building

### Requirements

- Mac OS X 10.5.8 Leopard
- Xcode 3.1.4 with GCC 4.2 (or GCC 10 from MacPorts/Tigerbrew)
- Admin access for installation

### Build Commands

```bash
cd /path/to/leopard/
./BUILD_LEOPARD_PKG.sh
```

This creates:
- `LeopardSecurityPatches.pkg` - Installer package
- `LeopardSecurityPatches.dmg` - Disk image (if hdiutil available)

### Cross-Compiling (from Linux)

You can cross-compile using a PowerPC cross-toolchain, but the resulting
dylibs should be tested on real hardware before deployment.

## Installation

### From DMG

```bash
hdiutil attach LeopardSecurityPatches.dmg
sudo installer -pkg /Volumes/Leopard\ Security\ Patches/LeopardSecurityPatches.pkg -target /
sudo reboot
```

### Manual Installation

```bash
# Copy dylibs
sudo mkdir -p /Library/Security/LeopardPatches
sudo cp build/patches/*.dylib /Library/Security/LeopardPatches/
sudo chmod 644 /Library/Security/LeopardPatches/*.dylib
sudo chown -R root:wheel /Library/Security/LeopardPatches

# Add to profile (for shell processes)
echo 'export DYLD_INSERT_LIBRARIES="/Library/Security/LeopardPatches/dns_randomizer.dylib:/Library/Security/LeopardPatches/tcp_isn_randomizer.dylib:/Library/Security/LeopardPatches/hfs_overflow_guard.dylib:/Library/Security/LeopardPatches/font_parsing_guard.dylib:/Library/Security/LeopardPatches/iokit_bounds_guard.dylib"' | sudo tee -a /etc/profile

sudo reboot
```

## Uninstallation

```bash
sudo rm -rf /Library/Security/LeopardPatches
sudo rm /System/Library/LaunchDaemons/com.elya.leopard-dns-security.plist
sudo rm /etc/profile.d/leopard_security.sh
sudo sed -i '' '/leopard_security/d' /etc/profile
sudo reboot
```

## Verification

After installation, verify patches are loaded:

```bash
# Check environment
echo $DYLD_INSERT_LIBRARIES

# Check loaded libraries in a process
DYLD_PRINT_LIBRARIES=1 /bin/ls 2>&1 | grep LeopardPatches

# Check DNS randomization (should see random high ports)
sudo tcpdump -i en0 port 53
nslookup example.com
```

## Known Issues

1. **System Preferences**: May show warnings about "unsigned code"
2. **Some GUI apps**: Certain apps clear DYLD variables for security
3. **Boot time**: First boot after install may be slightly slower
4. **Rosetta apps**: Intel apps under Rosetta may not load PPC dylibs

## Technical Details

### Directory Structure

```
/Library/Security/LeopardPatches/
├── dns_randomizer.dylib       # CVE-2008-1447
├── tcp_isn_randomizer.dylib   # CVE-2009-2414
├── hfs_overflow_guard.dylib   # CVE-2010-0036
├── font_parsing_guard.dylib   # CVE-2011-0182
└── iokit_bounds_guard.dylib   # CVE-2014-4377

/etc/profile.d/
└── leopard_security.sh        # Shell environment loader

/System/Library/LaunchDaemons/
└── com.elya.leopard-dns-security.plist  # DNS security loader
```

### Differences from Tiger Patches

| Aspect | Tiger (10.4) | Leopard (10.5) |
|--------|--------------|----------------|
| DNS Service | lookupd | mDNSResponder |
| Init System | mach_init | launchd |
| DYLD Interpose | Manual hooks | __DATA,__interpose section |
| Security Framework | Basic | Improved (ASLR partial) |

## Credits

- **Scott (Scottcjn)** - Creator, architect, hardware lab
- **Claude (Opus 4.5)** - Implementation assistance

*"Keeping vintage Macs secure, one CVE at a time."*

## License

MIT License - Use at your own risk.

## Community

Join the RustChain Discord for support and updates:

[![Discord](https://img.shields.io/badge/Discord-RustChain-7289DA?logo=discord&logoColor=white)](https://discord.gg/tQ4q3z4M)
