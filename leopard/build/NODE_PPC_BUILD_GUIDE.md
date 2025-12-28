# Building Node.js v22 on PowerPC Mac OS X Leopard

## Overview

This guide documents building Node.js v22.x on PowerPC Mac OS X 10.5 (Leopard) using GCC 10. The main challenge is the `__text_cold` linker error caused by GCC's hot/cold code partitioning.

## Prerequisites

- Mac OS X 10.5.8 (Leopard) on PowerPC G4/G5
- GCC 10.5.0 installed at `/usr/local/gcc-10/`
- Python 2.7 or 3.x for gyp build system
- At least 4GB RAM recommended

## The `__text_cold` Problem

### Symptoms

```
ld: bl out of range (-33220552 max is +/-16M) from __ZN2v84base2OS5AbortEv
at 0x01FB0FA8 in __text_cold of libv8_libbase.a(platform-posix.o)
```

### Root Cause

GCC 10's `-freorder-blocks-and-partition` optimization (enabled at `-O2`/`-O3`) splits "cold" (rarely executed) code into a separate `__text_cold` section. On Darwin/Mach-O:

1. The linker places `__text_cold` after all `__text` code
2. This can be 33MB+ away from the main code
3. PowerPC's `bl` (branch and link) instruction only reaches ±16MB
4. Branch islands work **within** sections but can't span **across** sections

### Research Sources

