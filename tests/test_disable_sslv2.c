#include "../kernel_patches/CVE-2016-0800-DROWN/disable_sslv2.c"

static int failures;

static void check(int condition)
{
    if (!condition) {
        failures++;
    }
}

static void test_sslv2_client_hello_is_rejected(void)
{
    // Genuine SSLv2 ClientHello prefix: 2-byte length header with the
    // high bit set (0x80 0x2e = 46-byte record), message type 1,
    // offered version 0x0002 (SSLv2).
    const unsigned char sslv2_hello[] = {
        0x80, 0x2e, 0x01, 0x00, 0x02, 0x00, 0x15, 0x00,
        0x00, 0x00, 0x10
    };
    int alert = ALERT_NONE;

    check(is_sslv2_client_hello(sslv2_hello, sizeof(sslv2_hello)) == 1);
    check(accept_first_client_record(sslv2_hello, sizeof(sslv2_hello),
                                     &alert) == 0);
    check(alert == ALERT_PROTOCOL_VERSION);
}

static void test_sslv2_hello_offering_tls_is_still_rejected(void)
{
    // DROWN doesn't care what version the SSLv2 hello OFFERS -- the
    // SSLv2 handshake itself is the oracle. Offering TLS 1.2 inside an
    // SSLv2-format record must still be refused.
    const unsigned char sslv2_offering_tls[] = {
        0x80, 0x2e, 0x01, 0x03, 0x03, 0x00, 0x15, 0x00,
        0x00, 0x00, 0x10
    };
    int alert = ALERT_NONE;

    check(accept_first_client_record(sslv2_offering_tls,
                                     sizeof(sslv2_offering_tls),
                                     &alert) == 0);
    check(alert == ALERT_PROTOCOL_VERSION);
}

static void test_tls_client_hello_is_accepted(void)
{
    // TLS record header: handshake (0x16), version, length...
    const unsigned char tls10[] = { 0x16, 0x03, 0x01, 0x00, 0x2f, 0x01 };
    const unsigned char tls12[] = { 0x16, 0x03, 0x03, 0x00, 0x2f, 0x01 };
    int alert = ALERT_NONE;

    check(accept_first_client_record(tls10, sizeof(tls10), &alert) == 1);
    check(alert == ALERT_NONE);
    check(accept_first_client_record(tls12, sizeof(tls12), &alert) == 1);
    check(alert == ALERT_NONE);
}

static void test_sslv3_record_is_rejected_at_floor(void)
{
    // SSL 3.0 record version is below the TLS 1.0 floor (POODLE).
    const unsigned char ssl3[] = { 0x16, 0x03, 0x00, 0x00, 0x2f, 0x01 };
    int alert = ALERT_NONE;

    check(sslv2_version_is_allowed(SSL3_VERSION) == 0);
    check(sslv2_version_is_allowed(SSL2_VERSION) == 0);
    check(sslv2_version_is_allowed(TLS1_0_VERSION) == 1);
    check(accept_first_client_record(ssl3, sizeof(ssl3), &alert) == 0);
    check(alert == ALERT_PROTOCOL_VERSION);
}

static void test_garbage_first_byte_is_rejected(void)
{
    const unsigned char garbage[] = { 0x47, 0x45, 0x54, 0x20 };  // "GET "
    int alert = ALERT_NONE;

    check(accept_first_client_record(garbage, sizeof(garbage), &alert) == 0);
    check(alert == ALERT_PROTOCOL_VERSION);
}

static void test_high_bit_but_not_client_hello(void)
{
    // High bit set but message type is not CLIENT-HELLO: not detected
    // as an SSLv2 hello (and still rejected as garbage at the door).
    const unsigned char not_hello[] = { 0x80, 0x10, 0x05, 0x00, 0x02 };
    int alert = ALERT_NONE;

    check(is_sslv2_client_hello(not_hello, sizeof(not_hello)) == 0);
    check(accept_first_client_record(not_hello, sizeof(not_hello),
                                     &alert) == 0);
}

static void test_short_and_null_buffers_are_safe(void)
{
    const unsigned char two[] = { 0x80, 0x2e };
    int alert = ALERT_NONE;

    check(is_sslv2_client_hello(NULL, 64) == 0);
    check(is_sslv2_client_hello(two, sizeof(two)) == 0);
    check(accept_first_client_record(NULL, 64, &alert) == 0);
    check(accept_first_client_record(two, sizeof(two), &alert) == 0);
    check(accept_first_client_record(two, 0, NULL) == 0);  // NULL alert OK
}

int main(void)
{
    test_sslv2_client_hello_is_rejected();
    test_sslv2_hello_offering_tls_is_still_rejected();
    test_tls_client_hello_is_accepted();
    test_sslv3_record_is_rejected_at_floor();
    test_garbage_first_byte_is_rejected();
    test_high_bit_but_not_client_hello();
    test_short_and_null_buffers_are_safe();

    return failures == 0 ? 0 : 1;
}
