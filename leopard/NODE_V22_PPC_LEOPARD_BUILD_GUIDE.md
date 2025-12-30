# Building Node.js v22 on PowerPC Mac OS X Leopard (10.5.8)

This guide documents all the patches and fixes needed to build Node.js v22 on PowerPC Mac OS X Leopard (10.5.8) with GCC 10.

## Prerequisites

- Mac OS X Leopard 10.5.8 (PPC)
- GCC 10 installed at `/usr/local/gcc-10/`
- Python 3.7+
- Xcode Command Line Tools

## Configuration

```bash
cd ~/node-ppc
./configure --dest-cpu=ppc --dest-os=mac --without-intl
```

## Required Patches

### 1. V8 ThreadLocalTop Struct Size (32-bit PPC)

**File:** `deps/v8/src/execution/thread-local-top.h`

V8 expects 64-bit pointers. Add padding for 32-bit:

```cpp
// After line 40, in ThreadLocalTop struct
#if V8_HOST_ARCH_32_BIT
  // Padding for 32-bit architectures to match 64-bit size expectations
  void* padding_[8];
#endif
```

### 2. V8 IsolateData Field Offsets (32-bit PPC)

**File:** `deps/v8/src/execution/isolate-data.h`

Static assertion fixes for 32-bit pointer sizes in IsolateData fields.

### 3. V8 Turboshaft Operations Sizes

**File:** `deps/v8/src/compiler/turboshaft/operations.h`

Size assertions for 32-bit compatibility.

### 4. OpenSSL linux-ppc/no-asm Configuration

**Directory:** `deps/openssl/`

Configure OpenSSL for big-endian PPC without assembly:
- Use `linux-ppc` target
- Add `no-asm` flag

### 5. libuv Leopard Symbol Fixes

**File:** `deps/uv/src/unix/darwin.c`

Functions not available on Leopard:
- `pthread_setname_np` - stub implementation
- `mach_continuous_time` - use `mach_absolute_time` fallback

**File:** `deps/uv/src/unix/udp.c`

- `recvmsg_x` - not available on Leopard, disable multi-message receive

### 6. clock_gettime Compatibility (Leopard)

**File:** `deps/v8/third_party/abseil-cpp/absl/synchronization/internal/kernel_timeout.cc`

Leopard doesn't have `clock_gettime` (macOS 10.12+). Add compatibility shim:

```cpp
#include <sys/time.h>

#if defined(__APPLE__)
#include <AvailabilityMacros.h>
#include <mach/mach_time.h>

#if !defined(MAC_OS_X_VERSION_10_12) || MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_12

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

typedef int clockid_t;

static inline int clock_gettime_compat(clockid_t clk_id, struct timespec *tp) {
    if (clk_id == CLOCK_REALTIME) {
        struct timeval tv;
        if (gettimeofday(&tv, NULL) != 0) return -1;
        tp->tv_sec = tv.tv_sec;
        tp->tv_nsec = tv.tv_usec * 1000;
        return 0;
    } else if (clk_id == CLOCK_MONOTONIC) {
        static mach_timebase_info_data_t timebase = {0, 0};
        if (timebase.denom == 0) mach_timebase_info(&timebase);
        uint64_t mach_time = mach_absolute_time();
        uint64_t nanos = mach_time * timebase.numer / timebase.denom;
        tp->tv_sec = nanos / 1000000000ULL;
        tp->tv_nsec = nanos % 1000000000ULL;
        return 0;
    }
    return -1;
}
#define clock_gettime clock_gettime_compat

#endif
#endif
```

### 7. os/signpost.h (macOS 10.14+ API)

**File:** `deps/v8/src/libplatform/tracing/recorder.h`

Signpost API is macOS 10.14+ only. Add stubs:

```cpp
#if defined(__APPLE__)
#include <AvailabilityMacros.h>

#if defined(__MAC_10_14) && MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_10_14
#include <os/signpost.h>
#define V8_HAS_SIGNPOST 1
#else
// Stub types for pre-10.14 macOS
typedef void* os_log_t;
#define os_log_create(subsystem, category) ((os_log_t)NULL)
#define os_signpost_event_emit(log, id, name, ...) ((void)0)
#define os_signpost_interval_begin(log, id, name, ...) ((void)0)
#define os_signpost_interval_end(log, id, name, ...) ((void)0)
#define os_signpost_id_generate(log) (0)
#define OS_LOG_DEFAULT ((os_log_t)NULL)
typedef uint64_t os_signpost_id_t;
#define V8_HAS_SIGNPOST 0
#endif
#endif
```

**File:** `deps/v8/src/libplatform/tracing/recorder-mac.cc`

Wrap signpost calls with `#if V8_HAS_SIGNPOST` guards.

### 8. C++20 Features Compatibility (GCC 10 with C++17)

#### 8.1 std::endian and std::u8string_view

**File:** `src/util.h`

Add fallbacks after includes:

