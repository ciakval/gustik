#include <unity.h>
#include <string>
#include "transmit/ingest_response.h"

void setUp(void) {}
void tearDown(void) {}

void test_parses_a_fully_stored_batch(void) {
    IngestResponse r = parseIngestResponse("{\"received\":2,\"inserted\":2,\"duplicates\":0,\"backfilled\":0}");
    TEST_ASSERT_TRUE(r.hasCounts);
    TEST_ASSERT_EQUAL_INT(2, r.received);
    TEST_ASSERT_EQUAL_INT(2, r.inserted);
    TEST_ASSERT_EQUAL_INT(0, r.duplicates);
    TEST_ASSERT_EQUAL_INT(0, r.backfilled);
}

void test_parses_the_stored_nothing_body_including_its_warning_string(void) {
    IngestResponse r = parseIngestResponse(
        "{\"received\":1,\"inserted\":0,\"duplicates\":1,\"backfilled\":0,"
        "\"warning\":\"no readings stored: every clientId in this batch was already known\"}");
    TEST_ASSERT_TRUE(r.hasCounts);
    TEST_ASSERT_EQUAL_INT(1, r.received);
    TEST_ASSERT_EQUAL_INT(0, r.inserted);
    TEST_ASSERT_EQUAL_INT(1, r.duplicates);
}

void test_parses_a_backfill_batch(void) {
    IngestResponse r = parseIngestResponse("{\"received\":5,\"inserted\":4,\"duplicates\":1,\"backfilled\":4}");
    TEST_ASSERT_EQUAL_INT(4, r.inserted);
    TEST_ASSERT_EQUAL_INT(1, r.duplicates);
    TEST_ASSERT_EQUAL_INT(4, r.backfilled);
}

void test_tolerates_whitespace_around_names_and_values(void) {
    IngestResponse r = parseIngestResponse("{ \"received\" : 3 , \"inserted\" : 3 }");
    TEST_ASSERT_TRUE(r.hasCounts);
    TEST_ASSERT_EQUAL_INT(3, r.received);
    TEST_ASSERT_EQUAL_INT(3, r.inserted);
}

void test_reports_no_counts_for_an_error_body(void) {
    IngestResponse r = parseIngestResponse("{\"error\":\"unauthorized\"}");
    TEST_ASSERT_FALSE(r.hasCounts);
}

void test_reports_no_counts_for_an_empty_body(void) {
    TEST_ASSERT_FALSE(parseIngestResponse("").hasCounts);
    TEST_ASSERT_FALSE(parseIngestResponse("{}").hasCounts);
}

void test_reports_no_counts_when_only_one_of_the_required_fields_is_present(void) {
    // Half-parsed counts are worse than none: a phantom inserted=0 reads
    // exactly like the "backend stored nothing" alarm.
    TEST_ASSERT_FALSE(parseIngestResponse("{\"inserted\":2}").hasCounts);
    TEST_ASSERT_FALSE(parseIngestResponse("{\"received\":2}").hasCounts);
}

void test_does_not_match_a_field_whose_name_merely_contains_the_one_sought(void) {
    IngestResponse r = parseIngestResponse("{\"notinserted\":9,\"inserted_total\":9,\"received\":1,\"inserted\":1}");
    TEST_ASSERT_TRUE(r.hasCounts);
    TEST_ASSERT_EQUAL_INT(1, r.inserted);
}

void test_reports_no_counts_when_a_value_is_not_a_number(void) {
    TEST_ASSERT_FALSE(parseIngestResponse("{\"received\":\"2\",\"inserted\":null}").hasCounts);
}

void test_describes_a_failed_send_with_and_without_a_status_code(void) {
    IngestResponse none;
    TEST_ASSERT_EQUAL_STRING("send=FAILED (no response)", describeIngestOutcome(false, -1, none).c_str());
    TEST_ASSERT_EQUAL_STRING("send=FAILED http=401", describeIngestOutcome(false, 401, none).c_str());
}

void test_describes_a_normal_fully_stored_send(void) {
    IngestResponse r = parseIngestResponse("{\"received\":1,\"inserted\":1,\"duplicates\":0,\"backfilled\":0}");
    TEST_ASSERT_EQUAL_STRING("send=ok http=201 stored=1/1", describeIngestOutcome(true, 201, r).c_str());
}

void test_describes_the_stored_nothing_case_as_an_explicit_alarm(void) {
    // This is the line that would have made bug-031 visible on Serial
    // instead of needing a live device-vs-backend cross-check by hand.
    IngestResponse r = parseIngestResponse("{\"received\":1,\"inserted\":0,\"duplicates\":1,\"backfilled\":0}");
    TEST_ASSERT_EQUAL_STRING("send=ok http=200 stored=0/1 !! BACKEND STORED NOTHING (duplicate clientId)",
                             describeIngestOutcome(true, 200, r).c_str());
}

void test_describes_a_partial_backfill_without_raising_the_alarm(void) {
    IngestResponse r = parseIngestResponse("{\"received\":5,\"inserted\":4,\"duplicates\":1,\"backfilled\":4}");
    TEST_ASSERT_EQUAL_STRING("send=ok http=201 stored=4/5 backfilled=4 (duplicates=1)",
                             describeIngestOutcome(true, 201, r).c_str());
}

void test_describes_a_2xx_whose_body_carried_no_counts(void) {
    IngestResponse none;
    TEST_ASSERT_EQUAL_STRING("send=ok http=201 (no counts in response)",
                             describeIngestOutcome(true, 201, none).c_str());
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_parses_a_fully_stored_batch);
    RUN_TEST(test_parses_the_stored_nothing_body_including_its_warning_string);
    RUN_TEST(test_parses_a_backfill_batch);
    RUN_TEST(test_tolerates_whitespace_around_names_and_values);
    RUN_TEST(test_reports_no_counts_for_an_error_body);
    RUN_TEST(test_reports_no_counts_for_an_empty_body);
    RUN_TEST(test_reports_no_counts_when_only_one_of_the_required_fields_is_present);
    RUN_TEST(test_does_not_match_a_field_whose_name_merely_contains_the_one_sought);
    RUN_TEST(test_reports_no_counts_when_a_value_is_not_a_number);
    RUN_TEST(test_describes_a_failed_send_with_and_without_a_status_code);
    RUN_TEST(test_describes_a_normal_fully_stored_send);
    RUN_TEST(test_describes_the_stored_nothing_case_as_an_explicit_alarm);
    RUN_TEST(test_describes_a_partial_backfill_without_raising_the_alarm);
    RUN_TEST(test_describes_a_2xx_whose_body_carried_no_counts);
    return UNITY_END();
}
