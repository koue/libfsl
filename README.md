# libfsl
Libraries used by Fossil SCM to work with SQLite database.

### compile
```
$ make
$ cc example.c -I /usr/local/include/ -I src/base -I src/db \
      -L/usr/local/lib -lsqlite3 src/base/libfslbase.a src/db/libfsldb.a
```

### example.c:
```
#include "fslbase.h"
#include "fsldb.h"

Global g;

int
main(void)
{
	char *word = NULL;
	int i;
	const char TestSchema[] =
		"CREATE TABLE tbl_test("
		" id INTEGER PRIMARY KEY,"
		" name TEXT"
		");";
	const char *dbname = "/tmp/testme-xzfadget48.db";
	Stmt q;


	/* create database */
	db_init_database(dbname, TestSchema, (char*)0);
	/* open database */
	sqlite3_open(dbname, &g.db);
	/* insert record */
	db_multi_exec("INSERT INTO tbl_test(name) VALUES (%Q)", "testuser0");
	db_multi_exec("INSERT INTO tbl_test(name) VALUES (%Q)", "testuser1");
	db_multi_exec("INSERT INTO tbl_test(name) VALUES (%Q)", "testuser2");
	db_multi_exec("INSERT INTO tbl_test(name) VALUES (%Q)", "testuser3");
	/* get text */
	word = db_text(0, "SELECT name FROM tbl_test WHERE name='testuser0'");
	free(word);
	/* get int */
	i = db_int(0, "SELECT id FROM tbl_test WHERE name='testuser2'");
	/* multiple results */
	db_prepare(&q, "SELECT id, name FROM tbl_test");
	while(db_step(&q)==SQLITE_ROW){
		i = db_column_int(&q, 0);
		i = strlen(db_column_text(&q, 1));
	}
	db_finalize(&q);
	sqlite3_close(g.db);

	return (0);
}
```