```cpp
// C++20 fallbacks for older compilers
#if __cplusplus < 202002L || !defined(__cpp_lib_endian)
namespace std {
enum class endian {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    little = __ORDER_LITTLE_ENDIAN__,
    big    = __ORDER_BIG_ENDIAN__,
    native = __ORDER_BIG_ENDIAN__
#else
    little = 0,
    big    = 1,
    native = 1  // PPC is big-endian
#endif
};
}
#endif

#if __cplusplus < 202002L || !defined(__cpp_char8_t)
namespace std {
using u8string_view = std::basic_string_view<char>;
}
#endif
```

#### 8.2 Spaceship Operator (<=>)

**Files:** `deps/ncrypto/ncrypto.h`, `deps/ncrypto/ncrypto.cc`

Replace C++20 spaceship operator with compare method and traditional operators:

```cpp
// In ncrypto.h - replace operator<=> declarations:
int compare(const BignumPointer& other) const noexcept;
int compare(const BIGNUM* other) const noexcept;

// Add comparison operators:
bool operator<(const BignumPointer& other) const noexcept { return compare(other) < 0; }
bool operator<(const BIGNUM* other) const noexcept { return compare(other) < 0; }
bool operator>(const BignumPointer& other) const noexcept { return compare(other) > 0; }
bool operator>(const BIGNUM* other) const noexcept { return compare(other) > 0; }
bool operator<=(const BignumPointer& other) const noexcept { return compare(other) <= 0; }
bool operator<=(const BIGNUM* other) const noexcept { return compare(other) <= 0; }
bool operator>=(const BignumPointer& other) const noexcept { return compare(other) >= 0; }
bool operator>=(const BIGNUM* other) const noexcept { return compare(other) >= 0; }
bool operator==(const BignumPointer& other) const noexcept { return compare(other) == 0; }
bool operator==(const BIGNUM* other) const noexcept { return compare(other) == 0; }
bool operator!=(const BignumPointer& other) const noexcept { return compare(other) != 0; }
bool operator!=(const BIGNUM* other) const noexcept { return compare(other) != 0; }
```

#### 8.3 ends_with/starts_with Methods

**File:** `src/cxx20_compat.h` (NEW FILE)

Create compatibility header:

```cpp
#ifndef SRC_CXX20_COMPAT_H_
#define SRC_CXX20_COMPAT_H_

#include <string>
#include <string_view>
#include <algorithm>

namespace cxx20compat {

inline bool ends_with(const std::string& str, const std::string& suffix) {
  if (suffix.size() > str.size()) return false;
  return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}
inline bool ends_with(const std::string& str, char c) {
  return !str.empty() && str.back() == c;
}
inline bool starts_with(const std::string& str, const std::string& prefix) {
  if (prefix.size() > str.size()) return false;
  return std::equal(prefix.begin(), prefix.end(), str.begin());
}
inline bool starts_with(std::string_view str, std::string_view prefix) {
  if (prefix.size() > str.size()) return false;
  return str.substr(0, prefix.size()) == prefix;
}
inline bool ends_with(std::string_view str, std::string_view suffix) {
  if (suffix.size() > str.size()) return false;
  return str.substr(str.size() - suffix.size()) == suffix;
}

}  // namespace cxx20compat

#endif
```

**Files to modify:** (add `#include "cxx20_compat.h"` and replace calls)
- `src/node_credentials.cc` - replace `dir.ends_with("/")` with `dir.back() == '/'`
- `src/node_dotenv.cc` - use `cxx20compat::starts_with()`
- `src/node_file.cc` - use `cxx20compat::ends_with/starts_with()`
- `src/node_modules.cc` - use `cxx20compat::ends_with()`
- `src/node_task_runner.cc` - use `cxx20compat::ends_with()`
- `src/node_util.cc` - use `cxx20compat::starts_with()`

#### 8.4 consteval and [[unlikely]]

**File:** `src/node_url.cc`

Remove C++20 `consteval` keyword and `[[unlikely]]` attribute:

```cpp
// Change:
constexpr auto lookup_table = []() consteval {
// To:
constexpr auto lookup_table = []() {

// Remove [[unlikely]] attributes
```

### 9. AI_NUMERICSERV (Leopard DNS)

**File:** `src/inspector_socket_server.cc`

`AI_NUMERICSERV` not available until macOS 10.6. Add after includes:

```cpp
#ifndef AI_NUMERICSERV
#define AI_NUMERICSERV 0
#endif
```

### 10. X509Pointer Comparison Fix

**File:** `src/crypto/crypto_context.cc`

Change `x != nullptr` to `static_cast<bool>(x)` for X509Pointer comparisons.

## Build Command

```bash
cd ~/node-ppc/out
make -j1 V=0
```

Note: Use `-j1` (single thread) to avoid memory exhaustion on older hardware.

## Target

The goal is to build Node.js to run Claude Code on PowerPC Mac OS X Leopard.

### 11. QUIC ngtcp2_ssize Type Mismatch

**Files:** `src/quic/application.cc`, `src/quic/application.h`

On 32-bit systems, `ssize_t` and `ngtcp2_ssize` (which is `ptrdiff_t`) are incompatible.

