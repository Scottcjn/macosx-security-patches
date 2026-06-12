#include "../kernel_patches/CVE-2014-0224-CCS-Injection/ccs_state_gate.c"

static int failures;

static void check(int condition)
{
    if (!condition) {
        failures++;
    }
}

static void test_early_ccs_is_rejected(void)
{
    ccs_state st;
    int alert = ALERT_NONE;

    ccs_state_init(&st);

    // CCS before the key exchange completes is the CVE itself: keys
    // would derive from an empty master secret. Must be refused.
    check(ccs_receive(&st, &alert) == 0);
    check(alert == ALERT_UNEXPECTED_MESSAGE);
}

static void test_correct_sequence_is_accepted(void)
{
    ccs_state st;
    int alert = ALERT_NONE;

    ccs_state_init(&st);
    ccs_mark_master_secret_ready(&st);

    check(ccs_receive(&st, &alert) == 1);
    check(alert == ALERT_NONE);

    check(ccs_finished_receive(&st, &alert) == 1);
    check(alert == ALERT_NONE);
}

static void test_duplicate_ccs_is_rejected(void)
{
    ccs_state st;
    int alert = ALERT_NONE;

    ccs_state_init(&st);
    ccs_mark_master_secret_ready(&st);

    check(ccs_receive(&st, &alert) == 1);

    // A second CCS in the same handshake is never legitimate.
    check(ccs_receive(&st, &alert) == 0);
    check(alert == ALERT_UNEXPECTED_MESSAGE);
}

static void test_finished_before_ccs_is_rejected(void)
{
    ccs_state st;
    int alert = ALERT_NONE;

    ccs_state_init(&st);
    ccs_mark_master_secret_ready(&st);

    // Finished is the first record under the NEW keys; without a CCS
    // first, the state machine was bypassed.
    check(ccs_finished_receive(&st, &alert) == 0);
    check(alert == ALERT_UNEXPECTED_MESSAGE);
}

static void test_duplicate_finished_is_rejected(void)
{
    ccs_state st;
    int alert = ALERT_NONE;

    ccs_state_init(&st);
    ccs_mark_master_secret_ready(&st);
    check(ccs_receive(&st, &alert) == 1);
    check(ccs_finished_receive(&st, &alert) == 1);

    check(ccs_finished_receive(&st, &alert) == 0);
    check(alert == ALERT_UNEXPECTED_MESSAGE);
}

static void test_reinit_allows_fresh_handshake(void)
{
    ccs_state st;
    int alert = ALERT_NONE;

    ccs_state_init(&st);
    ccs_mark_master_secret_ready(&st);
    check(ccs_receive(&st, &alert) == 1);

    // Renegotiation: gate resets, sequence must be honored again.
    ccs_state_init(&st);
    check(ccs_receive(&st, &alert) == 0);
    check(alert == ALERT_UNEXPECTED_MESSAGE);
    ccs_mark_master_secret_ready(&st);
    check(ccs_receive(&st, &alert) == 1);
}

static void test_null_pointers_are_safe(void)
{
    int alert = ALERT_NONE;

    // Must not crash on NULL state or NULL alert pointer.
    ccs_state_init(NULL);
    ccs_mark_master_secret_ready(NULL);
    check(ccs_receive(NULL, &alert) == 0);
    check(ccs_finished_receive(NULL, NULL) == 0);

    {
        ccs_state st;
        ccs_state_init(&st);
        ccs_mark_master_secret_ready(&st);
        check(ccs_receive(&st, NULL) == 1);
    }
}

int main(void)
{
    test_early_ccs_is_rejected();
    test_correct_sequence_is_accepted();
    test_duplicate_ccs_is_rejected();
    test_finished_before_ccs_is_rejected();
    test_duplicate_finished_is_rejected();
    test_reinit_allows_fresh_handshake();
    test_null_pointers_are_safe();

    return failures == 0 ? 0 : 1;
}
