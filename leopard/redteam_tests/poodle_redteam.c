/*
 * CVE-2014-3566 POODLE Red Team Test (Leopard 10.5)
 *
 * Drives the SSLv3-disable / TLS_FALLBACK_SCSV guard with the forced
 * downgrade attack. Directly relevant to Leopard: its Secure Transport
 * and system OpenSSL 0.9.8 both speak SSL 3.0, so a MITM downgrade is a
 * real threat here.
 */
#include <stdio.h>
#include <stdint.h>

#include "../kernel_patches/CVE-2014-3566-POODLE/disable_sslv3.c"

static int passed, failed;
static void expect(const char *what, int got, int want) {
    if (got == want) { printf("  PASS: %s\n", what); passed++; }
    else { printf("  FAIL: %s (got %d want %d)\n", what, got, want); failed++; }
}

int main(void) {
    int alert;
    printf("=== CVE-2014-3566 POODLE Red Team Test (Leopard) ===\n\n");

    printf("Attack: MITM forces SSL 3.0 downgrade (client offers SSLv3)\n");
    alert = ALERT_NONE;
    expect("SSL 3.0 refused", (int)negotiate_protocol(SSL3_VERSION, TLS1_2_VERSION, 0, &alert), 0);
    expect("alert = protocol_version", alert, ALERT_PROTOCOL_VERSION);

    printf("Attack: TLS_FALLBACK_SCSV downgrade (client retries low w/ SCSV)\n");
    alert = ALERT_NONE;
    expect("downgrade refused", (int)negotiate_protocol(TLS1_0_VERSION, TLS1_2_VERSION, 1, &alert), 0);
    expect("alert = inappropriate_fallback", alert, ALERT_INAPPROPRIATE_FALLBACK);

    printf("Legit: normal TLS 1.0 client, no fallback signal\n");
    alert = ALERT_NONE;
    expect("negotiates TLS 1.0", (int)negotiate_protocol(TLS1_0_VERSION, TLS1_2_VERSION, 0, &alert), TLS1_0_VERSION);
    expect("no alert", alert, ALERT_NONE);

    printf("Legit: top-version client that also sends fallback SCSV\n");
    alert = ALERT_NONE;
    expect("top-version fallback allowed", (int)negotiate_protocol(TLS1_2_VERSION, TLS1_2_VERSION, 1, &alert), TLS1_2_VERSION);

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
