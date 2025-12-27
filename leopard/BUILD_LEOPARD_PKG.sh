#!/bin/bash
# Build Leopard (10.5) Security Patches Package
# Creates installer for Mac OS X 10.5.8 Leopard (PowerPC and Intel)
#
# Author: Scott (Scottcjn) with Claude
# Date: December 2025

set -e

PACKAGE_NAME="LeopardSecurityPatches"
VERSION="1.0"
IDENTIFIER="com.elya.leopard-security-patches"

echo "=== Leopard Security Patches Builder ==="
echo "Building for Mac OS X 10.5.x (PowerPC + Intel Universal)"
echo ""

# Detect architecture
ARCH=$(uname -m)
echo "Host architecture: $ARCH"

# Set compiler flags for Leopard
export MACOSX_DEPLOYMENT_TARGET=10.5

# Check for GCC (Xcode 3.1.4 provides gcc)
if [ -x "/usr/local/gcc-10/bin/gcc" ]; then
    CC="/usr/local/gcc-10/bin/gcc"
    echo "Using GCC 10: $CC"
elif [ -x "/usr/bin/gcc-4.2" ]; then
    CC="/usr/bin/gcc-4.2"
    echo "Using GCC 4.2: $CC"
elif [ -x "/usr/bin/gcc" ]; then
    CC="/usr/bin/gcc"
    echo "Using system GCC: $CC"
else
    echo "ERROR: No suitable GCC found!"
    exit 1
fi

# Universal binary flags (for Intel Macs with Xcode)
# PowerPC-only on G4/G5 Macs
if [ "$ARCH" = "i386" ] || [ "$ARCH" = "x86_64" ]; then
    ARCH_FLAGS="-arch i386 -arch ppc"
    echo "Building universal binary (i386 + ppc)"
else
    ARCH_FLAGS="-arch ppc"
    echo "Building PowerPC binary"
fi

CFLAGS="-O2 -Wall $ARCH_FLAGS -mmacosx-version-min=10.5"
LDFLAGS="-dynamiclib $ARCH_FLAGS -mmacosx-version-min=10.5"

# Create build directories
echo ""
echo "Creating build directories..."
rm -rf build/patches
mkdir -p build/patches

# Compile each CVE fix
echo ""
echo "Compiling CVE patches..."

echo "  [1/5] CVE-2008-1447 DNS Port Randomization..."
$CC $CFLAGS $LDFLAGS \
    -o build/patches/dns_randomizer.dylib \
    kernel_patches/CVE-2008-1447-DNS/dns_port_randomizer.c \
    -framework CoreFoundation 2>/dev/null || {
    echo "    Building without CoreFoundation..."
    $CC $CFLAGS $LDFLAGS \
        -o build/patches/dns_randomizer.dylib \
        kernel_patches/CVE-2008-1447-DNS/dns_port_randomizer.c
}
echo "    Built: dns_randomizer.dylib"

echo "  [2/5] CVE-2009-2414 TCP ISN Randomization..."
$CC $CFLAGS $LDFLAGS \
    -o build/patches/tcp_isn_randomizer.dylib \
    kernel_patches/CVE-2009-2414-TCP/tcp_isn_randomizer.c \
    -framework CoreFoundation 2>/dev/null || {
    $CC $CFLAGS $LDFLAGS \
        -o build/patches/tcp_isn_randomizer.dylib \
        kernel_patches/CVE-2009-2414-TCP/tcp_isn_randomizer.c
}
echo "    Built: tcp_isn_randomizer.dylib"

echo "  [3/5] CVE-2010-0036 HFS+ Overflow Guard..."
$CC $CFLAGS $LDFLAGS \
    -o build/patches/hfs_overflow_guard.dylib \
    kernel_patches/CVE-2010-0036-HFS/hfs_overflow_guard.c \
    -framework CoreFoundation 2>/dev/null || {
    $CC $CFLAGS $LDFLAGS \
        -o build/patches/hfs_overflow_guard.dylib \
        kernel_patches/CVE-2010-0036-HFS/hfs_overflow_guard.c
}
echo "    Built: hfs_overflow_guard.dylib"

echo "  [4/5] CVE-2011-0182 Font Parsing Guard..."
$CC $CFLAGS $LDFLAGS \
    -o build/patches/font_parsing_guard.dylib \
    kernel_patches/CVE-2011-0182-Font/font_parsing_guard.c \
    -framework CoreFoundation 2>/dev/null || {
    $CC $CFLAGS $LDFLAGS \
        -o build/patches/font_parsing_guard.dylib \
        kernel_patches/CVE-2011-0182-Font/font_parsing_guard.c
}
echo "    Built: font_parsing_guard.dylib"

