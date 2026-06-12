# Snow Leopard Security Patches (Mac OS X 10.6.x)

Security patches for Mac OS X 10.6 Snow Leopard that Apple no longer supports.
**Snow Leopard is Intel-only** (10.5 Leopard was the last PowerPC release), so
everything here builds `i386 + x86_64`, deployment target 10.6.

## WARNING - EXPERIMENTAL

**These patches are EXPERIMENTAL and have NOT been tested on all hardware configurations.**

- Use at your own risk
- Make a FULL BACKUP before installing
- Test on non-production machines first

## Supported Configurations

| Hardware | Status | Notes |
|----------|--------|-------|
| Intel iMac / MacBook / MacBook Pro | Planned | Primary target |
| Mac mini (Intel) | Planned | Should work |
| Mac Pro (Intel) | Planned | Should work |
| PowerPC | N/A | 10.6 is Intel-only |

## CVEs Addressed

| CVE | Description | Severity | Method |
|-----|-------------|----------|--------|
| CVE-2008-1447 | DNS cache poisoning (Kaminsky) | High | Port randomization via DYLD interpose |
| CVE-2009-2414 | TCP ISN hijacking | Medium | ISN entropy enhancement |
| CVE-2010-0036 | HFS+ integer overflow | High | Userspace bounds checking |
| CVE-2011-0182 | Font parsing RCE | Critical | Font header validation |
| CVE-2014-4377 | IOKit privilege escalation | High | ioctl bounds checking |
| CVE-2014-0160 | Heartbleed (OpenSSL heartbeat over-read) | Critical | Heartbeat payload bounds¹ |
| CVE-2014-3566 | POODLE (SSL 3.0 padding oracle) | High | Disable SSLv3 + fallback SCSV |
| CVE-2016-0777 | OpenSSH client roaming info leak | Medium | Disable roaming + resume bounds² |
| CVE-2014-0224 | CCS injection (early ChangeCipherSpec MITM) | High | Handshake state gate |
| CVE-2016-0800 | DROWN (SSLv2 cross-protocol RSA oracle) | High | Reject SSLv2 at front door |
| CVE-2022-37434 | zlib inflate() gzip EXTRA over-read | Critical | Bounds-check EXTRA copy |

**Applicability on Snow Leopard (10.6) — stated honestly:**
- **POODLE** is directly relevant: 10.6's Secure Transport and system OpenSSL 0.9.8 both speak SSL 3.0, so a forced downgrade is a real threat here.
- ¹ **Heartbleed** does *not* affect 10.6's system OpenSSL 0.9.8 (the heartbeat extension arrived in 1.0.1). This guard protects **ported modern OpenSSL 1.0.1** builds on Snow Leopard.
- ² **OpenSSH roaming** does *not* affect 10.6's system OpenSSH 5.2 (client roaming arrived in 5.4). This guard protects **ported modern OpenSSH** builds.

## How It Works

The five kernel-surface guards use **DYLD_INSERT_LIBRARIES** to inject security
checks into running processes — the same Darwin userspace-interpose mechanism as
the Tiger and Leopard editions (the logic is OS-version-agnostic; only the build
arch differs: Intel-only here). The three TLS/SSH guards are link-libraries (pure
protocol bounds, identical to the Tiger/Leopard implementations), not interposers.

## Build

```bash
cd snowleopard
./BUILD_SNOWLEOPARD_PKG.sh   # i386 + x86_64, deployment target 10.6
```

## Tests

Red-team (adversarial) tests live in `redteam_tests/`. The protocol-logic guards
compile and pass on any host:

```bash
cd redteam_tests
for t in heartbleed poodle openssh_roaming; do
    cc -Wall -Wextra -o /tmp/sl_$t ${t}_redteam.c && /tmp/sl_$t
done
```

## Lineage

Snow Leopard (10.6) is the third OS tier in this suite, after Tiger (10.4) and
Leopard (10.5). It carries the same 11 CVEs as the Leopard edition. The kernel
guards reuse the Darwin-generic interpose code; the TLS/SSH guards reuse the
shared protocol-bounds logic; only the build is Intel-only.
