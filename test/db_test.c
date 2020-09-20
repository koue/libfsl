/*
 * Copyright (c) 2017-2020 Nikola Kolev <koue@chaosophia.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "fslbase.h"
#include "fsldb.h"
#include "cez_test.h"

Global g;

const char TestSchema[] =
	"CREATE TABLE tbl_test("
	" id INTEGER PRIMARY KEY,"
	" name TEXT"
	");";

const char *dbname = "/tmp/testme-xzfadget48.db";
const char *sqltracefile = "/tmp/testme-4trwerfsdfrf89.log";

Stmt q;

int
main(void)
{
	Blob sqltrace_list = empty_blob;
	char command[256];
	FILE *pf;
	char *word = NULL;

	cez_test_start();
	/* create database */
	db_init_database(dbname, TestSchema, (char*)0);
	/* open database */
	assert(sqlite3_open(dbname, &g.db) == SQLITE_OK);
	sqlite3_trace_v2(g.db, SQLITE_TRACE_STMT, db_sql_trace, 0);
	assert((g.sqltrace = fopen(sqltracefile, "w+")) != NULL);
	/* insert record */
	db_multi_exec("INSERT INTO tbl_test(name) VALUES (%Q)", "testuser0");
	db_multi_exec("INSERT INTO tbl_test(name) VALUES (%Q)", "testuser1");
	db_multi_exec("INSERT INTO tbl_test(name) VALUES (%Q)", "testuser2");
	db_multi_exec("INSERT INTO tbl_test(name) VALUES (%Q)", "testuser3");
	/* get text */
	assert(!word);
	word = db_text(0, "SELECT name FROM tbl_test WHERE name='testuser0'");
	free(word);
	word = db_text(0, "SELECT name FROM tbl_test WHERE name='testuser1'");
	free(word);
	/* get int */
	assert(db_int(0, "SELECT id FROM tbl_test WHERE name='testuser2'"));
	/* multiple results */
	db_prepare(&q, "SELECT id, name FROM tbl_test");
	while(db_step(&q)==SQLITE_ROW){
		assert(db_column_int(&q, 0));
		assert(strlen(db_column_text(&q, 1)));
	}
	db_finalize(&q);
	sqlite3_close(g.db);
	snprintf(command, sizeof(command), "rm %s", dbname);
	system(command);
	fclose(g.sqltrace);
	snprintf(command, sizeof(command), "cat %s", sqltracefile);
	pf = popen(command, "r");
	blob_read_from_channel(&sqltrace_list, pf, -1);
	pclose(pf);
	assert(blob_str(&sqltrace_list));
	snprintf(command, sizeof(command), "rm %s", sqltracefile);
	system(command);
	blob_reset(&sqltrace_list);

	return (0);
}
