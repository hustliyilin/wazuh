
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <string.h>

#include "../wrappers/wazuh/shared/debug_op_wrappers.h"
#include "../wrappers/wazuh/wazuh_db/wdb_syscollector_wrappers.h"
#include "../wrappers/wazuh/wazuh_db/wdb_wrappers.h"
#include "../wrappers/wazuh/wazuh_db/wdb_agents_wrappers.h"

#include "os_err.h"
#include "wazuh_db/wdb.h"

extern const char* SYSCOLLECTOR_LEGACY_CHECKSUM_VALUE;
typedef struct test_struct {
    wdb_t *wdb;
    wdb_t *wdb_global;
    char *output;
} test_struct_t;

static int test_setup(void **state) {
    test_struct_t *init_data;

    init_data = malloc(sizeof(test_struct_t));
    init_data->wdb = malloc(sizeof(wdb_t));
    init_data->wdb_global = malloc(sizeof(wdb_t));
    init_data->wdb->id = strdup("000");
    init_data->output = calloc(256, sizeof(char));
    init_data->wdb->peer = 1234;

    *state = init_data;

    return 0;
}

static int test_teardown(void **state){
    test_struct_t *data  = (test_struct_t *)*state;

    free(data->output);
    free(data->wdb->id);
    free(data->wdb);
    free(data->wdb_global);
    free(data);

    return 0;
}

void test_wdb_parse_syscheck_no_space(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;

    expect_string(__wrap__mdebug2, formatted_msg, "DB(000) Invalid FIM query syntax: badquery_nospace");

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, "badquery_nospace", data->output);

    assert_string_equal(data->output, "err Invalid FIM query syntax, near \'badquery_nospace\'");
    assert_int_equal(ret, -1);
}

void test_scan_info_error(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("scan_info_get ");

    will_return(__wrap_wdb_scan_info_get, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Cannot get FIM scan info.");

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "err Cannot get fim scan info.");
    assert_int_equal(ret, -1);
    os_free(query);
}

void test_scan_info_ok(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("scan_info_get ");


    will_return(__wrap_wdb_scan_info_get, 1);

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "ok 0");
    assert_int_equal(ret, 1);

    os_free(query);
}


void test_update_info_error(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("updatedate ");

    will_return(__wrap_wdb_fim_update_date_entry, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Cannot update fim date field.");

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "err Cannot update fim date field.");
    assert_int_equal(ret, -1);

    os_free(query);
}

void test_update_info_ok(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("updatedate ");

    will_return(__wrap_wdb_fim_update_date_entry, 1);

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, 1);

    os_free(query);
}


void test_clean_old_entries_error(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("cleandb ");

    will_return(__wrap_wdb_fim_clean_old_entries, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Cannot clean fim database.");

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "err Cannot clean fim database.");
    assert_int_equal(ret, -1);

    os_free(query);
}

void test_clean_old_entries_ok(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("cleandb ");

    will_return(__wrap_wdb_fim_clean_old_entries, 1);

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, 1);

    os_free(query);
}

void test_scan_info_update_noarg(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("scan_info_update ");

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid scan_info fim query syntax.");

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "err Invalid Syscheck query syntax, near \'\'");
    assert_int_equal(ret, -1);

    os_free(query);
}

void test_scan_info_update_error(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("scan_info_update \"191919\" ");

    will_return(__wrap_wdb_scan_info_update, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Cannot save fim control message.");

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "err Cannot save fim control message");
    assert_int_equal(ret, -1);

    os_free(query);
}

void test_scan_info_update_ok(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("scan_info_update \"191919\" ");

    will_return(__wrap_wdb_scan_info_update, 1);

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, 1);

    os_free(query);
}

void test_scan_info_fim_check_control_error(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("control ");

    will_return(__wrap_wdb_scan_info_fim_checks_control, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Cannot save fim check_control message.");

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "err Cannot save fim control message");
    assert_int_equal(ret, -1);

    os_free(query);
}

void test_scan_info_fim_check_control_ok(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("control ");

    will_return(__wrap_wdb_scan_info_fim_checks_control, 1);

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, 1);

    os_free(query);
}

void test_syscheck_load_error(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("load ");

    will_return(__wrap_wdb_syscheck_load, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Cannot load FIM.");

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "err Cannot load Syscheck");
    assert_int_equal(ret, -1);

    os_free(query);
}

void test_syscheck_load_ok(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("load ");

    will_return(__wrap_wdb_syscheck_load, 1);

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "ok TEST STRING");
    assert_int_equal(ret, 1);

    os_free(query);
}

void test_fim_delete_error(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("delete ");

    will_return(__wrap_wdb_fim_delete, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Cannot delete FIM entry.");

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "err Cannot delete Syscheck");
    assert_int_equal(ret, -1);

    os_free(query);
}

void test_fim_delete_ok(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("delete ");

    will_return(__wrap_wdb_fim_delete, 1);

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, 1);

    os_free(query);
}

void test_syscheck_save_noarg(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("save ");

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid FIM query syntax.");
    expect_string(__wrap__mdebug2, formatted_msg, "DB(000) FIM query: ");

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "err Invalid Syscheck query syntax, near \'\'");
    assert_int_equal(ret, -1);

    os_free(query);
}

void test_syscheck_save_invalid_type(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("save invalid_type ");

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid FIM query syntax.");
    expect_string(__wrap__mdebug2, formatted_msg, "DB(000) FIM query: invalid_type");

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "err Invalid Syscheck query syntax, near \'invalid_type\'");
    assert_int_equal(ret, -1);

    os_free(query);
}

void test_syscheck_save_file_type_error(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("save file 1212121 ");

    will_return(__wrap_wdb_syscheck_save, -1);
    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Cannot save FIM.");

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "err Cannot save Syscheck");
    assert_int_equal(ret, -1);

    os_free(query);
}

void test_syscheck_save_file_nospace(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("save file ");

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid FIM query syntax.");
    expect_string(__wrap__mdebug2, formatted_msg, "FIM query: ");

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "err Invalid Syscheck query syntax, near \'\'");
    assert_int_equal(ret, -1);

    os_free(query);
}

void test_syscheck_save_file_type_ok(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("save file !1212121 ");

    will_return(__wrap_wdb_syscheck_save, 1);

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, 1);

    os_free(query);
}

void test_syscheck_save_registry_type_error(void **state) {
    int ret;

    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("save registry 1212121 ");

    will_return(__wrap_wdb_syscheck_save, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Cannot save FIM.");

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "err Cannot save Syscheck");
    assert_int_equal(ret, -1);

    os_free(query);
}

void test_syscheck_save_registry_type_ok(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("save registry !1212121 ");

    will_return(__wrap_wdb_syscheck_save, 1);

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, 1);

    os_free(query);
}

void test_syscheck_save2_error(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("save2 ");

    will_return(__wrap_wdb_syscheck_save2, -1);
    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Cannot save FIM.");

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "err Cannot save Syscheck");
    assert_int_equal(ret, -1);

    os_free(query);
}

void test_syscheck_save2_ok(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("save2 ");

    will_return(__wrap_wdb_syscheck_save2, 1);

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, 0);

    os_free(query);
}

void test_integrity_check_error(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("integrity_check_ ");

    will_return(__wrap_wdbi_query_checksum, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Cannot query FIM range checksum.");

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "err Cannot perform range checksum");
    assert_int_equal(ret, -1);

    os_free(query);
}

void test_integrity_check_no_data(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("integrity_check_ ");

    will_return(__wrap_wdbi_query_checksum, 0);

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "ok no_data");
    assert_int_equal(ret, 0);

    os_free(query);
}

void test_integrity_check_checksum_fail(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("integrity_check_ ");

    will_return(__wrap_wdbi_query_checksum, 1);

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "ok checksum_fail");
    assert_int_equal(ret, 0);

    os_free(query);
}

void test_integrity_check_ok(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("integrity_check_ ");

    will_return(__wrap_wdbi_query_checksum, 2);

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "ok ");
    assert_int_equal(ret, 0);

    os_free(query);
}

void test_integrity_clear_error(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("integrity_clear ");

    will_return(__wrap_wdbi_query_clear, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Cannot query FIM range checksum.");

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "err Cannot perform range checksum");
    assert_int_equal(ret, -1);

    os_free(query);
}

void test_integrity_clear_ok(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("integrity_clear ");

    will_return(__wrap_wdbi_query_clear, 2);

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "ok ");
    assert_int_equal(ret, 0);

    os_free(query);
}


void test_invalid_command(void **state) {
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("wrong_command ");

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid FIM query syntax.");
    expect_string(__wrap__mdebug2, formatted_msg, "DB query error near: wrong_command");

    ret = wdb_parse_syscheck(data->wdb, WDB_FIM_FILE, query, data->output);

    assert_string_equal(data->output, "err Invalid Syscheck query syntax, near 'wrong_command'");
    assert_int_equal(ret, -1);

    os_free(query);
}


