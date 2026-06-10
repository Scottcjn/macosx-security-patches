#include "../kernel_patches/CVE-2014-0160-Heartbleed/heartbeat_bounds.c"

static int failures;

static void check(int condition)
{
    if (!condition) {
        failures++;
    }
}

// Build a heartbeat record: [type][declared_len hi][declared_len lo][body...]
// body_len is how many bytes we actually place after the 3-byte header.
static size_t build_heartbeat(uint8_t *buf, uint8_t type,
                              uint16_t declared_len, size_t body_len)
{
    buf[0] = type;
    buf[1] = (uint8_t)(declared_len >> 8);
    buf[2] = (uint8_t)(declared_len & 0xFF);
    memset(buf + 3, 'A', body_len);
    return 3 + body_len;
}

static void test_accepts_well_formed_request(void)
{
    uint8_t buf[256];
    // declared payload 32, plus 16 padding -> record of 3 + 32 + 16 = 51
    size_t len = build_heartbeat(buf, TLS1_HB_REQUEST, 32, 32 + 16);

    check(heartbeat_request_is_safe(buf, len) == 1);
    check(heartbeat_should_respond(buf, len) == 1);
    check(heartbeat_safe_response_payload(buf, len) == 32);
}

static void test_rejects_the_heartbleed_overread(void)
{
    uint8_t buf[64];
    // The classic attack: claim 0xFFFF payload but send almost nothing.
    size_t len = build_heartbeat(buf, TLS1_HB_REQUEST, 0xFFFF, 1);

    check(heartbeat_request_is_safe(buf, len) == 0);
    check(heartbeat_should_respond(buf, len) == 0);
    check(heartbeat_safe_response_payload(buf, len) == -1);
}

static void test_rejects_zero_length_probe(void)
{
    uint8_t buf[64];
    // 0-length heartbeat (only a header, no padding) must be discarded.
    size_t len = build_heartbeat(buf, TLS1_HB_REQUEST, 0, 0);

    check(heartbeat_request_is_safe(buf, len) == 0);
    check(heartbeat_safe_response_payload(buf, len) == -1);
}

static void test_rejects_null_and_short_records(void)
{
    uint8_t buf[64];
    size_t len = build_heartbeat(buf, TLS1_HB_REQUEST, 4, 4 + 16);

    check(heartbeat_request_is_safe(NULL, len) == 0);
    // A record one byte short of header + min padding must fail.
    check(heartbeat_request_is_safe(buf, TLS_HB_HEADER_LEN
                                         + TLS_HB_MIN_PADDING - 1) == 0);
}

static void test_padding_boundary_is_enforced(void)
{
    uint8_t buf[256];
    // Exactly enough room: declared 10 + 16 padding + 3 header = 29.
    size_t exact = build_heartbeat(buf, TLS1_HB_REQUEST, 10, 10 + 16);
    check(heartbeat_request_is_safe(buf, exact) == 1);

    // One byte short of the mandatory 16-byte padding must be rejected,
    // even though the payload itself would fit.
    size_t short_pad = build_heartbeat(buf, TLS1_HB_REQUEST, 10, 10 + 15);
    check(heartbeat_request_is_safe(buf, short_pad) == 0);
}

static void test_only_requests_are_answered(void)
{
    uint8_t buf[256];
    size_t len = build_heartbeat(buf, TLS1_HB_RESPONSE, 32, 32 + 16);

    // A well-formed RESPONSE is in-bounds but must not be echoed back.
    check(heartbeat_request_is_safe(buf, len) == 1);
    check(heartbeat_should_respond(buf, len) == 0);
}

int main(void)
{
    test_accepts_well_formed_request();
    test_rejects_the_heartbleed_overread();
    test_rejects_zero_length_probe();
    test_rejects_null_and_short_records();
    test_padding_boundary_is_enforced();
    test_only_requests_are_answered();

    return failures == 0 ? 0 : 1;
}