- [Apple ld64 branch_island.cpp](https://opensource.apple.com/source/ld64/ld64-242.2/src/ld/passes/branch_island.cpp.auto.html)
- [MaskRay Linker Notes on Power ISA](https://maskray.me/blog/2023-02-26-linker-notes-on-power-isa)
- [GCC Optimize Options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- [michaelweiser/ld64 PPC forward-port](https://github.com/michaelweiser/ld64)

## Solution: The `-O0` Fix

The `-fno-reorder-blocks-and-partition` flag does **not** fully prevent `__text_cold` on Darwin because:
- Functions marked with `__attribute__((cold))` still get sectioned
- Some code paths ignore the flag on Mach-O

The only reliable fix is to compile affected files with `-O0`:

```bash
# Identify files with __text_cold sections
find out/Release -name "*.o" -exec sh -c \
  'otool -l "$1" 2>/dev/null | grep -q __text_cold && echo "$1"' _ {} \;

# Recompile with -O0 (replace -O3 in original command)
g++ -O0 ... source.cpp -o output.o
```

## Build Configuration

### Required Compiler Flags

Add to `common.gypi` or set via environment:

```python
'cflags': [
  '-mlongcall',                      # Use indirect calls for all branches
  '-fno-reorder-blocks-and-partition', # Reduce cold section creation
  '-fno-reorder-functions',          # Keep function order stable
],
'cflags_cc': [
  '-Wno-extra',                      # Suppress GCC 10 strictness warnings
],
```

### Environment Variables

```bash
export CC=/usr/local/gcc-10/bin/gcc
export CXX=/usr/local/gcc-10/bin/g++
export CFLAGS="-mlongcall -fno-reorder-blocks-and-partition -Wno-extra"
export CXXFLAGS="-mlongcall -fno-reorder-blocks-and-partition -Wno-extra"
```

## Build Steps

### 1. Configure

```bash
cd ~/node-ppc
./configure --dest-cpu=ppc --dest-os=mac \
  --without-snapshot --without-intl
```

### 2. Build with PPC Fixes

```bash
export CC=/usr/local/gcc-10/bin/gcc
export CXX=/usr/local/gcc-10/bin/g++
make -C out BUILDTYPE=Release -j2 2>&1 | tee build.log
```

### 3. Fix `__text_cold` Errors

When you hit a link error, identify and fix the affected file:

```bash
# Example: platform-posix.o has __text_cold
otool -l out/Release/obj.target/v8_libbase/deps/v8/src/base/platform/platform-posix.o | grep __text_cold

# Recompile with -O0
/usr/local/gcc-10/bin/g++ -mlongcall -fno-reorder-blocks-and-partition \
  -O0 -c deps/v8/src/base/platform/platform-posix.cc \
  -o out/Release/obj.target/v8_libbase/deps/v8/src/base/platform/platform-posix.o \
  [... other flags from build log ...]

# Rebuild the library
cd out/Release
rm -f libv8_libbase.a
ar -cr libv8_libbase.a obj.target/v8_libbase/**/*.o
ranlib libv8_libbase.a

# Continue build
make -C out BUILDTYPE=Release -j2
```

### 4. Comprehensive Fix Script

For bulk fixing all affected files:

```bash
#!/bin/bash
# Save as fix_cold_sections.sh

cd ~/node-ppc

# Find all affected object files
find out/Release -name "*.o" -exec sh -c \
  'otool -l "$1" 2>/dev/null | grep -q __text_cold && echo "$1"' _ {} \; \
  > /tmp/cold_files.txt

echo "Found $(wc -l < /tmp/cold_files.txt) files with __text_cold"

# For each file, extract compilation command from build log and recompile with -O0
while read OBJ; do
  BASENAME=$(basename "$OBJ")
  CMD=$(grep -F "$BASENAME" build.log | grep -v "^make" | head -1)

  if [ -n "$CMD" ]; then
    NEW_CMD=$(echo "$CMD" | sed 's/-O[23]/-O0/g')
    echo "Fixing: $BASENAME"
    eval "$NEW_CMD"
  fi
done < /tmp/cold_files.txt
```

## Known Issues

### 1. GCC 10 Warning Strictness

GCC 10 treats more warnings as errors. Fix with:
```
-Wno-extra -Wno-class-memaccess
```

### 2. Missing `strnlen` on Leopard

Some code uses `strnlen` which isn't in Leopard's libc. Add:
```c
#ifndef strnlen
size_t strnlen(const char *s, size_t maxlen) {
    size_t len;
    for (len = 0; len < maxlen && s[len]; len++);
    return len;
}
#endif
```

### 3. Architecture Detection

GCC 10 ignores `-arch i386` flags (it's PPC only). These warnings are harmless:
```
gcc: warning: this compiler does not support X86 (arch flags ignored)
```

## Modern Mac Comparison

Modern macOS (arm64/x86_64) handles this differently:

| Approach | Modern macOS | Leopard PPC Fix |
|----------|--------------|-----------------|
| Branch Range | ARM64: ±128MB | PPC: ±16MB |
| Branch Islands | Linker inserts automatically | Only within sections |
| Cold Sections | Linker reorders with `-order_file` | Doesn't work across sections |
| Solution | LLD with range thunks | Compile with `-O0` |

## References

- [Apple ld64 Source](https://github.com/apple-oss-distributions/ld64)
- [LLVM LLD Range Extension Thunks](https://reviews.llvm.org/D39744)
- [PowerPC ISA Branch Limits](https://fenixfox-studios.com/manual/powerpc/instructions/bl.html)
- [GCC Hot/Cold Partitioning](https://gcc.gnu.org/legacy-ml/gcc-patches/2003-10/msg00652.html)

## V8 32-bit PowerPC Limitations (December 2025 Discovery)

### Critical Finding: V8 Does NOT Support 32-bit PowerPC

After extensive testing with Node.js v22 (V8 11.8), we discovered that **32-bit PowerPC was never officially supported by V8/Node.js**:

- [V8-users group confirms PPC32 not supported](https://groups.google.com/g/v8-users/c/f9GViw45eSw)
- [Node.js issue #30323 - PPC32 port never completed](https://github.com/nodejs/node/issues/30323)
- IBM's PowerPC port only targets **64-bit** (ppc64/ppc64le on Linux)

### Static Assertion Failures

V8 has numerous `static_assert` checks for structure layouts that fail on 32-bit PPC:

```
error: static assertion failed
  ThreadLocalTop::kSizeInBytes == sizeof(ThreadLocalTop)

error: static assertion failed
  offsetof(IsolateData, Name##_) == Offset
```

**Root Cause**: V8's structure layouts are carefully calculated for officially supported platforms (x86, ARM, PPC64LE). The 32-bit PowerPC Darwin case has:
- Wrong pointer sizes (4 bytes vs expected 8)
- Wrong field alignments
- Missing platform-specific code paths

### What We Tried

1. **Fixed `__text_cold` sections** - Success with Perl binary patching
2. **Fixed ThreadLocalTop::kSizeInBytes** - Changed from 30 to 31 pointers
3. **Disabled static assertions** - Reveals cascading layout issues in Turboshaft

### G5 64-bit Possibility

The G5 CPU supports 64-bit (`hw.cpu64bit_capable = 1`), but:
- Leopard runs in 32-bit kernel mode
- Our GCC 10 targets 32-bit (`powerpc-apple-darwin9.8.0`)
- Would need 64-bit GCC cross-compiler and compatible libraries

### Alternatives

1. **Use older JavaScript engine** - SpiderMonkey or older V8 (pre-2015)
2. **Try Deno with different JS engine** - QuickJS has better portability
3. **Run 64-bit Linux** - Yellow Dog Linux or Debian ppc64 on G5
4. **Accept limitation** - Node.js cannot run on 32-bit PowerPC Mac

### Binary Patching Script (What Works)

For `__text_cold` section renaming (works on any Mach-O):

```bash
#!/bin/bash
# patch_cold.sh - Rename __text_cold to __text
for FILE in "$@"; do
    if [ -f "$FILE" ]; then
        perl -pi -e 's/__text_cold/__text\x00\x00\x00\x00\x00/g' "$FILE"
        echo "Patched: $FILE"
    fi
done
```

## Credits

Research and implementation by Claude (Anthropic) for the Leopard Security Patches project, December 2025.
