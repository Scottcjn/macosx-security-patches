#include "../kernel_patches/CVE-2014-3566-POODLE/disable_sslv3.c"

static int failures;

static void check(int condition)
{
    if (!condition) {
        failures++;
    }
}

static void test_sslv3_is_rejected(void)
{
    int alert = ALERT_NONE;

    check(protocol_version_is_allowed(SSL3_VERSION) == 0);
    check(is_sslv3(SSL3_VERSION) == 1);

    // A client offering SSL 3.0 must be refused with protocol_version.
    check(negotiate_protocol(SSL3_VERSION, TLS1_2_VERSION, 0, &alert) == 0);
    check(alert == ALERT_PROTOCOL_VERSION);
}

static void test_tls_versions_are_allowed(void)
{
    check(protocol_version_is_allowed(TLS1_0_VERSION) == 1);
    check(protocol_version_is_allowed(TLS1_1_VERSION) == 1);
    check(protocol_version_is_allowed(TLS1_2_VERSION) == 1);
}

static void test_normal_handshake_negotiates_min_of_both(void)
{
    int alert = ALERT_NONE;

    // Client maxes at TLS 1.0, server at TLS 1.2 -> TLS 1.0, no fallback.
    check(negotiate_protocol(TLS1_0_VERSION, TLS1_2_VERSION, 0, &alert)
              == TLS1_0_VERSION);
    check(alert == ALERT_NONE);

    // Both at TLS 1.2 -> TLS 1.2.
    check(negotiate_protocol(TLS1_2_VERSION, TLS1_2_VERSION, 0, &alert)
              == TLS1_2_VERSION);
    check(alert == ALERT_NONE);
}

static void test_fallback_scsv_blocks_downgrade(void)
{
    int alert = ALERT_NONE;

    // Client retries at TLS 1.0 WITH fallback SCSV while server supports
    // TLS 1.2 -> active downgrade -> inappropriate_fallback.
    check(negotiate_protocol(TLS1_0_VERSION, TLS1_2_VERSION, 1, &alert) == 0);
    check(alert == ALERT_INAPPROPRIATE_FALLBACK);
}

static void test_fallback_scsv_at_top_version_is_fine(void)
{
    int alert = ALERT_NONE;

    // Client offers server's max AND sends fallback SCSV -> legitimate,
    // not a downgrade.
    check(negotiate_protocol(TLS1_2_VERSION, TLS1_2_VERSION, 1, &alert)
              == TLS1_2_VERSION);
    check(alert == ALERT_NONE);
}

static void test_null_alert_pointer_is_safe(void)
{
    // Must not crash when caller doesn't want the alert code.
    check(negotiate_protocol(SSL3_VERSION, TLS1_2_VERSION, 0, NULL) == 0);
    check(negotiate_protocol(TLS1_2_VERSION, TLS1_2_VERSION, 0, NULL)
              == TLS1_2_VERSION);
}

int main(void)
{
    test_sslv3_is_rejected();
    test_tls_versions_are_allowed();
    test_normal_handshake_negotiates_min_of_both();
    test_fallback_scsv_blocks_downgrade();
    test_fallback_scsv_at_top_version_is_fine();
    test_null_alert_pointer_is_safe();

    return failures == 0 ? 0 : 1;
}