void test_wdb_parse_rootcheck_badquery(void **state)
{
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("badquery ");
    expect_string(__wrap__mdebug2, formatted_msg, "DB(000) Invalid rootcheck query syntax: badquery");
    ret = wdb_parse_rootcheck(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid rootcheck query syntax, near 'badquery'");
    assert_int_equal(ret, -1);
    os_free(query);
}

void test_wdb_parse_rootcheck_delete_error(void **state)
{
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("delete");
    will_return(__wrap_wdb_stmt_cache, -1);
    expect_string(__wrap__merror, formatted_msg, "DB(000) Cannot cache statement");

    ret = wdb_parse_rootcheck(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Error deleting rootcheck PM tuple");
    assert_int_equal(ret, -1);
    os_free(query);
}

void test_wdb_parse_rootcheck_delete_ok(void **state)
{
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("delete");
    will_return(__wrap_wdb_stmt_cache, 0);
    will_return(__wrap_wdb_step, SQLITE_DONE);
    will_return(__wrap_sqlite3_changes, 10);

    ret = wdb_parse_rootcheck(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok 0");
    assert_int_equal(ret, 0);
    os_free(query);
}

void test_wdb_parse_rootcheck_save_invalid_no_next(void **state)
{
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("save");
    expect_string(__wrap__mdebug2, formatted_msg, "DB(000) Invalid rootcheck query syntax: save");
    ret = wdb_parse_rootcheck(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid rootcheck query syntax, near 'save'");
    assert_int_equal(ret, -1);
    os_free(query);
}

void test_wdb_parse_rootcheck_save_no_ptr(void **state)
{
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("save ");
    expect_string(__wrap__mdebug2, formatted_msg, "DB(000) Invalid rootcheck query syntax: save");
    ret = wdb_parse_rootcheck(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid rootcheck query syntax, near 'save'");
    assert_int_equal(ret, -1);
    os_free(query);
}

void test_wdb_parse_rootcheck_save_date_max_long(void **state)
{
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("save 9223372036854775807 asdasd");
    expect_string(__wrap__mdebug2, formatted_msg, "DB(000) Invalid rootcheck date timestamp: 9223372036854775807");
    ret = wdb_parse_rootcheck(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid rootcheck query syntax, near 'save'");
    assert_int_equal(ret, -1);
    os_free(query);
}

void test_wdb_parse_rootcheck_save_update_cache_error(void **state)
{
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("save 123456789 Test");

    will_return(__wrap_wdb_stmt_cache, -1);
    expect_string(__wrap__merror, formatted_msg, "DB(000) Cannot cache statement");

    expect_string(__wrap__merror, formatted_msg, "DB(000) Error updating rootcheck PM tuple on SQLite database");

    ret = wdb_parse_rootcheck(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Error updating rootcheck PM tuple");
    assert_int_equal(ret, -1);
    os_free(query);
}

void test_wdb_parse_rootcheck_save_update_success(void **state)
{
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("save 123456789 Test");

    will_return(__wrap_wdb_stmt_cache, 0);
    expect_value(__wrap_sqlite3_bind_int, index, 1);
    expect_value(__wrap_sqlite3_bind_int, value, 123456789);
    will_return_always(__wrap_sqlite3_bind_int, 1);
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "Test");
    will_return(__wrap_sqlite3_bind_text, 1);
    will_return(__wrap_wdb_step, SQLITE_DONE);
    will_return(__wrap_sqlite3_changes, 10);

    ret = wdb_parse_rootcheck(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok 1");
    assert_int_equal(ret, 0);
    os_free(query);
}

void test_wdb_parse_rootcheck_save_update_insert_cache_error(void **state)
{
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("save 123456789 Test");

    will_return(__wrap_wdb_stmt_cache, 0);
    expect_value(__wrap_sqlite3_bind_int, index, 1);
    expect_value(__wrap_sqlite3_bind_int, value, 123456789);
    will_return_always(__wrap_sqlite3_bind_int, 1);
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "Test");
    will_return(__wrap_sqlite3_bind_text, 1);
    will_return(__wrap_wdb_step, SQLITE_DONE);
    will_return(__wrap_sqlite3_changes, 0);

    will_return(__wrap_wdb_stmt_cache, -1);
    expect_string(__wrap__merror, formatted_msg, "DB(000) Cannot cache statement");

    expect_string(__wrap__merror, formatted_msg, "DB(000) Error inserting rootcheck PM tuple on SQLite database for agent");

    ret = wdb_parse_rootcheck(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Error updating rootcheck PM tuple");
    assert_int_equal(ret, -1);
    os_free(query);
}

void test_wdb_parse_rootcheck_save_update_insert_success(void **state)
{
    int ret;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = strdup("save 123456789 Test");

    will_return(__wrap_wdb_stmt_cache, 0);
    expect_value(__wrap_sqlite3_bind_int, index, 1);
    expect_value(__wrap_sqlite3_bind_int, value, 123456789);
    will_return_always(__wrap_sqlite3_bind_int, 1);
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "Test");
    will_return(__wrap_sqlite3_bind_text, 1);
    will_return(__wrap_wdb_step, SQLITE_DONE);
    will_return(__wrap_sqlite3_changes, 0);

    will_return(__wrap_wdb_stmt_cache, 0);
    expect_value(__wrap_sqlite3_bind_int, index, 1);
    expect_value(__wrap_sqlite3_bind_int, value, 123456789);
    expect_value(__wrap_sqlite3_bind_int, index, 2);
    expect_value(__wrap_sqlite3_bind_int, value, 123456789);

    expect_value(__wrap_sqlite3_bind_text, pos, 3);
    expect_string(__wrap_sqlite3_bind_text, buffer, "Test");
    will_return(__wrap_sqlite3_bind_text, 1);
    expect_value(__wrap_sqlite3_bind_text, pos, 4);
    will_return(__wrap_sqlite3_bind_text, 1);
    expect_value(__wrap_sqlite3_bind_text, pos, 5);
    will_return(__wrap_sqlite3_bind_text, 1);
    will_return(__wrap_wdb_step, SQLITE_DONE);
    will_return(__wrap_sqlite3_last_insert_rowid, 10);

    ret = wdb_parse_rootcheck(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok 2");
    assert_int_equal(ret, 0);
    os_free(query);
}

/* Tests osinfo */

void test_osinfo_syntax_error(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("agent 000 osinfo", query);

    expect_value(__wrap_wdb_open_agent2, agent_id, atoi(data->wdb->id));
    will_return(__wrap_wdb_open_agent2, data->wdb);
    expect_string(__wrap__mdebug2, formatted_msg, "Agent 000 query: osinfo");

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid DB query syntax.");
    expect_string(__wrap__mdebug2, formatted_msg, "DB(000) query error near: osinfo");

    ret = wdb_parse(query, data->output, 0);

    assert_string_equal(data->output, "err Invalid DB query syntax, near 'osinfo'");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_invalid_action(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("agent 000 osinfo invalid", query);
    expect_value(__wrap_wdb_open_agent2, agent_id, atoi(data->wdb->id));
    will_return(__wrap_wdb_open_agent2, data->wdb);
    expect_string(__wrap__mdebug2, formatted_msg, "Agent 000 query: osinfo invalid");

    ret = wdb_parse(query, data->output, 0);

    assert_string_equal(data->output, "err Invalid osinfo action: invalid");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_missing_action(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("", query);

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Missing osinfo action");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_get_error(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("get", query);

    // wdb_agents_get_sys_osinfo
    will_return(__wrap_wdb_agents_get_sys_osinfo, NULL);
    will_return_count(__wrap_sqlite3_errmsg, "ERROR MESSAGE", -1);

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Cannot get sys_osinfo database table information; SQL err: ERROR MESSAGE");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_get_success(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;
    char *result = NULL;

    os_strdup("get", query);
    os_strdup("[]", result);
    cJSON *test =  cJSON_CreateObject();

    // wdb_agents_get_sys_osinfo
    will_return(__wrap_wdb_agents_get_sys_osinfo, test);
    will_return(__wrap_cJSON_PrintUnformatted, result);

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok []");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_osinfo_set_triaged_error(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;
    os_strdup("set_triaged", query);

    // wdb_agents_set_sys_osinfo_triaged
    will_return(__wrap_wdb_agents_set_sys_osinfo_triaged, OS_INVALID);
    will_return_count(__wrap_sqlite3_errmsg, "ERROR MESSAGE", -1);

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Cannot set sys_osinfo as triaged; SQL err: ERROR MESSAGE");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_set_triaged_success(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;
    os_strdup("set_triaged", query);

    // wdb_agents_get_sys_osinfo
    will_return(__wrap_wdb_agents_set_sys_osinfo_triaged, OS_SUCCESS);

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_osinfo_set_error(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("set", query);

    // wdb_parse_agents_set_sys_osinfo
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid OS info query syntax.");

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid OS info query syntax");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_set_error_no_scan_id(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("set ", query);

    // wdb_parse_agents_set_sys_osinfo
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid OS info query syntax.");

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid OS info query syntax");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_set_error_no_scan_time(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("set NULL|", query);

    // wdb_parse_agents_set_sys_osinfo
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid OS info query syntax.");

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid OS info query syntax");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_set_error_no_hostname(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("set scan_id|NULL|", query);

    // wdb_parse_agents_set_sys_osinfo
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid OS info query syntax.");

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid OS info query syntax");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_set_error_no_architecture(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("set scan_id|scan_time|NULL|", query);

    // wdb_parse_agents_set_sys_osinfo
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid OS info query syntax.");

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid OS info query syntax");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_set_error_no_os_name(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("set scan_id|scan_time|hostname|NULL|", query);

    // wdb_parse_agents_set_sys_osinfo
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid OS info query syntax.");

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid OS info query syntax");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_set_error_no_os_version(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("set scan_id|scan_time|hostname|architecture|NULL|", query);

    // wdb_parse_agents_set_sys_osinfo
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid OS info query syntax.");

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid OS info query syntax");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_set_error_no_os_codename(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("set scan_id|scan_time|hostname|architecture|os_name|NULL|", query);

    // wdb_parse_agents_set_sys_osinfo
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid OS info query syntax.");

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid OS info query syntax");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_set_error_no_os_major(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("set scan_id|scan_time|hostname|architecture|os_name|os_version|NULL|", query);

    // wdb_parse_agents_set_sys_osinfo
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid OS info query syntax.");

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid OS info query syntax");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_set_error_no_os_minor(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("set scan_id|scan_time|hostname|architecture|os_name|os_version|os_codename|NULL|", query);

    // wdb_parse_agents_set_sys_osinfo
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid OS info query syntax.");

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid OS info query syntax");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_set_error_no_os_build(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("set scan_id|scan_time|hostname|architecture|os_name|os_version|os_codename|os_major|NULL|", query);

    // wdb_parse_agents_set_sys_osinfo
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid OS info query syntax.");

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid OS info query syntax");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_set_error_no_os_platform(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("set scan_id|scan_time|hostname|architecture|os_name|os_version|os_codename|os_major|os_minor|NULL|", query);

    // wdb_parse_agents_set_sys_osinfo
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid OS info query syntax.");

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid OS info query syntax");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_set_error_no_sysname(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("set scan_id|scan_time|hostname|architecture|os_name|os_version|os_codename|os_major|os_minor|os_build|NULL|", query);

    // wdb_parse_agents_set_sys_osinfo
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid OS info query syntax.");

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid OS info query syntax");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_set_error_no_release(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("set scan_id|scan_time|hostname|architecture|os_name|os_version|os_codename|os_major|os_minor|os_build|os_platform|NULL|", query);

    // wdb_parse_agents_set_sys_osinfo
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid OS info query syntax.");

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid OS info query syntax");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_set_error_no_version(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("set scan_id|scan_time|hostname|architecture|os_name|os_version|os_codename|os_major|os_minor|os_build|os_platform|sysname|NULL|", query);

    // wdb_parse_agents_set_sys_osinfo
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid OS info query syntax.");

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid OS info query syntax");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_set_error_no_os_release(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("set scan_id|scan_time|hostname|architecture|os_name|os_version|os_codename|os_major|os_minor|os_build|os_platform|sysname|NULL|NULL|", query);

    // wdb_parse_agents_set_sys_osinfo
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid OS info query syntax.");

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid OS info query syntax");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_set_error_saving(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("set scan_id|scan_time|hostname|architecture|os_name|os_version|os_codename|os_major|os_minor|os_build|os_platform|sysname|release|NULL|NULL|NULL|NULL", query);

    // wdb_parse_agents_set_sys_osinfo
    expect_string(__wrap_wdb_osinfo_save, scan_id, "scan_id");
    expect_string(__wrap_wdb_osinfo_save, scan_time, "scan_time");
    expect_string(__wrap_wdb_osinfo_save, hostname, "hostname");
    expect_string(__wrap_wdb_osinfo_save, architecture, "architecture");
    expect_string(__wrap_wdb_osinfo_save, os_name, "os_name");
    expect_string(__wrap_wdb_osinfo_save, os_version, "os_version");
    expect_string(__wrap_wdb_osinfo_save, os_codename, "os_codename");
    expect_string(__wrap_wdb_osinfo_save, os_major, "os_major");
    expect_string(__wrap_wdb_osinfo_save, os_minor, "os_minor");
    expect_string(__wrap_wdb_osinfo_save, os_build, "os_build");
    expect_string(__wrap_wdb_osinfo_save, os_platform, "os_platform");
    expect_string(__wrap_wdb_osinfo_save, sysname, "sysname");
    expect_string(__wrap_wdb_osinfo_save, release, "release");
    expect_string(__wrap_wdb_osinfo_save, checksum, "legacy");
    expect_value(__wrap_wdb_osinfo_save, replace, FALSE);
    will_return(__wrap_wdb_osinfo_save, OS_INVALID);
    expect_string(__wrap__mdebug1, formatted_msg, "Cannot save OS information.");

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Cannot save OS information.");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_osinfo_set_success(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("set scan_id|scan_time|hostname|architecture|os_name|os_version|os_codename|os_major|os_minor|os_build|os_platform|sysname|release|version|os_release|os_patch|os_display_version", query);

    // wdb_parse_agents_set_sys_osinfo
    expect_string(__wrap_wdb_osinfo_save, scan_id, "scan_id");
    expect_string(__wrap_wdb_osinfo_save, scan_time, "scan_time");
    expect_string(__wrap_wdb_osinfo_save, hostname, "hostname");
    expect_string(__wrap_wdb_osinfo_save, architecture, "architecture");
    expect_string(__wrap_wdb_osinfo_save, os_name, "os_name");
    expect_string(__wrap_wdb_osinfo_save, os_version, "os_version");
    expect_string(__wrap_wdb_osinfo_save, os_codename, "os_codename");
    expect_string(__wrap_wdb_osinfo_save, os_major, "os_major");
    expect_string(__wrap_wdb_osinfo_save, os_minor, "os_minor");
    expect_string(__wrap_wdb_osinfo_save, os_patch, "os_patch");
    expect_string(__wrap_wdb_osinfo_save, os_build, "os_build");
    expect_string(__wrap_wdb_osinfo_save, os_platform, "os_platform");
    expect_string(__wrap_wdb_osinfo_save, sysname, "sysname");
    expect_string(__wrap_wdb_osinfo_save, release, "release");
    expect_string(__wrap_wdb_osinfo_save, version, "version");
    expect_string(__wrap_wdb_osinfo_save, os_release, "os_release");
    expect_string(__wrap_wdb_osinfo_save, os_display_version, "os_display_version");
    expect_string(__wrap_wdb_osinfo_save, checksum, "legacy");
    expect_value(__wrap_wdb_osinfo_save, replace, FALSE);
    will_return(__wrap_wdb_osinfo_save, OS_SUCCESS);

    ret = wdb_parse_osinfo(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

/* Tests vuln_cves */

void test_vuln_cves_syntax_error(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("agent 000 vuln_cves", query);

    expect_value(__wrap_wdb_open_agent2, agent_id, atoi(data->wdb->id));
    will_return(__wrap_wdb_open_agent2, data->wdb);
    expect_string(__wrap__mdebug2, formatted_msg, "Agent 000 query: vuln_cves");

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid vuln_cves query syntax.");
    expect_string(__wrap__mdebug2, formatted_msg, "DB(000) vuln_cves query error near: vuln_cves");

    ret = wdb_parse(query, data->output, 0);

    assert_string_equal(data->output, "err Invalid vuln_cves query syntax, near 'vuln_cves'");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_vuln_cves_invalid_action(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("agent 000 vuln_cves invalid", query);
    expect_value(__wrap_wdb_open_agent2, agent_id, atoi(data->wdb->id));
    will_return(__wrap_wdb_open_agent2, data->wdb);
    expect_string(__wrap__mdebug2, formatted_msg, "Agent 000 query: vuln_cves invalid");

    ret = wdb_parse(query, data->output, 0);

    assert_string_equal(data->output, "err Invalid vuln_cves action: invalid");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_vuln_cves_missing_action(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("", query);

    ret = wdb_parse_vuln_cves(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Missing vuln_cves action");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_vuln_cves_insert_syntax_error(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("insert {\"name\":\"package\",\"version\":}", query);

    // wdb_parse_agents_insert_vuln_cves
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid vuln_cves JSON syntax when inserting vulnerable package.");
    expect_string(__wrap__mdebug2, formatted_msg, "JSON error near: }");

    ret = wdb_parse_vuln_cves(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid JSON syntax, near '{\"name\":\"package\",\"version\":}'");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_vuln_cves_insert_constraint_error(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("insert {\"name\":\"package\",\"version\":\"2.2\",\"architecture\":\"x86\"}", query);

    // wdb_parse_agents_insert_vuln_cves
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid vuln_cves JSON data when inserting vulnerable package."
    " Not compliant with constraints defined in the database.");

    ret = wdb_parse_vuln_cves(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid JSON data, missing required fields");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_vuln_cves_insert_command_error(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("insert {\"name\":\"package\",\"version\":\"2.2\",\"architecture\":\"x86\",\"cve\":\"CVE-2021-1500\","
              "\"reference\":\"8549fd9faf9b124635298e9311ccf672c2ad05d1\",\"type\":\"PACKAGE\",\"status\":\"VALID\","
              "\"check_pkg_existence\":true,\"severity\":null,\"cvss2_score\":0,\"cvss3_score\":0}", query);

    // wdb_parse_agents_insert_vuln_cves
    expect_string(__wrap_wdb_agents_insert_vuln_cves, name, "package");
    expect_string(__wrap_wdb_agents_insert_vuln_cves, version, "2.2");
    expect_string(__wrap_wdb_agents_insert_vuln_cves, architecture, "x86");
    expect_string(__wrap_wdb_agents_insert_vuln_cves, cve, "CVE-2021-1500");
    expect_string(__wrap_wdb_agents_insert_vuln_cves, reference, "8549fd9faf9b124635298e9311ccf672c2ad05d1");
    expect_string(__wrap_wdb_agents_insert_vuln_cves, type, "PACKAGE");
    expect_string(__wrap_wdb_agents_insert_vuln_cves, status, "VALID");
    expect_value(__wrap_wdb_agents_insert_vuln_cves, check_pkg_existence, true);
    expect_value(__wrap_wdb_agents_insert_vuln_cves, severity, NULL);
    expect_value(__wrap_wdb_agents_insert_vuln_cves, cvss2_score, 0);
    expect_value(__wrap_wdb_agents_insert_vuln_cves, cvss3_score, 0);
    will_return(__wrap_cJSON_PrintUnformatted, NULL);

    will_return(__wrap_wdb_agents_insert_vuln_cves, NULL);

    expect_string(__wrap__mdebug1, formatted_msg, "Error inserting vulnerability in vuln_cves.");

    ret = wdb_parse_vuln_cves(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Error inserting vulnerability in vuln_cves.");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_vuln_cves_insert_command_success(void **state) {
    int ret = OS_INVALID;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;
    char *result = NULL;
    os_strdup("[{\"test\":\"TEST\"}]", result);
    os_strdup("insert {\"name\":\"package\",\"version\":\"2.2\",\"architecture\":\"x86\",\"cve\":\"CVE-2021-1500\","
              "\"reference\":\"8549fd9faf9b124635298e9311ccf672c2ad05d1\",\"type\":\"PACKAGE\",\"status\":\"VALID\","
              "\"check_pkg_existence\":true,\"severity\":\"MEDIUM\",\"cvss2_score\":5.2,\"cvss3_score\":6,"
              "\"external_references\":[\"https.//refs.com/refs1\",\"https.//refs.com/refs1\"],\"condition\":\"Package unfixes\","
              "\"title\":\"CVE-2021-1500 affects package\",\"published\":\"01-01-2020\",\"updated\":\"02-01-2020\"}", query);

    cJSON *test =  cJSON_CreateObject();

    // wdb_parse_agents_insert_vuln_cves
    expect_string(__wrap_wdb_agents_insert_vuln_cves, name, "package");
    expect_string(__wrap_wdb_agents_insert_vuln_cves, version, "2.2");
    expect_string(__wrap_wdb_agents_insert_vuln_cves, architecture, "x86");
    expect_string(__wrap_wdb_agents_insert_vuln_cves, cve, "CVE-2021-1500");
    expect_string(__wrap_wdb_agents_insert_vuln_cves, reference, "8549fd9faf9b124635298e9311ccf672c2ad05d1");
    expect_string(__wrap_wdb_agents_insert_vuln_cves, type, "PACKAGE");
    expect_string(__wrap_wdb_agents_insert_vuln_cves, status, "VALID");
    expect_value(__wrap_wdb_agents_insert_vuln_cves, check_pkg_existence, true);
    expect_string(__wrap_wdb_agents_insert_vuln_cves, severity, "MEDIUM");
    expect_value(__wrap_wdb_agents_insert_vuln_cves, cvss2_score, 5.2);
    expect_value(__wrap_wdb_agents_insert_vuln_cves, cvss3_score, 6);
    will_return(__wrap_wdb_agents_insert_vuln_cves, test);
    will_return(__wrap_cJSON_PrintUnformatted, strdup("[\"https.//refs.com/refs1\",\"https.//refs.com/refs1\"]"));

    will_return(__wrap_cJSON_PrintUnformatted, result);

    ret = wdb_parse_vuln_cves(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok [{\"test\":\"TEST\"}]");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_vuln_cves_update_status_syntax_error(void **state){
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("update_status {\"old_status\",\"new_status\"}", query);

    // wdb_parse_agents_update_status_vuln_cves
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid vuln_cves JSON syntax when updating status value.");
    expect_string(__wrap__mdebug2, formatted_msg, "JSON error near: ,\"new_status\"}");

    ret = wdb_parse_vuln_cves(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid JSON syntax, near '{\"old_status\",\"new_status\"}'");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_vuln_cves_update_status_constraint_error(void **state){
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("update_status {\"old_status\":\"new_status\"}", query);

    // wdb_parse_agents_update_status_vuln_cves
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid vuln_cves JSON data when updating CVE's status.");

    ret = wdb_parse_vuln_cves(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid JSON data, missing or wrong required fields");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_vuln_cves_update_status_command_error(void **state){
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("update_status {\"old_status\":\"valid\",\"new_status\":\"obsolete\"}", query);

    // wdb_parse_agents_update_status_vuln_cves
    will_return(__wrap_wdb_agents_update_vuln_cves_status, OS_INVALID);
    expect_string(__wrap_wdb_agents_update_vuln_cves_status, old_status, "valid");
    expect_string(__wrap_wdb_agents_update_vuln_cves_status, new_status, "obsolete");
    expect_value(__wrap_wdb_agents_update_vuln_cves_status, type, NULL);
    will_return_count(__wrap_sqlite3_errmsg, "ERROR MESSAGE", -1);
    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Cannot execute vuln_cves update_status command; SQL err: ERROR MESSAGE");

    ret = wdb_parse_vuln_cves(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Cannot execute vuln_cves update_status command; SQL err: ERROR MESSAGE");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_vuln_cves_update_status_command_success(void **state){
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("update_status {\"old_status\":\"valid\",\"new_status\":\"obsolete\"}", query);

    // wdb_parse_agents_update_status_vuln_cves
    will_return(__wrap_wdb_agents_update_vuln_cves_status, OS_SUCCESS);
    expect_string(__wrap_wdb_agents_update_vuln_cves_status, old_status, "valid");
    expect_string(__wrap_wdb_agents_update_vuln_cves_status, new_status, "obsolete");
    expect_value(__wrap_wdb_agents_update_vuln_cves_status, type, NULL);

    ret = wdb_parse_vuln_cves(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_vuln_cves_update_status_by_type_command_error(void **state){
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("update_status {\"type\":\"PACKAGES\",\"new_status\":\"VALID\"}", query);

    // wdb_parse_agents_update_status_vuln_cves
    will_return(__wrap_wdb_agents_update_vuln_cves_status, OS_INVALID);
    expect_string(__wrap_wdb_agents_update_vuln_cves_status, type, "PACKAGES");
    expect_string(__wrap_wdb_agents_update_vuln_cves_status, new_status, "VALID");
    expect_value(__wrap_wdb_agents_update_vuln_cves_status, old_status, NULL);
    will_return_count(__wrap_sqlite3_errmsg, "ERROR MESSAGE", -1);
    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Cannot execute vuln_cves update_status command; SQL err: ERROR MESSAGE");

    ret = wdb_parse_vuln_cves(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Cannot execute vuln_cves update_status command; SQL err: ERROR MESSAGE");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_vuln_cves_update_status_by_type_command_success(void **state){
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("update_status {\"type\":\"PACKAGES\",\"new_status\":\"VALID\"}", query);

    // wdb_parse_agents_update_status_vuln_cves
    will_return(__wrap_wdb_agents_update_vuln_cves_status, OS_SUCCESS);
    expect_string(__wrap_wdb_agents_update_vuln_cves_status, type, "PACKAGES");
    expect_string(__wrap_wdb_agents_update_vuln_cves_status, new_status, "VALID");
    expect_value(__wrap_wdb_agents_update_vuln_cves_status, old_status, NULL);

    ret = wdb_parse_vuln_cves(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_vuln_cves_remove_syntax_error(void **state){
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("remove {\"status\"}", query);

    // wdb_parse_agents_update_status_vuln_cves
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid vuln_cves JSON syntax when removing vulnerabilities.");
    expect_string(__wrap__mdebug2, formatted_msg, "JSON error near: }");

    ret = wdb_parse_vuln_cves(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid JSON syntax, near '{\"status\"}'");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_vuln_cves_remove_json_data_error(void **state){
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("remove {}", query);

    // wdb_parse_agents_update_status_vuln_cves
    expect_string(__wrap__mdebug1, formatted_msg, "Invalid vuln_cves JSON data to remove vulnerabilities.");

    ret = wdb_parse_vuln_cves(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid JSON data");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_vuln_cves_remove_by_status_success(void **state){
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("remove {\"status\":\"OBSOLETE\"}", query);

    // wdb_agents_remove_vuln_cves_by_status
    expect_string(__wrap_wdb_agents_remove_vuln_cves_by_status, status, "OBSOLETE");
    will_return(__wrap_wdb_agents_remove_vuln_cves_by_status, "{\"cve\":\"cve-xxxx-yyyy\"}");
    will_return(__wrap_wdb_agents_remove_vuln_cves_by_status, WDBC_OK);

    ret = wdb_parse_vuln_cves(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok {\"cve\":\"cve-xxxx-yyyy\"}");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_vuln_cves_remove_entry_error(void **state){
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("remove {\"cve\":\"cve-xxxx-yyyy\",\"reference\":\"ref-cve-xxxx-yyyy\"}", query);

    // wdb_agents_remove_vuln_cves
    expect_string(__wrap_wdb_agents_remove_vuln_cves, cve, "cve-xxxx-yyyy");
    expect_string(__wrap_wdb_agents_remove_vuln_cves, reference, "ref-cve-xxxx-yyyy");
    will_return(__wrap_wdb_agents_remove_vuln_cves, OS_INVALID);

    will_return_count(__wrap_sqlite3_errmsg, "ERROR MESSAGE", -1);
    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Cannot execute vuln_cves remove command; SQL err: ERROR MESSAGE");

    ret = wdb_parse_vuln_cves(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Cannot execute vuln_cves remove command; SQL err: ERROR MESSAGE");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_vuln_cves_remove_entry_success(void **state){
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("remove {\"cve\":\"cve-xxxx-yyyy\",\"reference\":\"ref-cve-xxxx-yyyy\"}", query);

    // wdb_agents_remove_vuln_cves
    expect_string(__wrap_wdb_agents_remove_vuln_cves, cve, "cve-xxxx-yyyy");
    expect_string(__wrap_wdb_agents_remove_vuln_cves, reference, "ref-cve-xxxx-yyyy");
    will_return(__wrap_wdb_agents_remove_vuln_cves, OS_SUCCESS);

    ret = wdb_parse_vuln_cves(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_vuln_cves_clear_command_error(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("clear", query);

    // wdb_parse_agents_clear_vuln_cves
    will_return(__wrap_wdb_agents_clear_vuln_cves, OS_INVALID);
    will_return_count(__wrap_sqlite3_errmsg, "ERROR MESSAGE", -1);
    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Cannot execute vuln_cves clear command; SQL err: ERROR MESSAGE");

    ret = wdb_parse_vuln_cves(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Cannot execute vuln_cves clear command; SQL err: ERROR MESSAGE");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_vuln_cves_clear_command_success(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("clear", query);

    // wdb_parse_agents_clear_vuln_cves
    will_return(__wrap_wdb_agents_clear_vuln_cves, OS_SUCCESS);

    ret = wdb_parse_vuln_cves(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

/* wdb_parse_packages */

/* get */

void test_packages_get_success(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    char* result = NULL;
    os_strdup("get", query);
    os_strdup("[{\"status\":\"SUCCESS\"}]", result);
    cJSON *test =  cJSON_CreateObject();

    expect_value(__wrap_wdb_agents_get_packages, not_triaged_only, FALSE);
    will_return(__wrap_wdb_agents_get_packages, test);
    will_return(__wrap_wdb_agents_get_packages, OS_SUCCESS);
    will_return(__wrap_cJSON_PrintUnformatted, result);

    ret = wdb_parse_packages(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok [{\"status\":\"SUCCESS\"}]");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_packages_get_not_triaged_success(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    char* result = NULL;
    os_strdup("get not_triaged", query);
    os_strdup("[{\"status\":\"SUCCESS\"}]", result);
    cJSON *test =  cJSON_CreateObject();

    expect_value(__wrap_wdb_agents_get_packages, not_triaged_only, TRUE);
    will_return(__wrap_wdb_agents_get_packages, test);
    will_return(__wrap_wdb_agents_get_packages, OS_SUCCESS);
    will_return(__wrap_cJSON_PrintUnformatted, result);

    ret = wdb_parse_packages(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok [{\"status\":\"SUCCESS\"}]");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_packages_get_null_response(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("get", query);

    expect_value(__wrap_wdb_agents_get_packages, not_triaged_only, FALSE);
    will_return(__wrap_wdb_agents_get_packages, NULL);
    will_return(__wrap_wdb_agents_get_packages, OS_SUCCESS);
    expect_string(__wrap__mdebug1, formatted_msg, "Error getting packages from sys_programs");

    ret = wdb_parse_packages(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Error getting packages from sys_programs");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_packages_get_err_response(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    char* result = NULL;
    os_strdup("get", query);
    os_strdup("[{\"status\":\"ERROR\"}]", result);
    cJSON *test =  cJSON_CreateObject();

    expect_value(__wrap_wdb_agents_get_packages, not_triaged_only, FALSE);
    will_return(__wrap_wdb_agents_get_packages, test);
    will_return(__wrap_wdb_agents_get_packages, OS_INVALID);
    will_return(__wrap_cJSON_PrintUnformatted, result);

    ret = wdb_parse_packages(data->wdb, query, data->output);

    assert_string_equal(data->output, "err [{\"status\":\"ERROR\"}]");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_packages_get_sock_err_response(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    char* result = NULL;
    os_strdup("get", query);
    os_strdup("[{\"status\":\"ERROR\"}]", result);
    cJSON *test =  cJSON_CreateObject();

    expect_value(__wrap_wdb_agents_get_packages, not_triaged_only, FALSE);
    will_return(__wrap_wdb_agents_get_packages, test);
    will_return(__wrap_wdb_agents_get_packages, OS_SOCKTERR);
    will_return(__wrap_cJSON_PrintUnformatted, result);

    ret = wdb_parse_packages(data->wdb, query, data->output);

    assert_string_equal(data->output, "");
    assert_int_equal(ret, OS_SOCKTERR);

    os_free(query);
}

/* save */

void test_packages_save_success(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("save 0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15", query);

    expect_string(__wrap_wdb_package_save, scan_id, "0");
    expect_string(__wrap_wdb_package_save, scan_time, "1");
    expect_string(__wrap_wdb_package_save, format, "2");
    expect_string(__wrap_wdb_package_save, name, "3");
    expect_string(__wrap_wdb_package_save, priority, "4");
    expect_string(__wrap_wdb_package_save, section, "5");
    expect_value(__wrap_wdb_package_save, size, 6);
    expect_string(__wrap_wdb_package_save, vendor, "7");
    expect_string(__wrap_wdb_package_save, install_time, "8");
    expect_string(__wrap_wdb_package_save, version, "9");
    expect_string(__wrap_wdb_package_save, architecture, "10");
    expect_string(__wrap_wdb_package_save, multiarch, "11");
    expect_string(__wrap_wdb_package_save, source, "12");
    expect_string(__wrap_wdb_package_save, description, "13");
    expect_string(__wrap_wdb_package_save, location, "14");
    expect_string(__wrap_wdb_package_save, checksum, SYSCOLLECTOR_LEGACY_CHECKSUM_VALUE);
    expect_string(__wrap_wdb_package_save, item_id, "15");
    expect_value(__wrap_wdb_package_save, replace, FALSE);
    will_return(__wrap_wdb_package_save, OS_SUCCESS);

    will_return(__wrap_time, 0);
    expect_value(__wrap_wdbi_update_attempt, component, WDB_SYSCOLLECTOR_PACKAGES);
    expect_value(__wrap_wdbi_update_attempt, timestamp, 0);
    expect_value(__wrap_wdbi_update_attempt, legacy, TRUE);
    expect_string(__wrap_wdbi_update_attempt, last_agent_checksum, "");
    expect_string(__wrap_wdbi_update_attempt, manager_checksum, "");

    ret = wdb_parse_packages(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_packages_save_success_null_items(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("save 0|1|2|3|4|5|NULL|7|8|9|10|11|12|13|NULL|NULL", query);

    expect_string(__wrap_wdb_package_save, scan_id, "0");
    expect_string(__wrap_wdb_package_save, scan_time, "1");
    expect_string(__wrap_wdb_package_save, format, "2");
    expect_string(__wrap_wdb_package_save, name, "3");
    expect_string(__wrap_wdb_package_save, priority, "4");
    expect_string(__wrap_wdb_package_save, section, "5");
    expect_value(__wrap_wdb_package_save, size, -1);
    expect_string(__wrap_wdb_package_save, vendor, "7");
    expect_string(__wrap_wdb_package_save, install_time, "8");
    expect_string(__wrap_wdb_package_save, version, "9");
    expect_string(__wrap_wdb_package_save, architecture, "10");
    expect_string(__wrap_wdb_package_save, multiarch, "11");
    expect_string(__wrap_wdb_package_save, source, "12");
    expect_string(__wrap_wdb_package_save, description, "13");
    expect_value (__wrap_wdb_package_save, location, NULL);
    expect_string(__wrap_wdb_package_save, checksum, SYSCOLLECTOR_LEGACY_CHECKSUM_VALUE);
    expect_value(__wrap_wdb_package_save, item_id, NULL);
    expect_value(__wrap_wdb_package_save, replace, FALSE);
    will_return(__wrap_wdb_package_save, OS_SUCCESS);

    will_return(__wrap_time, 0);
    expect_value(__wrap_wdbi_update_attempt, component, WDB_SYSCOLLECTOR_PACKAGES);
    expect_value(__wrap_wdbi_update_attempt, timestamp, 0);
    expect_value(__wrap_wdbi_update_attempt, legacy, TRUE);
    expect_string(__wrap_wdbi_update_attempt, last_agent_checksum, "");
    expect_string(__wrap_wdbi_update_attempt, manager_checksum, "");

    ret = wdb_parse_packages(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_packages_save_success_empty_items(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("save |1|2|3||5|6|7||9|10|11||13|14|", query);

    expect_string(__wrap_wdb_package_save, scan_id, "");
    expect_string(__wrap_wdb_package_save, scan_time, "1");
    expect_string(__wrap_wdb_package_save, format, "2");
    expect_string(__wrap_wdb_package_save, name, "3");
    expect_string(__wrap_wdb_package_save, priority, "");
    expect_string(__wrap_wdb_package_save, section, "5");
    expect_value(__wrap_wdb_package_save, size, 6);
    expect_string(__wrap_wdb_package_save, vendor, "7");
    expect_string(__wrap_wdb_package_save, install_time, "");
    expect_string(__wrap_wdb_package_save, version, "9");
    expect_string(__wrap_wdb_package_save, architecture, "10");
    expect_string(__wrap_wdb_package_save, multiarch, "11");
    expect_string(__wrap_wdb_package_save, source, "");
    expect_string(__wrap_wdb_package_save, description, "13");
    expect_string(__wrap_wdb_package_save, location, "14");
    expect_string(__wrap_wdb_package_save, checksum, SYSCOLLECTOR_LEGACY_CHECKSUM_VALUE);
    expect_string(__wrap_wdb_package_save, item_id, "");
    expect_value(__wrap_wdb_package_save, replace, FALSE);
    will_return(__wrap_wdb_package_save, OS_SUCCESS);

    will_return(__wrap_time, 0);
    expect_value(__wrap_wdbi_update_attempt, component, WDB_SYSCOLLECTOR_PACKAGES);
    expect_value(__wrap_wdbi_update_attempt, timestamp, 0);
    expect_value(__wrap_wdbi_update_attempt, legacy, TRUE);
    expect_string(__wrap_wdbi_update_attempt, last_agent_checksum, "");
    expect_string(__wrap_wdbi_update_attempt, manager_checksum, "");

    ret = wdb_parse_packages(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_packages_save_missing_items(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("save 0|1|2|3|4|5|6|7|8|9|10|11|12|13|14", query);

    expect_string(__wrap__mdebug1, formatted_msg, "Invalid package info query syntax.");
    expect_string(__wrap__mdebug2, formatted_msg,  "Package info query: 14");

    ret = wdb_parse_packages(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid package info query syntax, near '14'");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_packages_save_err(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("save 0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15", query);

    expect_string(__wrap_wdb_package_save, scan_id, "0");
    expect_string(__wrap_wdb_package_save, scan_time, "1");
    expect_string(__wrap_wdb_package_save, format, "2");
    expect_string(__wrap_wdb_package_save, name, "3");
    expect_string(__wrap_wdb_package_save, priority, "4");
    expect_string(__wrap_wdb_package_save, section, "5");
    expect_value(__wrap_wdb_package_save, size, 6);
    expect_string(__wrap_wdb_package_save, vendor, "7");
    expect_string(__wrap_wdb_package_save, install_time, "8");
    expect_string(__wrap_wdb_package_save, version, "9");
    expect_string(__wrap_wdb_package_save, architecture, "10");
    expect_string(__wrap_wdb_package_save, multiarch, "11");
    expect_string(__wrap_wdb_package_save, source, "12");
    expect_string(__wrap_wdb_package_save, description, "13");
    expect_string(__wrap_wdb_package_save, location, "14");
    expect_string(__wrap_wdb_package_save, checksum, SYSCOLLECTOR_LEGACY_CHECKSUM_VALUE);
    expect_string(__wrap_wdb_package_save, item_id, "15");
    expect_value(__wrap_wdb_package_save, replace, FALSE);
    will_return(__wrap_wdb_package_save, OS_INVALID);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot save package information.");

    ret = wdb_parse_packages(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Cannot save package information.");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

/* del */

void test_packages_del_success(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("del 0", query);

    expect_string(__wrap_wdb_package_update, scan_id, "0");
    will_return(__wrap_wdb_package_update, OS_SUCCESS);

    expect_string(__wrap_wdb_package_delete, scan_id, "0");
    will_return(__wrap_wdb_package_delete, OS_SUCCESS);

    will_return(__wrap_time, 0);
    expect_value(__wrap_wdbi_update_completion, component, WDB_SYSCOLLECTOR_PACKAGES);
    expect_value(__wrap_wdbi_update_completion, timestamp, 0);
    expect_string(__wrap_wdbi_update_completion, last_agent_checksum, "");
    expect_string(__wrap_wdbi_update_completion, manager_checksum, "");

    ret = wdb_parse_packages(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_packages_del_success_null_items(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("del NULL", query);

    expect_value(__wrap_wdb_package_update, scan_id, NULL);
    will_return(__wrap_wdb_package_update, OS_SUCCESS);

    expect_value(__wrap_wdb_package_delete, scan_id, NULL);
    will_return(__wrap_wdb_package_delete, OS_SUCCESS);

    will_return(__wrap_time, 0);
    expect_value(__wrap_wdbi_update_completion, component, WDB_SYSCOLLECTOR_PACKAGES);
    expect_value(__wrap_wdbi_update_completion, timestamp, 0);
    expect_string(__wrap_wdbi_update_completion, last_agent_checksum, "");
    expect_string(__wrap_wdbi_update_completion, manager_checksum, "");

    ret = wdb_parse_packages(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_packages_del_update_err(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("del 0", query);

    expect_string(__wrap_wdb_package_update, scan_id, "0");
    will_return(__wrap_wdb_package_update, OS_INVALID);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot update scanned packages.");

    expect_string(__wrap_wdb_package_delete, scan_id, "0");
    will_return(__wrap_wdb_package_delete, OS_SUCCESS);

    will_return(__wrap_time, 0);
    expect_value(__wrap_wdbi_update_completion, component, WDB_SYSCOLLECTOR_PACKAGES);
    expect_value(__wrap_wdbi_update_completion, timestamp, 0);
    expect_string(__wrap_wdbi_update_completion, last_agent_checksum, "");
    expect_string(__wrap_wdbi_update_completion, manager_checksum, "");

    ret = wdb_parse_packages(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_packages_del_delete_err(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("del 0", query);

    expect_string(__wrap_wdb_package_update, scan_id, "0");
    will_return(__wrap_wdb_package_update, OS_SUCCESS);

    expect_string(__wrap_wdb_package_delete, scan_id, "0");
    will_return(__wrap_wdb_package_delete, OS_INVALID);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot delete old package information.");

    ret = wdb_parse_packages(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Cannot delete old package information.");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

/* invalid action */

void test_packages_invalid_action(void **state) {

    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("invalid", query);

    expect_string(__wrap__mdebug1, formatted_msg, "Invalid package info query syntax.");
    expect_string(__wrap__mdebug2, formatted_msg, "DB query error near: invalid");

    ret = wdb_parse_packages(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid package info query syntax, near 'invalid'");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_packages_no_action(void **state) {

    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("", query);

    expect_string(__wrap__mdebug1, formatted_msg, "Invalid package info query syntax. Missing action");
    expect_string(__wrap__mdebug2, formatted_msg, "DB query error. Missing action");

    ret = wdb_parse_packages(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid package info query syntax. Missing action");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}


/* wdb_parse_hotfixes */

/* get */

void test_hotfixes_get_success(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    char* result = NULL;
    os_strdup("get", query);
    os_strdup("[{\"status\":\"SUCCESS\"}]", result);
    cJSON *test =  cJSON_CreateObject();

    will_return(__wrap_wdb_agents_get_hotfixes, test);
    will_return(__wrap_wdb_agents_get_hotfixes, OS_SUCCESS);
    will_return(__wrap_cJSON_PrintUnformatted, result);

    ret = wdb_parse_hotfixes(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok [{\"status\":\"SUCCESS\"}]");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_hotfixes_get_null_response(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("get", query);

    will_return(__wrap_wdb_agents_get_hotfixes, NULL);
    will_return(__wrap_wdb_agents_get_hotfixes, OS_SUCCESS);
    expect_string(__wrap__mdebug1, formatted_msg, "Error getting hotfixes from sys_hotfixes");

    ret = wdb_parse_hotfixes(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Error getting hotfixes from sys_hotfixes");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_hotfixes_get_err_response(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    char* result = NULL;
    os_strdup("get", query);
    os_strdup("[{\"status\":\"ERROR\"}]", result);
    cJSON *test =  cJSON_CreateObject();

    will_return(__wrap_wdb_agents_get_hotfixes, test);
    will_return(__wrap_wdb_agents_get_hotfixes, OS_INVALID);
    will_return(__wrap_cJSON_PrintUnformatted, result);

    ret = wdb_parse_hotfixes(data->wdb, query, data->output);

    assert_string_equal(data->output, "err [{\"status\":\"ERROR\"}]");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_hotfixes_get_sock_err_response(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    char* result = NULL;
    os_strdup("get", query);
    os_strdup("[{\"status\":\"ERROR\"}]", result);
    cJSON *test =  cJSON_CreateObject();

    will_return(__wrap_wdb_agents_get_hotfixes, test);
    will_return(__wrap_wdb_agents_get_hotfixes, OS_SOCKTERR);
    will_return(__wrap_cJSON_PrintUnformatted, result);

    ret = wdb_parse_hotfixes(data->wdb, query, data->output);

    assert_string_equal(data->output, "");
    assert_int_equal(ret, OS_SOCKTERR);

    os_free(query);
}

/* save */

void test_hotfixes_save_success(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("save 0|1|2", query);

    expect_string(__wrap_wdb_hotfix_save, scan_id, "0");
    expect_string(__wrap_wdb_hotfix_save, scan_time, "1");
    expect_string(__wrap_wdb_hotfix_save, hotfix, "2");
    expect_string(__wrap_wdb_hotfix_save, checksum, SYSCOLLECTOR_LEGACY_CHECKSUM_VALUE);
    expect_value(__wrap_wdb_hotfix_save, replace, FALSE);
    will_return(__wrap_wdb_hotfix_save, OS_SUCCESS);

    will_return(__wrap_time, 0);
    expect_value(__wrap_wdbi_update_attempt, component, WDB_SYSCOLLECTOR_HOTFIXES);
    expect_value(__wrap_wdbi_update_attempt, timestamp, 0);
    expect_value(__wrap_wdbi_update_attempt, legacy, TRUE);
    expect_string(__wrap_wdbi_update_attempt, last_agent_checksum, "");
    expect_string(__wrap_wdbi_update_attempt, manager_checksum, "");

    ret = wdb_parse_hotfixes(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_hotfixes_save_success_null_items(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("save 0|NULL|2", query);

    expect_string(__wrap_wdb_hotfix_save, scan_id, "0");
    expect_value(__wrap_wdb_hotfix_save, scan_time, NULL);
    expect_string(__wrap_wdb_hotfix_save, hotfix, "2");
    expect_string(__wrap_wdb_hotfix_save, checksum, SYSCOLLECTOR_LEGACY_CHECKSUM_VALUE);
    expect_value(__wrap_wdb_hotfix_save, replace, FALSE);
    will_return(__wrap_wdb_hotfix_save, OS_SUCCESS);

    will_return(__wrap_time, 0);
    expect_value(__wrap_wdbi_update_attempt, component, WDB_SYSCOLLECTOR_HOTFIXES);
    expect_value(__wrap_wdbi_update_attempt, timestamp, 0);
    expect_value(__wrap_wdbi_update_attempt, legacy, TRUE);
    expect_string(__wrap_wdbi_update_attempt, last_agent_checksum, "");
    expect_string(__wrap_wdbi_update_attempt, manager_checksum, "");

    ret = wdb_parse_hotfixes(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_hotfixes_save_missing_items(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("save 0|1", query);

    expect_string(__wrap__mdebug1, formatted_msg, "Invalid hotfix info query syntax.");
    expect_string(__wrap__mdebug2, formatted_msg,  "Hotfix info query: 1");

    ret = wdb_parse_hotfixes(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid hotfix info query syntax, near '1'");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_hotfixes_save_err(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("save 0|1|2", query);

    expect_string(__wrap_wdb_hotfix_save, scan_id, "0");
    expect_string(__wrap_wdb_hotfix_save, scan_time, "1");
    expect_string(__wrap_wdb_hotfix_save, hotfix, "2");
    expect_string(__wrap_wdb_hotfix_save, checksum, SYSCOLLECTOR_LEGACY_CHECKSUM_VALUE);
    expect_value(__wrap_wdb_hotfix_save, replace, FALSE);
    will_return(__wrap_wdb_hotfix_save, OS_INVALID);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot save hotfix information.");

    ret = wdb_parse_hotfixes(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Cannot save hotfix information.");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

/* del */

void test_hotfixes_del_success(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("del 0", query);

    expect_string(__wrap_wdb_hotfix_delete, scan_id, "0");
    will_return(__wrap_wdb_hotfix_delete, OS_SUCCESS);

    will_return(__wrap_time, 0);
    expect_value(__wrap_wdbi_update_completion, component, WDB_SYSCOLLECTOR_HOTFIXES);
    expect_value(__wrap_wdbi_update_completion, timestamp, 0);
    expect_string(__wrap_wdbi_update_completion, last_agent_checksum, "");
    expect_string(__wrap_wdbi_update_completion, manager_checksum, "");

    ret = wdb_parse_hotfixes(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_hotfixes_del_success_null_items(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("del NULL", query);

    expect_value(__wrap_wdb_hotfix_delete, scan_id, NULL);
    will_return(__wrap_wdb_hotfix_delete, OS_SUCCESS);

    will_return(__wrap_time, 0);
    expect_value(__wrap_wdbi_update_completion, component, WDB_SYSCOLLECTOR_HOTFIXES);
    expect_value(__wrap_wdbi_update_completion, timestamp, 0);
    expect_string(__wrap_wdbi_update_completion, last_agent_checksum, "");
    expect_string(__wrap_wdbi_update_completion, manager_checksum, "");

    ret = wdb_parse_hotfixes(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_hotfixes_del_delete_err(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("del 0", query);

    expect_string(__wrap_wdb_hotfix_delete, scan_id, "0");
    will_return(__wrap_wdb_hotfix_delete, OS_INVALID);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot delete old hotfix information.");

    ret = wdb_parse_hotfixes(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Cannot delete old hotfix information.");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

/* invalid action */

void test_hotfixes_invalid_action(void **state) {

    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("invalid", query);

    expect_string(__wrap__mdebug1, formatted_msg, "Invalid hotfix info query syntax.");
    expect_string(__wrap__mdebug2, formatted_msg, "DB query error near: invalid");

    ret = wdb_parse_hotfixes(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid hotfix info query syntax, near 'invalid'");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_hotfixes_no_action(void **state) {

    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char* query = NULL;
    os_strdup("", query);

    expect_string(__wrap__mdebug1, formatted_msg, "Invalid hotfix info query syntax. Missing action");
    expect_string(__wrap__mdebug2, formatted_msg, "DB query error. Missing action");

    ret = wdb_parse_hotfixes(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid hotfix info query syntax. Missing action");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_dbsync_insert_fail_0_arguments(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("clear", query);

    expect_string(__wrap__mdebug2, formatted_msg, "DBSYNC query: clear");

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid dbsync query syntax, near 'clear'");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_dbsync_insert_fail_1_arguments(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("os_info clear", query);

    expect_string(__wrap__mdebug2, formatted_msg, "DBSYNC query: os_info");

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid dbsync query syntax, near 'os_info'");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_dbsync_insert_fail_2_arguments(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("os_info INSERTED ", query);

    expect_string(__wrap__mdebug2, formatted_msg, "DBSYNC query: os_info");

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "err Invalid dbsync query syntax, near 'os_info'");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_dbsync_insert_type_not_exists(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("os_que INSERTED data?", query);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "err");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_dbsync_insert_type_exists_data_incorrect(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;
    char error_message[128] = { "\0" };

    os_strdup("hotfixes INSERTED data?", query);
    sprintf(error_message, DB_INVALID_DELTA_MSG, "data?", 3ul, 0ul);

    expect_string(__wrap__merror, formatted_msg, error_message);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "err");
    assert_int_equal(ret, OS_INVALID);

    os_free(query);
}

void test_dbsync_insert_type_exists_data_correct(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("hotfixes INSERTED data1|data2|data3|", query);

    will_return(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_int, index, 1);
    expect_value(__wrap_sqlite3_bind_int, value, 0);
    will_return_always(__wrap_sqlite3_bind_int, SQLITE_OK);

    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");

    expect_value(__wrap_sqlite3_bind_text, pos, 3);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");

    expect_value(__wrap_sqlite3_bind_text, pos, 4);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data3");

    will_return_always(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok ");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_dbsync_delete_type_exists_data_1(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("hotfixes DELETED NULL|data5|NULL|", query);

    will_return(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data5");
    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);

    will_return(__wrap_wdb_get_cache_stmt, 1);
    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data5");
    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);
    will_return(__wrap_sqlite3_changes, 1);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok ");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_dbsync_modify_type_exists_data_1(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("hotfixes MODIFIED data1|data2|data3|", query);

    will_return_always(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data3");
    expect_value(__wrap_sqlite3_bind_text, pos, 3);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");

    will_return_always(__wrap_sqlite3_bind_text, SQLITE_OK);
    will_return(__wrap_sqlite3_changes, 1);

    will_return(__wrap_wdb_step, SQLITE_DONE);
    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");

    will_return(__wrap_wdb_step, SQLITE_DONE);
    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok ");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_dbsync_insert_type_exists_null_stmt(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("hotfixes INSERTED data?|data2?|data3?|", query);

    will_return(__wrap_wdb_get_cache_stmt, 0);

    expect_string(__wrap__merror, formatted_msg, DB_CACHE_NULL_STMT);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "err");
    assert_int_equal(ret, OS_NOTFOUND);

    os_free(query);
}

void test_dbsync_delete_type_exists_data_compound_pk(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("network_protocol DELETED data1|data2|NULL|NULL|NULL|NULL|NULL|", query);

    will_return(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");

    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);
    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);

    will_return(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");
    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);
    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);
    will_return(__wrap_sqlite3_changes, 1);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok ");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_dbsync_modify_type_exists_data_real_value(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("hwinfo MODIFIED data1|data2|data3|NULL|NULL|NULL|NULL|NULL|NULL|", query);

    will_return_always(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data3");
    expect_value(__wrap_sqlite3_bind_text, pos, 3);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");

    will_return_always(__wrap_sqlite3_bind_text, SQLITE_OK);
    will_return(__wrap_sqlite3_changes, 1);

    will_return(__wrap_wdb_step, SQLITE_DONE);
    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");

    will_return(__wrap_wdb_step, SQLITE_ROW);

    expect_value(__wrap_sqlite3_column_text, iCol, 0);
    will_return(__wrap_sqlite3_column_text, "data1");
    expect_value(__wrap_sqlite3_column_text, iCol, 1);
    will_return(__wrap_sqlite3_column_text, "data2");
    expect_value(__wrap_sqlite3_column_text, iCol, 2);
    will_return(__wrap_sqlite3_column_text, "data3");
    expect_value(__wrap_sqlite3_column_text, iCol, 3);
    will_return(__wrap_sqlite3_column_text, "4");
    expect_value(__wrap_sqlite3_column_text, iCol, 4);
    will_return(__wrap_sqlite3_column_text, "5.0");
    expect_value(__wrap_sqlite3_column_text, iCol, 5);
    will_return(__wrap_sqlite3_column_text, "6");
    expect_value(__wrap_sqlite3_column_text, iCol, 6);
    will_return(__wrap_sqlite3_column_text, "7");
    expect_value(__wrap_sqlite3_column_text, iCol, 7);
    will_return(__wrap_sqlite3_column_text, "8");
    expect_value(__wrap_sqlite3_column_text, iCol, 8);
    will_return(__wrap_sqlite3_column_text, "data9");

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok data1|data2|data3|4|5.0|6|7|8|data9|");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_dbsync_modify_type_exists_data_integer_null_value(void ** state) {
    int ret = -1;
    test_struct_t * data = (test_struct_t *) *state;
    char * query = NULL;

    os_strdup("hwinfo MODIFIED data1|data2|data3||NULL|NULL|NULL|NULL|NULL|", query);

    will_return_always(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data3");
    expect_value(__wrap_sqlite3_bind_null, index, 3);
    will_return(__wrap_sqlite3_bind_null, SQLITE_OK);
    expect_value(__wrap_sqlite3_bind_text, pos, 4);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");

    will_return_always(__wrap_sqlite3_bind_text, SQLITE_OK);
    will_return(__wrap_sqlite3_changes, 1);

    will_return(__wrap_wdb_step, SQLITE_DONE);
    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");

    will_return(__wrap_wdb_step, SQLITE_ROW);

    expect_value(__wrap_sqlite3_column_text, iCol, 0);
    will_return(__wrap_sqlite3_column_text, "data1");
    expect_value(__wrap_sqlite3_column_text, iCol, 1);
    will_return(__wrap_sqlite3_column_text, "data2");
    expect_value(__wrap_sqlite3_column_text, iCol, 2);
    will_return(__wrap_sqlite3_column_text, "data3");
    expect_value(__wrap_sqlite3_column_text, iCol, 3);
    will_return(__wrap_sqlite3_column_text, NULL);
    expect_value(__wrap_sqlite3_column_text, iCol, 4);
    will_return(__wrap_sqlite3_column_text, "5.0");
    expect_value(__wrap_sqlite3_column_text, iCol, 5);
    will_return(__wrap_sqlite3_column_text, "6");
    expect_value(__wrap_sqlite3_column_text, iCol, 6);
    will_return(__wrap_sqlite3_column_text, "7");
    expect_value(__wrap_sqlite3_column_text, iCol, 7);
    will_return(__wrap_sqlite3_column_text, "8");
    expect_value(__wrap_sqlite3_column_text, iCol, 8);
    will_return(__wrap_sqlite3_column_text, "data9");

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok data1|data2|data3||5.0|6|7|8|data9|");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_dbsync_modify_type_exists_data_float_null_value(void ** state) {
    int ret = -1;
    test_struct_t * data = (test_struct_t *) *state;
    char * query = NULL;

    os_strdup("hwinfo MODIFIED data1|data2|data3|NULL||NULL|NULL|NULL|NULL|", query);

    will_return_always(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data3");
    expect_value(__wrap_sqlite3_bind_null, index, 3);
    will_return(__wrap_sqlite3_bind_null, SQLITE_OK);
    expect_value(__wrap_sqlite3_bind_text, pos, 4);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");

    will_return_always(__wrap_sqlite3_bind_text, SQLITE_OK);
    will_return(__wrap_sqlite3_changes, 1);

    will_return(__wrap_wdb_step, SQLITE_DONE);
    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");

    will_return(__wrap_wdb_step, SQLITE_ROW);

    expect_value(__wrap_sqlite3_column_text, iCol, 0);
    will_return(__wrap_sqlite3_column_text, "data1");
    expect_value(__wrap_sqlite3_column_text, iCol, 1);
    will_return(__wrap_sqlite3_column_text, "data2");
    expect_value(__wrap_sqlite3_column_text, iCol, 2);
    will_return(__wrap_sqlite3_column_text, "data3");
    expect_value(__wrap_sqlite3_column_text, iCol, 3);
    will_return(__wrap_sqlite3_column_text, "4");
    expect_value(__wrap_sqlite3_column_text, iCol, 4);
    will_return(__wrap_sqlite3_column_text, NULL);
    expect_value(__wrap_sqlite3_column_text, iCol, 5);
    will_return(__wrap_sqlite3_column_text, "6");
    expect_value(__wrap_sqlite3_column_text, iCol, 6);
    will_return(__wrap_sqlite3_column_text, "7");
    expect_value(__wrap_sqlite3_column_text, iCol, 7);
    will_return(__wrap_sqlite3_column_text, "8");
    expect_value(__wrap_sqlite3_column_text, iCol, 8);
    will_return(__wrap_sqlite3_column_text, "data9");

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok data1|data2|data3|4||6|7|8|data9|");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_dbsync_modify_type_exists_data_text_null_value(void ** state) {
    int ret = -1;
    test_struct_t * data = (test_struct_t *) *state;
    char * query = NULL;

    os_strdup("hwinfo MODIFIED data1|data2||NULL|NULL|NULL|NULL|NULL|NULL|", query);

    will_return_always(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "");
    expect_value(__wrap_sqlite3_bind_text, pos, 3);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");

    will_return_always(__wrap_sqlite3_bind_text, SQLITE_OK);
    will_return(__wrap_sqlite3_changes, 1);

    will_return(__wrap_wdb_step, SQLITE_DONE);
    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");

    will_return(__wrap_wdb_step, SQLITE_ROW);

    expect_value(__wrap_sqlite3_column_text, iCol, 0);
    will_return(__wrap_sqlite3_column_text, "data1");
    expect_value(__wrap_sqlite3_column_text, iCol, 1);
    will_return(__wrap_sqlite3_column_text, NULL);
    expect_value(__wrap_sqlite3_column_text, iCol, 2);
    will_return(__wrap_sqlite3_column_text, "data3");
    expect_value(__wrap_sqlite3_column_text, iCol, 3);
    will_return(__wrap_sqlite3_column_text, "4");
    expect_value(__wrap_sqlite3_column_text, iCol, 4);
    will_return(__wrap_sqlite3_column_text, "5.0");
    expect_value(__wrap_sqlite3_column_text, iCol, 5);
    will_return(__wrap_sqlite3_column_text, "6");
    expect_value(__wrap_sqlite3_column_text, iCol, 6);
    will_return(__wrap_sqlite3_column_text, "7");
    expect_value(__wrap_sqlite3_column_text, iCol, 7);
    will_return(__wrap_sqlite3_column_text, "8");
    expect_value(__wrap_sqlite3_column_text, iCol, 8);
    will_return(__wrap_sqlite3_column_text, "data9");

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok data1||data3|4|5.0|6|7|8|data9|");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_dbsync_modify_type_exists_data_compound_pk(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("network_protocol MODIFIED data1|data2|data3|NULL|NULL|NULL|NULL|", query);

    will_return_always(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data3");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 3);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");

    will_return_always(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);
    will_return(__wrap_sqlite3_changes, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");


    will_return(__wrap_wdb_step, SQLITE_ROW);

    expect_value(__wrap_sqlite3_column_text, iCol, 0);
    will_return(__wrap_sqlite3_column_text, "data1");
    expect_value(__wrap_sqlite3_column_text, iCol, 1);
    will_return(__wrap_sqlite3_column_text, "data2");
    expect_value(__wrap_sqlite3_column_text, iCol, 2);
    will_return(__wrap_sqlite3_column_text, "data3");
    expect_value(__wrap_sqlite3_column_text, iCol, 3);
    will_return(__wrap_sqlite3_column_text, "data4");
    expect_value(__wrap_sqlite3_column_text, iCol, 4);
    will_return(__wrap_sqlite3_column_text, "5");
    expect_value(__wrap_sqlite3_column_text, iCol, 5);
    will_return(__wrap_sqlite3_column_text, "data6");
    expect_value(__wrap_sqlite3_column_text, iCol, 6);
    will_return(__wrap_sqlite3_column_text, "data7");

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok data1|data2|data3|data4|5|data6|data7|");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_dbsync_modify_type_exists_data_stmt_fail(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("network_protocol MODIFIED data1|data2|data3|NULL|NULL|NULL|NULL|", query);

    will_return_always(__wrap_wdb_get_cache_stmt, 0);

    expect_string(__wrap__merror, formatted_msg, DB_CACHE_NULL_STMT);
    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "err");
    assert_int_equal(ret, OS_NOTFOUND);

    os_free(query);
}

void test_dbsync_delete_type_exists_data_select_stmt_fail(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("network_protocol DELETED data1|data2|NULL|NULL|NULL|NULL|NULL|", query);

    will_return(__wrap_wdb_get_cache_stmt, 0);

    will_return(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");
    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);
    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);
    will_return(__wrap_sqlite3_changes, 1);

    expect_string(__wrap__merror, formatted_msg, DB_CACHE_NULL_STMT);
    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok ");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_dbsync_delete_type_exists_data_stmt_fail(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("network_protocol DELETED data1|data2|NULL|NULL|NULL|NULL|NULL|", query);

    will_return(__wrap_wdb_get_cache_stmt, 0);
    expect_string(__wrap__merror, formatted_msg, DB_CACHE_NULL_STMT);

    will_return(__wrap_wdb_get_cache_stmt, 0);
    expect_string(__wrap__merror, formatted_msg, DB_CACHE_NULL_STMT);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "err");
    assert_int_equal(ret, OS_NOTFOUND);

    os_free(query);
}

void test_dbsync_delete_type_exists_data_bind_error(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;
    const char error_value[] = { "trc" };
    char error_message[128] = { "\0" };

    sprintf(error_message, DB_AGENT_SQL_ERROR, "000", error_value);

    os_strdup("osinfo DELETED NULL|NULL|NULL|data5|NULL|NULL|NULL|NULL|NULL|NULL|NULL|NULL|NULL|NULL|NULL|NULL|NULL|NULL|NULL|", query);

    will_return(__wrap_wdb_get_cache_stmt, 1);
    will_return_always(__wrap_sqlite3_errmsg, error_value);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data5");
    will_return(__wrap_sqlite3_bind_text, SQLITE_ERROR);

    expect_string(__wrap__merror, formatted_msg, error_message);
    will_return(__wrap_wdb_step, SQLITE_DONE);

    will_return(__wrap_wdb_get_cache_stmt, 1);
    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data5");
    will_return(__wrap_sqlite3_bind_text, SQLITE_ERROR);

    expect_string(__wrap__merror, formatted_msg, error_message);
    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "err");
    assert_int_equal(ret, OS_NOTFOUND);

    os_free(query);
}

void test_dbsync_modify_type_exists_data_bind_error(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;
    const char error_value[] = { "trc" };
    char error_message[128] = { "\0" };

    sprintf(error_message, DB_AGENT_SQL_ERROR, "000", error_value);

    os_strdup("hwinfo MODIFIED data1|data2|data3|0|NULL|NULL|NULL|NULL|NULL|", query);

    will_return_always(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data3");
    expect_value(__wrap_sqlite3_bind_int, index, 3);
    expect_value(__wrap_sqlite3_bind_int, value, 0);

    will_return_always(__wrap_sqlite3_errmsg, error_value);

    expect_value(__wrap_sqlite3_bind_text, pos, 4);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");

    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);

    will_return_always(__wrap_sqlite3_bind_int, SQLITE_ERROR);
    will_return_always(__wrap_sqlite3_bind_text, SQLITE_ERROR);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "err");
    assert_int_equal(ret, OS_NOTFOUND);

    os_free(query);
}

void test_dbsync_delete_type_exists_data_bind_error_ports(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;
    const char error_value[] = { "trc" };
    char error_message[128] = { "\0" };

    sprintf(error_message, DB_AGENT_SQL_ERROR, "000", error_value);

    os_strdup("ports DELETED NULL|data1|data2|0|NULL|NULL|NULL|NULL|1|NULL|NULL|NULL|NULL|NULL|", query);

    will_return(__wrap_wdb_get_cache_stmt, 1);
    will_return_always(__wrap_sqlite3_errmsg, error_value);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");
    expect_value(__wrap_sqlite3_bind_int, index, 3);
    expect_value(__wrap_sqlite3_bind_int, value, 0);
    expect_value(__wrap_sqlite3_bind_int64, index, 4);
    expect_value(__wrap_sqlite3_bind_int64, value, 1);

    will_return_always(__wrap_sqlite3_bind_text, SQLITE_ERROR);
    will_return_always(__wrap_sqlite3_bind_int, SQLITE_ERROR);
    will_return_always(__wrap_sqlite3_bind_int64, SQLITE_ERROR);

    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    will_return(__wrap_wdb_step, SQLITE_DONE);

    will_return(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");
    expect_value(__wrap_sqlite3_bind_int, index, 3);
    expect_value(__wrap_sqlite3_bind_int, value, 0);
    expect_value(__wrap_sqlite3_bind_int64, index, 4);
    expect_value(__wrap_sqlite3_bind_int64, value, 1);

    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "err");
    assert_int_equal(ret, OS_NOTFOUND);

    os_free(query);
}

void test_dbsync_modify_type_exists_data_bind_error_ports(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;
    const char error_value[] = { "trc" };
    char error_message[128] = { "\0" };

    sprintf(error_message, DB_AGENT_SQL_ERROR, "000", error_value);

    os_strdup("ports MODIFIED MMM|data1|data2|0|NULL|NULL|NULL|NULL|1|NULL|NULL|NULL|NULL|NULL|", query);

    will_return_always(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "MMM");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 3);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");
    expect_value(__wrap_sqlite3_bind_int, index, 4);
    expect_value(__wrap_sqlite3_bind_int, value, 0);
    expect_value(__wrap_sqlite3_bind_int64, index, 5);
    expect_value(__wrap_sqlite3_bind_int64, value, 1);

    will_return_always(__wrap_sqlite3_errmsg, error_value);

    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);

    will_return_always(__wrap_sqlite3_bind_int, SQLITE_ERROR);
    will_return_always(__wrap_sqlite3_bind_text, SQLITE_ERROR);
    will_return_always(__wrap_sqlite3_bind_int64, SQLITE_ERROR);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "err");
    assert_int_equal(ret, OS_NOTFOUND);

    os_free(query);
}

void test_dbsync_delete_type_exists_data_compound_pk_select_data(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;
    const char error_value[] = { "trc" };
    char error_message[128] = { "\0" };

    sprintf(error_message, DB_AGENT_SQL_ERROR, "000", error_value);

    os_strdup("network_protocol DELETED data1|data2|NULL|NULL|NULL|NULL|NULL|", query);

    will_return(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");

    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);
    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_ROW);

    expect_value(__wrap_sqlite3_column_text, iCol, 0);
    will_return(__wrap_sqlite3_column_text, "data1");
    expect_value(__wrap_sqlite3_column_text, iCol, 1);
    will_return(__wrap_sqlite3_column_text, "data2");
    expect_value(__wrap_sqlite3_column_text, iCol, 2);
    will_return(__wrap_sqlite3_column_text, "data3");
    expect_value(__wrap_sqlite3_column_text, iCol, 3);
    will_return(__wrap_sqlite3_column_text, "data4");
    expect_value(__wrap_sqlite3_column_text, iCol, 4);
    will_return(__wrap_sqlite3_column_text, "5");
    expect_value(__wrap_sqlite3_column_text, iCol, 5);
    will_return(__wrap_sqlite3_column_text, "data6");
    expect_value(__wrap_sqlite3_column_text, iCol, 6);
    will_return(__wrap_sqlite3_column_text, "data7");

    will_return(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");
    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);
    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);
    will_return(__wrap_sqlite3_changes, 1);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok data1|data2|data3|data4|5|data6|data7|");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_dbsync_delete_type_exists_data_compound_pk_select_data_fail(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;
    const char error_value[] = { "trc" };
    char error_message[128] = { "\0" };

    sprintf(error_message, DB_AGENT_SQL_ERROR, "000", error_value);

    os_strdup("network_protocol DELETED data1|data2|NULL|NULL|NULL|NULL|NULL|", query);

    will_return(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");

    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);
    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_ERROR);

    will_return_always(__wrap_sqlite3_errmsg, error_value);
    expect_string(__wrap__merror, formatted_msg, error_message);

    will_return(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2");
    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);
    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);
    will_return(__wrap_sqlite3_changes, 1);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok ");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_dbsync_insert_type_exists_data_return_values(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;
    const char error_value[] = { "trc" };
    char error_message[128] = { "\0" };

    sprintf(error_message, DB_AGENT_SQL_ERROR, "000", error_value);

    os_strdup("ports INSERTED data?|data2?|data3?|4|NULL|NULL|NULL|NULL|NULL|NULL|NULL|NULL|NULL|NULL|", query);

    will_return(__wrap_wdb_get_cache_stmt, 1);

    will_return_always(__wrap_sqlite3_bind_int, SQLITE_ERROR);
    will_return_always(__wrap_sqlite3_bind_text, SQLITE_ERROR);
    will_return_always(__wrap_sqlite3_bind_int64, SQLITE_ERROR);

    expect_value(__wrap_sqlite3_bind_int, index, 1);
    expect_value(__wrap_sqlite3_bind_int, value, 0);

    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data?");

    expect_value(__wrap_sqlite3_bind_text, pos, 3);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data2?");

    expect_value(__wrap_sqlite3_bind_text, pos, 4);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data3?");

    expect_value(__wrap_sqlite3_bind_int, index, 5);
    expect_value(__wrap_sqlite3_bind_int, value, 4);

    expect_value(__wrap_sqlite3_bind_text, pos, 6);
    expect_string(__wrap_sqlite3_bind_text, buffer, "");

    expect_value(__wrap_sqlite3_bind_int, index, 7);
    expect_value(__wrap_sqlite3_bind_int, value, 0);

    expect_value(__wrap_sqlite3_bind_int, index, 8);
    expect_value(__wrap_sqlite3_bind_int, value, 0);

    expect_value(__wrap_sqlite3_bind_int, index, 9);
    expect_value(__wrap_sqlite3_bind_int, value, 0);

    expect_value(__wrap_sqlite3_bind_int64, index, 10);
    expect_value(__wrap_sqlite3_bind_int64, value, 0);

    expect_value(__wrap_sqlite3_bind_text, pos, 11);
    expect_string(__wrap_sqlite3_bind_text, buffer, "");

    expect_value(__wrap_sqlite3_bind_int, index, 12);
    expect_value(__wrap_sqlite3_bind_int, value, 0);

    expect_value(__wrap_sqlite3_bind_text, pos, 13);
    expect_string(__wrap_sqlite3_bind_text, buffer, "");

    expect_value(__wrap_sqlite3_bind_text, pos, 14);
    expect_string(__wrap_sqlite3_bind_text, buffer, "");

    expect_value(__wrap_sqlite3_bind_text, pos, 15);
    expect_string(__wrap_sqlite3_bind_text, buffer, "");

    will_return_always(__wrap_sqlite3_errmsg, error_value);

    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);
    expect_string(__wrap__merror, formatted_msg, error_message);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "err");
    assert_int_equal(ret, OS_NOTFOUND);

    os_free(query);
}


void test_dbsync_insert_type_exists_data_correct_null_value(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("hotfixes INSERTED data1|_NULL_|_NULL_|", query);

    will_return(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_int, index, 1);
    expect_value(__wrap_sqlite3_bind_int, value, 0);
    will_return_always(__wrap_sqlite3_bind_int, SQLITE_OK);

    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");

    expect_value(__wrap_sqlite3_bind_text, pos, 3);
    expect_string(__wrap_sqlite3_bind_text, buffer, "NULL");

    expect_value(__wrap_sqlite3_bind_text, pos, 4);
    expect_string(__wrap_sqlite3_bind_text, buffer, "NULL");

    will_return_always(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok ");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_dbsync_delete_type_exists_data_null_value(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("hotfixes DELETED NULL|_NULL_|NULL|", query);

    will_return(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "NULL");
    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);

    will_return(__wrap_wdb_get_cache_stmt, 1);
    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "NULL");
    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);
    will_return(__wrap_sqlite3_changes, 1);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok ");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_dbsync_modify_type_exists_data_null_value(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("hotfixes MODIFIED data1|_NULL_|_NULL_|", query);

    will_return_always(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "NULL");
    expect_value(__wrap_sqlite3_bind_text, pos, 3);
    expect_string(__wrap_sqlite3_bind_text, buffer, "NULL");

    will_return_always(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);
    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "NULL");

    will_return(__wrap_wdb_step, SQLITE_DONE);
    will_return(__wrap_sqlite3_changes, 1);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok ");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_dbsync_modify_type_exists_avoid_old_implementation(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("packages MODIFIED 2021/10/01 00:00:20|deb|test-wazuh-1|mandatory|NULL|NULL|NULL|NULL|1.1.1-2|all|NULL|NULL|NULL|NULL|NULL|NULL|NULL|AAAa61b68678180d2debd374df900daa6fe35d73|AAAe5ea454e47141b5c6a8afefd6bd08e87057f9|", query);

    will_return_always(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "2021/10/01 00:00:20");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "deb");
    expect_value(__wrap_sqlite3_bind_text, pos, 3);
    expect_string(__wrap_sqlite3_bind_text, buffer, "mandatory");
    expect_value(__wrap_sqlite3_bind_text, pos, 4);
    expect_string(__wrap_sqlite3_bind_text, buffer, "AAAa61b68678180d2debd374df900daa6fe35d73");
    expect_value(__wrap_sqlite3_bind_text, pos, 5);
    expect_string(__wrap_sqlite3_bind_text, buffer, "AAAe5ea454e47141b5c6a8afefd6bd08e87057f9");
    expect_value(__wrap_sqlite3_bind_text, pos, 6);
    expect_string(__wrap_sqlite3_bind_text, buffer, "test-wazuh-1");
    expect_value(__wrap_sqlite3_bind_text, pos, 7);
    expect_string(__wrap_sqlite3_bind_text, buffer, "1.1.1-2");
    expect_value(__wrap_sqlite3_bind_text, pos, 8);
    expect_string(__wrap_sqlite3_bind_text, buffer, "all");

    will_return(__wrap_sqlite3_changes, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "test-wazuh-1");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "1.1.1-2");
    expect_value(__wrap_sqlite3_bind_text, pos, 3);
    expect_string(__wrap_sqlite3_bind_text, buffer, "all");

    will_return_always(__wrap_wdb_step, SQLITE_DONE);
    will_return_always(__wrap_sqlite3_bind_text, SQLITE_OK);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok ");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}


void test_dbsync_insert_type_exists_data_correct_null_value_variant(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("hotfixes INSERTED data1|__NULL__|__NULL__|", query);

    will_return(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_int, index, 1);
    expect_value(__wrap_sqlite3_bind_int, value, 0);
    will_return_always(__wrap_sqlite3_bind_int, SQLITE_OK);

    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");

    expect_value(__wrap_sqlite3_bind_text, pos, 3);
    expect_string(__wrap_sqlite3_bind_text, buffer, "_NULL_");

    expect_value(__wrap_sqlite3_bind_text, pos, 4);
    expect_string(__wrap_sqlite3_bind_text, buffer, "_NULL_");

    will_return_always(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok ");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_dbsync_delete_type_exists_data_null_value_variant(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("hotfixes DELETED NULL|__NULL__|NULL|", query);

    will_return(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "_NULL_");
    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);

    will_return(__wrap_wdb_get_cache_stmt, 1);
    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "_NULL_");
    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);
    will_return(__wrap_sqlite3_changes, 1);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok ");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_dbsync_modify_type_exists_data_null_value_variant(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("hotfixes MODIFIED data1|__NULL__|__NULL__|", query);

    will_return_always(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "_NULL_");
    expect_value(__wrap_sqlite3_bind_text, pos, 3);
    expect_string(__wrap_sqlite3_bind_text, buffer, "_NULL_");

    will_return_always(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);
    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "_NULL_");

    will_return(__wrap_wdb_step, SQLITE_DONE);
    will_return(__wrap_sqlite3_changes, 1);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok ");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_dbsync_insert_type_exists_data_correct_null_value_from_json(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("hotfixes INSERTED data1|__NULL__||", query);

    will_return(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_int, index, 1);
    expect_value(__wrap_sqlite3_bind_int, value, 0);
    will_return_always(__wrap_sqlite3_bind_int, SQLITE_OK);

    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");

    expect_value(__wrap_sqlite3_bind_text, pos, 3);
    expect_string(__wrap_sqlite3_bind_text, buffer, "_NULL_");

    expect_value(__wrap_sqlite3_bind_text, pos, 4);
    expect_string(__wrap_sqlite3_bind_text, buffer, "");

    will_return_always(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok ");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_dbsync_delete_type_exists_data_null_value_from_json(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("hotfixes DELETED NULL||NULL|", query);

    will_return(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "");
    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);

    will_return(__wrap_wdb_get_cache_stmt, 1);
    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "");
    will_return(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);
    will_return(__wrap_sqlite3_changes, 1);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok ");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}

void test_dbsync_modify_type_exists_data_null_value_from_json(void **state) {
    int ret = -1;
    test_struct_t *data  = (test_struct_t *)*state;
    char *query = NULL;

    os_strdup("hotfixes MODIFIED data1|__NULL__||", query);

    will_return_always(__wrap_wdb_get_cache_stmt, 1);

    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "data1");
    expect_value(__wrap_sqlite3_bind_text, pos, 2);
    expect_string(__wrap_sqlite3_bind_text, buffer, "");
    expect_value(__wrap_sqlite3_bind_text, pos, 3);
    expect_string(__wrap_sqlite3_bind_text, buffer, "_NULL_");

    will_return_always(__wrap_sqlite3_bind_text, SQLITE_OK);

    will_return(__wrap_wdb_step, SQLITE_DONE);
    expect_value(__wrap_sqlite3_bind_text, pos, 1);
    expect_string(__wrap_sqlite3_bind_text, buffer, "_NULL_");

    will_return(__wrap_wdb_step, SQLITE_DONE);
    will_return(__wrap_sqlite3_changes, 1);

    ret = wdb_parse_dbsync(data->wdb, query, data->output);

    assert_string_equal(data->output, "ok ");
    assert_int_equal(ret, OS_SUCCESS);

    os_free(query);
}



int main()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test_setup_teardown(test_wdb_parse_syscheck_no_space, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_scan_info_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_scan_info_ok, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_update_info_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_update_info_ok, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_clean_old_entries_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_clean_old_entries_ok, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_scan_info_update_noarg, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_scan_info_update_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_scan_info_update_ok, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_scan_info_fim_check_control_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_scan_info_fim_check_control_ok, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_syscheck_load_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_syscheck_load_ok, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_fim_delete_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_fim_delete_ok, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_syscheck_save_noarg, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_syscheck_save_invalid_type, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_syscheck_save_file_type_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_syscheck_save_file_nospace, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_syscheck_save_file_type_ok, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_syscheck_save_registry_type_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_syscheck_save_registry_type_ok, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_syscheck_save2_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_syscheck_save2_ok, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_integrity_check_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_integrity_check_no_data, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_integrity_check_checksum_fail, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_integrity_check_ok, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_integrity_clear_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_integrity_clear_ok, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_invalid_command, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_parse_rootcheck_badquery, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_parse_rootcheck_delete_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_parse_rootcheck_delete_ok, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_parse_rootcheck_save_invalid_no_next, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_parse_rootcheck_save_no_ptr, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_parse_rootcheck_save_date_max_long, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_parse_rootcheck_save_update_cache_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_parse_rootcheck_save_update_success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_parse_rootcheck_save_update_insert_cache_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_wdb_parse_rootcheck_save_update_insert_success, test_setup, test_teardown),
        /* Tests osinfo */
        cmocka_unit_test_setup_teardown(test_osinfo_syntax_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_invalid_action, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_missing_action, test_setup, test_teardown),
        // osinfo get
        cmocka_unit_test_setup_teardown(test_osinfo_get_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_get_success, test_setup, test_teardown),
        // osinfo set_triaged
        cmocka_unit_test_setup_teardown(test_osinfo_set_triaged_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_set_triaged_success, test_setup, test_teardown),
        // osinfo set
        cmocka_unit_test_setup_teardown(test_osinfo_set_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_set_error_no_scan_id, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_set_error_no_scan_time, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_set_error_no_hostname, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_set_error_no_architecture, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_set_error_no_os_name, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_set_error_no_os_version, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_set_error_no_os_codename, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_set_error_no_os_major, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_set_error_no_os_minor, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_set_error_no_os_build, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_set_error_no_os_platform, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_set_error_no_sysname, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_set_error_no_release, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_set_error_no_version, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_set_error_no_os_release, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_set_error_saving, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_osinfo_set_success, test_setup, test_teardown),
        /* Tests vuln_cves */
        cmocka_unit_test_setup_teardown(test_vuln_cves_syntax_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_vuln_cves_invalid_action, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_vuln_cves_missing_action, test_setup, test_teardown),
        // wdb_parse_agents_insert_vuln_cves
        cmocka_unit_test_setup_teardown(test_vuln_cves_insert_syntax_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_vuln_cves_insert_constraint_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_vuln_cves_insert_command_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_vuln_cves_insert_command_success, test_setup, test_teardown),
        // wdb_parse_agents_update_vuln_cves_status
        cmocka_unit_test_setup_teardown(test_vuln_cves_update_status_syntax_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_vuln_cves_update_status_constraint_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_vuln_cves_update_status_command_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_vuln_cves_update_status_command_success, test_setup, test_teardown),
        // wdb_parse_agents_update_vuln_cves_status_by_type
        cmocka_unit_test_setup_teardown(test_vuln_cves_update_status_by_type_command_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_vuln_cves_update_status_by_type_command_success, test_setup, test_teardown),
        // wdb_parse_agents_remove_vuln_cves
        cmocka_unit_test_setup_teardown(test_vuln_cves_remove_syntax_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_vuln_cves_remove_json_data_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_vuln_cves_remove_by_status_success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_vuln_cves_remove_entry_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_vuln_cves_remove_entry_success, test_setup, test_teardown),
        // wdb_parse_agents_clear_vuln_cves
        cmocka_unit_test_setup_teardown(test_vuln_cves_clear_command_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_vuln_cves_clear_command_success, test_setup, test_teardown),
        // wdb_parse_packages
        cmocka_unit_test_setup_teardown(test_packages_get_success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_packages_get_not_triaged_success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_packages_get_null_response, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_packages_get_err_response, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_packages_get_sock_err_response, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_packages_save_success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_packages_save_success_null_items, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_packages_save_success_empty_items, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_packages_save_missing_items, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_packages_save_err, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_packages_del_success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_packages_del_success_null_items, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_packages_del_update_err, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_packages_del_delete_err, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_packages_invalid_action, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_packages_no_action, test_setup, test_teardown),
        // wdb_parse_hotfixes
        cmocka_unit_test_setup_teardown(test_hotfixes_get_success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_hotfixes_get_null_response, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_hotfixes_get_err_response, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_hotfixes_get_sock_err_response, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_hotfixes_save_success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_hotfixes_save_success_null_items, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_hotfixes_save_missing_items, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_hotfixes_save_err, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_hotfixes_del_success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_hotfixes_del_success_null_items, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_hotfixes_del_delete_err, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_hotfixes_invalid_action, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_hotfixes_no_action, test_setup, test_teardown),
        /* dbsync Tests */
        cmocka_unit_test_setup_teardown(test_dbsync_insert_fail_0_arguments, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_insert_fail_1_arguments, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_insert_fail_2_arguments, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_insert_type_not_exists, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_insert_type_exists_data_incorrect, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_insert_type_exists_data_correct, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_delete_type_exists_data_1, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_modify_type_exists_data_1, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_insert_type_exists_null_stmt, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_modify_type_exists_data_real_value, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_modify_type_exists_data_float_null_value, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_modify_type_exists_data_text_null_value, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_modify_type_exists_data_integer_null_value, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_delete_type_exists_data_compound_pk, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_modify_type_exists_data_compound_pk, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_modify_type_exists_data_stmt_fail, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_delete_type_exists_data_stmt_fail, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_delete_type_exists_data_select_stmt_fail, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_delete_type_exists_data_bind_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_modify_type_exists_data_bind_error, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_delete_type_exists_data_bind_error_ports, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_modify_type_exists_data_bind_error_ports, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_delete_type_exists_data_compound_pk_select_data_fail, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_delete_type_exists_data_compound_pk_select_data, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_insert_type_exists_data_return_values, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_insert_type_exists_data_correct_null_value, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_delete_type_exists_data_null_value, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_modify_type_exists_data_null_value, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_insert_type_exists_data_correct_null_value_variant, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_delete_type_exists_data_null_value_variant, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_modify_type_exists_data_null_value_variant, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_insert_type_exists_data_correct_null_value_from_json, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_delete_type_exists_data_null_value_from_json, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_modify_type_exists_data_null_value_from_json, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dbsync_modify_type_exists_avoid_old_implementation, test_setup, test_teardown)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);

}
