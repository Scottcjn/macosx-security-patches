#include "../kernel_patches/CVE-2016-0777-OpenSSH-Roaming/disable_roaming.c"

static int failures;

static void check(int condition)
{
    if (!condition) {
        failures++;
    }
}

static void test_roaming_is_disabled_by_default(void)
{
    // The primary fix: client roaming must be off.
    check(client_roaming_is_enabled() == 0);

    // With roaming off, no resume request is ever honored, even a
    // perfectly in-bounds one.
    check(roaming_bytes_to_send(1024, 4096, 0, 512) == -1);
}

static void test_in_bounds_resume_is_accepted(void)
{
    // The bounds layer (defense in depth) accepts a window fully inside
    // the genuinely buffered bytes.
    check(roaming_resume_is_safe(1024, 4096, 0, 1024) == 1);
    check(roaming_resume_is_safe(1024, 4096, 512, 512) == 1);
    check(roaming_resume_is_safe(1024, 4096, 1023, 1) == 1);
}

static void test_the_overread_is_rejected(void)
{
    // CVE-2016-0777 core: server asks for more than was buffered ->
    // would leak adjacent heap. Must be rejected.
    check(roaming_resume_is_safe(1024, 4096, 0, 4096) == 0);   // past buffered
    check(roaming_resume_is_safe(1024, 4096, 1024, 1) == 0);   // offset at end
    check(roaming_resume_is_safe(1024, 4096, 2000, 1) == 0);   // offset > buffered
    check(roaming_resume_is_safe(1024, 4096, 512, 1024) == 0); // window overruns
}

static void test_corrupt_buffer_state_is_rejected(void)
{
    // buffered larger than capacity is an impossible/corrupt state.
    check(roaming_resume_is_safe(8192, 4096, 0, 1) == 0);
}

static void test_size_t_overflow_is_handled(void)
{
    // Attacker-controlled offset + length must not wrap. A huge req_len
    // with a small offset must be rejected, not overflow into "safe".
    check(roaming_resume_is_safe(1024, 4096, 1, (size_t)-1) == 0);
    check(roaming_resume_is_safe(1024, 4096, (size_t)-1, 8) == 0);
}

static void test_zero_length_is_harmless(void)
{
    // No bytes requested -> nothing to leak.
    check(roaming_resume_is_safe(1024, 4096, 0, 0) == 1);
}

int main(void)
{
    test_roaming_is_disabled_by_default();
    test_in_bounds_resume_is_accepted();
    test_the_overread_is_rejected();
    test_corrupt_buffer_state_is_rejected();
    test_size_t_overflow_is_handled();
    test_zero_length_is_harmless();

    return failures == 0 ? 0 : 1;
}
