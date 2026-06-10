/*
 * CVE-2016-0777 OpenSSH Roaming Red Team Test (Snow Leopard 10.6)
 *
 * Drives the roaming-disable / resume-bounds guard with the server-driven
 * over-read. Applies to a *ported* modern OpenSSH (>=5.4) on Snow Leopard 10.6; the
 * 4.x system ssh predates client roaming and is not vulnerable. The guard
 * is included so Snow Leopard 10.6 builds of current OpenSSH are safe by default.
 */
#include <stdio.h>
#include <stddef.h>

#include "../kernel_patches/CVE-2016-0777-OpenSSH-Roaming/disable_roaming.c"

static int passed, failed;
static void expect(const char *what, long got, long want) {
    if (got == want) { printf("  PASS: %s\n", what); passed++; }
    else { printf("  FAIL: %s (got %ld want %ld)\n", what, got, want); failed++; }
}

int main(void) {
    printf("=== CVE-2016-0777 OpenSSH Roaming Red Team Test (Snow Leopard 10.6) ===\n\n");

    printf("Primary fix: client roaming disabled\n");
    expect("roaming off", client_roaming_is_enabled(), 0);
    expect("no resume honored even if in-bounds", roaming_bytes_to_send(1024, 4096, 0, 512), -1);

    printf("Attack: malicious server resumes past buffered bytes (the leak)\n");
    expect("over-read rejected (req past buffered)", roaming_resume_is_safe(1024, 4096, 0, 4096), 0);
    expect("over-read rejected (offset at end)", roaming_resume_is_safe(1024, 4096, 1024, 1), 0);

    printf("Attack: size_t offset+length overflow\n");
    expect("overflow rejected", roaming_resume_is_safe(1024, 4096, 1, (size_t)-1), 0);

    printf("Defense-in-depth: legitimate in-bounds resume window\n");
    expect("in-bounds window safe", roaming_resume_is_safe(1024, 4096, 512, 512), 1);

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
