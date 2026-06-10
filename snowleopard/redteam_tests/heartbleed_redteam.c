/*
 * CVE-2014-0160 Heartbleed Red Team Test (Snow Leopard 10.6)
 *
 * Drives the heartbeat bounds guard with the actual over-read attack and
 * the boundary cases. Applies to a *ported* modern OpenSSL 1.0.1 on
 * Snow Leopard 10.6 (the 0.9.8 system OpenSSL has no heartbeat extension and is not
 * vulnerable; this guard protects builds that add 1.0.1).
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../kernel_patches/CVE-2014-0160-Heartbleed/heartbeat_bounds.c"

static int passed, failed;
static void expect(const char *what, int got, int want) {
    if (got == want) { printf("  PASS: %s\n", what); passed++; }
    else { printf("  FAIL: %s (got %d want %d)\n", what, got, want); failed++; }
}

static size_t build(uint8_t *b, uint8_t type, uint16_t declared, size_t body) {
    b[0] = type; b[1] = (uint8_t)(declared >> 8); b[2] = (uint8_t)(declared & 0xFF);
    memset(b + 3, 'A', body); return 3 + body;
}

int main(void) {
    uint8_t buf[256];
    size_t n;
    printf("=== CVE-2014-0160 Heartbleed Red Team Test (Snow Leopard 10.6) ===\n\n");

    printf("Attack 1: heartbeat claims 0xFFFF payload, sends 1 byte (the leak)\n");
    n = build(buf, TLS1_HB_REQUEST, 0xFFFF, 1);
    expect("over-read request rejected", heartbeat_request_is_safe(buf, n), 0);
    expect("no bytes echoed back", (int)heartbeat_safe_response_payload(buf, n), -1);

    printf("Attack 2: zero-length probe\n");
    n = build(buf, TLS1_HB_REQUEST, 0, 0);
    expect("zero-length probe rejected", heartbeat_request_is_safe(buf, n), 0);

    printf("Legit: well-formed 32-byte heartbeat\n");
    n = build(buf, TLS1_HB_REQUEST, 32, 32 + 16);
    expect("valid request accepted", heartbeat_request_is_safe(buf, n), 1);
    expect("echoes exactly 32 bytes", (int)heartbeat_safe_response_payload(buf, n), 32);

    printf("Boundary: one byte short of mandatory 16-byte padding\n");
    n = build(buf, TLS1_HB_REQUEST, 10, 10 + 15);
    expect("short-padding request rejected", heartbeat_request_is_safe(buf, n), 0);

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