```cpp
// In application.cc - Line 274:
// Change: ssize_t ndatalen = 0;
// To:
ngtcp2_ssize ndatalen = 0;

// In application.h - Line 132 (WriteVStream signature):
// Change: ssize_t* ndatalen,
// To:
ngtcp2_ssize* ndatalen,

// In application.cc - Line 403 (WriteVStream definition):
// Change: ssize_t* ndatalen,
// To:
ngtcp2_ssize* ndatalen,
```

### 12. QUIC AI_NUMERICSERV (preferredaddress.cc)

**File:** `src/quic/preferredaddress.cc`

Add after includes (similar to inspector_socket_server.cc):

```cpp
// Leopard compatibility: AI_NUMERICSERV not available until macOS 10.6
#ifndef AI_NUMERICSERV
#define AI_NUMERICSERV 0
#endif
```

### 13. V8 Baseline Assembler 32-bit PPC Support

V8's baseline compiler (Sparkplug) only has support for PPC64, not 32-bit PPC.

**File:** `deps/v8/src/baseline/baseline-assembler-inl.h`

```cpp
// Change line 27:
#elif V8_TARGET_ARCH_PPC64
// To:
#elif V8_TARGET_ARCH_PPC64 || V8_TARGET_ARCH_PPC
```

**File:** `deps/v8/src/baseline/baseline-compiler.cc`

```cpp
// Change line 41:
#elif V8_TARGET_ARCH_PPC64
// To:
#elif V8_TARGET_ARCH_PPC64 || V8_TARGET_ARCH_PPC
```

### 14. V8 macro-assembler-ppc SMI Assertions (32-bit)

**File:** `deps/v8/src/codegen/ppc/macro-assembler-ppc.h`

The SMI (Small Integer) layout differs between 32-bit and 64-bit PPC. Add 32-bit PPC checks:

```cpp
// Line 1105: SmiToPtrArrayOffset function
// Change:
#if defined(V8_COMPRESS_POINTERS) || defined(V8_31BIT_SMIS_ON_64BIT_ARCH)
// To:
#if defined(V8_COMPRESS_POINTERS) || defined(V8_31BIT_SMIS_ON_64BIT_ARCH) || (defined(V8_TARGET_ARCH_PPC) && !defined(V8_TARGET_ARCH_PPC64))

// Line 1848: SMI assertion block
// Change:
#if !defined(V8_COMPRESS_POINTERS) && !defined(V8_31BIT_SMIS_ON_64BIT_ARCH)
// To:
#if !defined(V8_COMPRESS_POINTERS) && !defined(V8_31BIT_SMIS_ON_64BIT_ARCH) && !(defined(V8_TARGET_ARCH_PPC) && !defined(V8_TARGET_ARCH_PPC64))
```

## Build Command

```bash
cd ~/node-ppc/out
make -j1 V=0
```

Note: Use `-j1` (single thread) to avoid memory exhaustion on older hardware.

## Target

The goal is to build Node.js to run Claude Code on PowerPC Mac OS X Leopard.

## Status

**Last Updated:** December 30, 2025

Build in progress on G5 Mac (192.168.0.179). V8 compilation proceeding with all 32-bit PPC patches applied.

**Progress:**
- V8 baseline compiler: ✅ Compiling successfully
- V8 builtins: ✅ ~42 object files compiled
- Currently compiling: `builtins-typed-array.cc`
- Errors: 0

**Estimated Time:** V8 alone has ~1170 source files. On G5 hardware with `-j1`, expect 6-12 hours for full build.

**All 14 patches applied successfully:**
1. ThreadLocalTop struct padding
2. IsolateData field offsets
3. Turboshaft operations sizes
4. OpenSSL linux-ppc/no-asm
5. libuv Leopard symbols
6. clock_gettime compatibility
7. os/signpost.h stubs
8. C++20 fallbacks (spaceship, ends_with, consteval)
9. AI_NUMERICSERV definition (inspector + QUIC)
10. X509Pointer comparison
11. ngtcp2_ssize type mismatch
12. V8 baseline assembler PPC32 include
13. V8 macro-assembler SMI assertions
14. QUIC preferredaddress AI_NUMERICSERV

## Monitoring Commands

```bash
# SSH to G5 Mac (needs old key algorithms)
sshpass -p 'Elyanlabs12@' ssh -o StrictHostKeyChecking=no -o HostKeyAlgorithms=+ssh-rsa -o PubkeyAcceptedKeyTypes=+ssh-rsa selenamac@192.168.0.179

# Watch build log
tail -f /tmp/node_build.log

# Check V8 object files compiled
find ~/node-ppc/out/Release/obj.host/v8_base_without_compiler -name "*.o" | wc -l

# Check for errors
grep -c "error:" /tmp/node_build.log

# Current file being compiled
ps aux | grep "[g]++" | grep -o "[^ ]*\.cc"

# Restart build if needed (do NOT run if already building)
cd ~/node-ppc/out && nohup make -j1 V=0 > /tmp/node_build.log 2>&1 &
```