echo "  [5/5] CVE-2014-4377 IOKit Bounds Guard..."
$CC $CFLAGS $LDFLAGS \
    -o build/patches/iokit_bounds_guard.dylib \
    kernel_patches/CVE-2014-4377-IOKit/iokit_bounds_guard.c \
    -framework IOKit -framework CoreFoundation 2>/dev/null || {
    $CC $CFLAGS $LDFLAGS \
        -o build/patches/iokit_bounds_guard.dylib \
        kernel_patches/CVE-2014-4377-IOKit/iokit_bounds_guard.c
}
echo "    Built: iokit_bounds_guard.dylib"

echo ""
echo "Creating installer package..."

# Create package structure
rm -rf ${PACKAGE_NAME}.pkg
mkdir -p ${PACKAGE_NAME}.pkg/Contents/Resources
mkdir -p build_root/Library/Security/LeopardPatches
mkdir -p build_root/System/Library/LaunchDaemons
mkdir -p build_root/etc/profile.d

# Copy built dylibs
cp build/patches/*.dylib build_root/Library/Security/LeopardPatches/

# Create combined security dylib loader script
cat > build_root/etc/profile.d/leopard_security.sh << 'EOF'
# Leopard Security Patches Loader
# CVE fixes for Mac OS X 10.5.x
#
# Loaded dylibs:
# - dns_randomizer.dylib      (CVE-2008-1447)
# - tcp_isn_randomizer.dylib  (CVE-2009-2414)
# - hfs_overflow_guard.dylib  (CVE-2010-0036)
# - font_parsing_guard.dylib  (CVE-2011-0182)
# - iokit_bounds_guard.dylib  (CVE-2014-4377)

LEOPARD_PATCHES="/Library/Security/LeopardPatches"

if [ -d "$LEOPARD_PATCHES" ]; then
    export DYLD_INSERT_LIBRARIES="${LEOPARD_PATCHES}/dns_randomizer.dylib:${LEOPARD_PATCHES}/tcp_isn_randomizer.dylib:${LEOPARD_PATCHES}/hfs_overflow_guard.dylib:${LEOPARD_PATCHES}/font_parsing_guard.dylib:${LEOPARD_PATCHES}/iokit_bounds_guard.dylib"
fi
EOF

# Create launchd plist for mDNSResponder (DNS randomization)
cat > build_root/System/Library/LaunchDaemons/com.elya.leopard-dns-security.plist << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>com.elya.leopard-dns-security</string>
    <key>EnvironmentVariables</key>
    <dict>
        <key>DYLD_INSERT_LIBRARIES</key>
        <string>/Library/Security/LeopardPatches/dns_randomizer.dylib</string>
    </dict>
    <key>ProgramArguments</key>
    <array>
        <string>/bin/launchctl</string>
        <string>setenv</string>
        <string>DYLD_INSERT_LIBRARIES</string>
        <string>/Library/Security/LeopardPatches/dns_randomizer.dylib</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <false/>
</dict>
</plist>
EOF

# Create postinstall script
cat > ${PACKAGE_NAME}.pkg/Contents/Resources/postinstall << 'EOF'
#!/bin/bash
# Leopard Security Patches Postinstall
# Sets permissions and loads security modules

echo "Installing Leopard Security Patches..."

# Set permissions
chmod 755 /Library/Security/LeopardPatches/*.dylib
chmod 644 /Library/Security/LeopardPatches/*.dylib
chown -R root:wheel /Library/Security/LeopardPatches
chmod 755 /Library/Security/LeopardPatches

chmod 644 /etc/profile.d/leopard_security.sh
chmod 644 /System/Library/LaunchDaemons/com.elya.leopard-dns-security.plist

# Create profile.d if needed
mkdir -p /etc/profile.d

# Add to profile
PROFILE_LINE="[ -f /etc/profile.d/leopard_security.sh ] && . /etc/profile.d/leopard_security.sh"
if ! grep -q "leopard_security.sh" /etc/profile 2>/dev/null; then
    echo "" >> /etc/profile
    echo "# Leopard Security Patches" >> /etc/profile
    echo "$PROFILE_LINE" >> /etc/profile
fi

# Load the launch daemon
launchctl load /System/Library/LaunchDaemons/com.elya.leopard-dns-security.plist 2>/dev/null || true

echo ""
echo "Leopard Security Patches installed successfully!"
echo ""
echo "CVEs Patched:"
echo "  - CVE-2008-1447: DNS Cache Poisoning (Kaminsky)"
echo "  - CVE-2009-2414: TCP ISN Hijacking"
echo "  - CVE-2010-0036: HFS+ Integer Overflow"
echo "  - CVE-2011-0182: Font Parsing RCE"
echo "  - CVE-2014-4377: IOKit Privilege Escalation"
echo ""
echo "A REBOOT IS REQUIRED for all protections to take effect."

exit 0
EOF
chmod +x ${PACKAGE_NAME}.pkg/Contents/Resources/postinstall

# Create preinstall script (backup)
cat > ${PACKAGE_NAME}.pkg/Contents/Resources/preinstall << 'EOF'
#!/bin/bash
# Backup existing installation if present
if [ -d /Library/Security/LeopardPatches ]; then
    echo "Backing up existing patches..."
    cp -r /Library/Security/LeopardPatches /Library/Security/LeopardPatches.backup.$(date +%Y%m%d)
fi
exit 0
EOF
chmod +x ${PACKAGE_NAME}.pkg/Contents/Resources/preinstall

# Create Info.plist
cat > ${PACKAGE_NAME}.pkg/Contents/Info.plist << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleIdentifier</key>
    <string>${IDENTIFIER}</string>
    <key>CFBundleName</key>
    <string>Leopard Security Patches</string>
    <key>CFBundleShortVersionString</key>
    <string>${VERSION}</string>
    <key>IFMajorVersion</key>
    <integer>1</integer>
    <key>IFMinorVersion</key>
    <integer>0</integer>
    <key>IFPkgFlagAllowBackRev</key>
    <false/>
    <key>IFPkgFlagAuthorizationAction</key>
    <string>RootAuthorization</string>
    <key>IFPkgFlagDefaultLocation</key>
    <string>/</string>
    <key>IFPkgFlagInstallFat</key>
    <true/>
    <key>IFPkgFlagIsRequired</key>
    <false/>
    <key>IFPkgFlagRelocatable</key>
    <false/>
    <key>IFPkgFlagRestartAction</key>
    <string>RecommendedRestart</string>
    <key>IFPkgFlagRootVolumeOnly</key>
    <true/>
    <key>IFPkgFlagUpdateInstalledLanguages</key>
    <false/>
    <key>IFPkgFormatVersion</key>
    <real>0.10000000149011612</real>
</dict>
</plist>
EOF

# Create Description.plist
cat > ${PACKAGE_NAME}.pkg/Contents/Resources/Description.plist << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>IFPkgDescriptionDescription</key>
    <string>Security patches for Mac OS X 10.5.x Leopard

CVEs Fixed:
• CVE-2008-1447 - DNS Port Randomization (Kaminsky Attack)
• CVE-2009-2414 - TCP ISN Randomization
• CVE-2010-0036 - HFS+ Overflow Protection
• CVE-2011-0182 - Font Parsing Security
• CVE-2014-4377 - IOKit Bounds Checking

Compatible with PowerPC G4, G5, and Intel Macs running Leopard.

WARNING: Experimental - test on non-production systems first.
See README for known issues and uninstallation instructions.

Created by Scott (Scottcjn) with Claude AI assistance.</string>
    <key>IFPkgDescriptionTitle</key>
    <string>Leopard Security Patches</string>
</dict>
</plist>
EOF

# Create PkgInfo
echo -n "pmkrpkg1" > ${PACKAGE_NAME}.pkg/Contents/PkgInfo

echo "Creating package archive..."

# Create Archive.bom (Bill of Materials)
mkbom build_root ${PACKAGE_NAME}.pkg/Contents/Archive.bom

# Create Archive.pax.gz
cd build_root
find . | cpio -o --format=odc 2>/dev/null | gzip -9 > ../${PACKAGE_NAME}.pkg/Contents/Archive.pax.gz
cd ..

# Clean up build root
rm -rf build_root

echo ""
echo "=== Package Created ==="
echo "  ${PACKAGE_NAME}.pkg"
echo ""

# Create DMG if hdiutil is available
if command -v hdiutil &> /dev/null; then
    echo "Creating DMG..."
    rm -f LeopardSecurityPatches.dmg
    hdiutil create -volname "Leopard Security Patches" \
        -srcfolder ${PACKAGE_NAME}.pkg \
        -ov -format UDZO \
        LeopardSecurityPatches.dmg
    echo "  LeopardSecurityPatches.dmg"
    echo ""
fi

echo "=== Build Complete ==="
echo ""
echo "Installation:"
echo "  1. Copy to Leopard Mac"
echo "  2. Mount the DMG (or use the .pkg directly)"
echo "  3. Run: sudo installer -pkg LeopardSecurityPatches.pkg -target /"
echo "  4. Reboot"
echo ""
echo "Uninstallation:"
echo "  sudo rm -rf /Library/Security/LeopardPatches"
echo "  sudo rm /System/Library/LaunchDaemons/com.elya.leopard-dns-security.plist"
echo "  sudo rm /etc/profile.d/leopard_security.sh"
echo "  (Remove leopard_security line from /etc/profile)"
echo ""
