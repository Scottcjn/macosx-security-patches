#include "../kernel_patches/CVE-2022-37434-zlib-inflate/gzip_extra_bounds.c"

static int failures;

static void check(int condition)
{
    if (!condition) {
        failures++;
    }
}

// Destination buffer wrapped in canary bytes: any write outside the
// caller's extra_max region flips a canary and fails the test.
#define CANARY 0xAA
#define EXTRA_CAP 16
#define PAD 32

typedef struct {
    unsigned char before[PAD];
    unsigned char extra[EXTRA_CAP];
    unsigned char after[PAD];
} guarded_buf;

static void guarded_init(guarded_buf *g)
{
    memset(g, CANARY, sizeof(*g));
}

static int canaries_intact(const guarded_buf *g)
{
    size_t i;
    for (i = 0; i < PAD; i++) {
        if (g->before[i] != CANARY || g->after[i] != CANARY) {
            return 0;
        }
    }
    return 1;
}

static void test_in_bounds_field_is_stored_fully(void)
{
    guarded_buf g;
    gz_header_model head;
    unsigned char input[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    guarded_init(&g);
    head.extra = g.extra;
    head.extra_max = EXTRA_CAP;
    head.extra_len = sizeof(input);

    check(gzip_extra_consume_all(&head, input, sizeof(input), 4)
              == sizeof(input));
    check(memcmp(g.extra, input, sizeof(input)) == 0);
    check(canaries_intact(&g));
}

static void test_oversized_field_never_escapes_extra_max(void)
{
    guarded_buf g;
    gz_header_model head;
    unsigned char input[64];
    size_t i;

    for (i = 0; i < sizeof(input); i++) input[i] = (unsigned char)i;

    guarded_init(&g);
    head.extra = g.extra;
    head.extra_max = EXTRA_CAP;
    head.extra_len = sizeof(input);  // stream declares 64, buffer holds 16

    // Whole field is consumed from the stream...
    check(gzip_extra_consume_all(&head, input, sizeof(input), 8)
              == sizeof(input));
    // ...but only extra_max bytes are stored, and nothing leaked past.
    check(memcmp(g.extra, input, EXTRA_CAP) == 0);
    check(canaries_intact(&g));
}

static void test_cve_trigger_offset_at_or_past_extra_max(void)
{
    guarded_buf g;
    gz_header_model head;
    unsigned char input[8] = {9, 9, 9, 9, 9, 9, 9, 9};

    guarded_init(&g);
    head.extra = g.extra;
    head.extra_max = EXTRA_CAP;
    head.extra_len = 64;

    // Deliver a chunk whose field offset (extra_len - remaining = 40)
    // is already past extra_max. Unfixed code computes
    // extra_max - 40 (unsigned underflow) and memcpy's ~SIZE_MAX bytes.
    check(gzip_extra_copy(&head, 24, input, sizeof(input)) == sizeof(input));
    check(canaries_intact(&g));

    // Exactly at the boundary (offset == extra_max) must also store
    // nothing: `len < extra_max` is strict.
    guarded_init(&g);
    check(gzip_extra_copy(&head, 64 - EXTRA_CAP, input, sizeof(input))
              == sizeof(input));
    check(g.extra[0] == CANARY);  // untouched
    check(canaries_intact(&g));
}

static void test_one_byte_streaming_chunks(void)
{
    guarded_buf g;
    gz_header_model head;
    unsigned char input[24];
    size_t i;

    for (i = 0; i < sizeof(input); i++) input[i] = (unsigned char)(100 + i);

    guarded_init(&g);
    head.extra = g.extra;
    head.extra_max = EXTRA_CAP;
    head.extra_len = sizeof(input);

    check(gzip_extra_consume_all(&head, input, sizeof(input), 1)
              == sizeof(input));
    check(memcmp(g.extra, input, EXTRA_CAP) == 0);
    check(canaries_intact(&g));
}

static void test_null_extra_and_null_head_are_safe(void)
{
    gz_header_model head;
    unsigned char input[8] = {0};

    // Caller passed no buffer: field still consumed, nothing stored.
    head.extra = NULL;
    head.extra_max = 0;
    head.extra_len = sizeof(input);
    check(gzip_extra_copy(&head, sizeof(input), input, sizeof(input))
              == sizeof(input));

    // No header at all.
    check(gzip_extra_copy(NULL, sizeof(input), input, sizeof(input))
              == sizeof(input));
}

static void test_zero_length_input_is_safe(void)
{
    guarded_buf g;
    gz_header_model head;
    unsigned char input[1] = {7};

    guarded_init(&g);
    head.extra = g.extra;
    head.extra_max = EXTRA_CAP;
    head.extra_len = 8;

    check(gzip_extra_copy(&head, 8, input, 0) == 0);
    check(gzip_extra_copy(&head, 0, input, 1) == 0);
    check(gzip_extra_copy(&head, 8, NULL, 1) == 0);
    check(canaries_intact(&g));
}

int main(void)
{
    test_in_bounds_field_is_stored_fully();
    test_oversized_field_never_escapes_extra_max();
    test_cve_trigger_offset_at_or_past_extra_max();
    test_one_byte_streaming_chunks();
    test_null_extra_and_null_head_are_safe();
    test_zero_length_input_is_safe();

    return failures == 0 ? 0 : 1;
}
