# Mac OS X Security Patches (PowerPC & Intel)

Security patches for legacy Mac OS X versions that Apple no longer supports.

## WARNING - EXPERIMENTAL

**These patches are EXPERIMENTAL and have NOT been tested on all hardware configurations.**

- Use at your own risk
- Make a FULL BACKUP before installing
- Test on non-production machines first
- Some patches may cause kernel panics on certain hardware

## Supported Versions

| OS Version | Codename | Architecture | Status |
|------------|----------|--------------|--------|
| Mac OS X 10.3 | Panther | PowerPC | Planned |
| Mac OS X 10.4 | Tiger | PowerPC | **Active** |
| Mac OS X 10.5 | Leopard | PowerPC/Intel | Planned |
| Mac OS X 10.6 | Snow Leopard | Intel | Planned |

## CVEs Addressed

### Tiger (10.4) - Currently Implemented

| CVE | Description | Severity | Method |
|-----|-------------|----------|--------|
| CVE-2008-1447 | DNS cache poisoning (Kaminsky) | High | Port randomization |
| CVE-2009-2414 | TCP ISN hijacking | Medium | ISN randomization |
| CVE-2010-0036 | HFS+ integer overflow | High | Bounds checking |
| CVE-2011-0182 | Font parsing RCE | Critical | Input validation |
| CVE-2014-4377 | IOKit privilege escalation | High | Bounds checking |

### Planned for Future Releases

- CVE-2014-0160 (Heartbleed) - OpenSSL memory disclosure
- CVE-2014-3566 (POODLE) - SSL 3.0 padding oracle
- CVE-2016-0777 - OpenSSH roaming buffer overflow
- Additional kernel and framework patches

## Installation (Tiger)

```bash
# Mount the DMG
hdiutil attach TigerSecurityComplete_FINAL.dmg

# Run the installer
sudo installer -pkg /Volumes/TigerSecurity/TigerKernelCVEPatches.pkg -target /

# Reboot
sudo reboot
```

## Building from Source

### Requirements
- Xcode 2.5 (Tiger) / Xcode 3.1 (Leopard) / Xcode 3.2 (Snow Leopard)
- Kernel Development Kit for your OS version
- Root access for installation

### Build Commands
```bash
./BUILD_KERNEL_PKG.sh

# Creates:
# - TigerKernelCVEPatches.pkg
# - TigerKernelPatches.dmg
```

## Tested Hardware

| Model | OS | Status | Notes |
|-------|-----|--------|-------|
| Power Mac G4 Dual 1.25 | Tiger 10.4.11 | Tested | OK |
| Power Mac G5 Quad | Tiger 10.4.11 | Untested | Should work |
| PowerBook G4 | Tiger 10.4.11 | Untested | Unknown |
| iMac G4 | Tiger 10.4.11 | Partial | AirPort issues |

## Known Issues

1. **AirPort Instability**: The AirPort security patch caused kernel panics on G4 iMac. Removed.
2. **Bluetooth Kext**: IOKit filter may interfere with Bluetooth on some systems.
3. **Boot Delays**: DYLD_INSERT_LIBRARIES adds ~1-2 seconds to app launch.

## Uninstallation

```bash
sudo rm -rf /Library/Security/TigerPatches/
sudo rm /System/Library/LaunchDaemons/com.elya.dns-randomizer.plist
sudo sed -i '' '/TigerPatches/d' /etc/profile
sudo reboot
```

## Credits

- **Scott (Scottcjn)** - Creator, architect, hardware lab, testing
- **Claude (Opus 4.1/4.5)** - Implementation assistance

*Designed by Scott, coded with Claude*

## License

MIT License - Use at your own risk.

---

*"Keeping vintage Macs secure, one CVE at a time."*
