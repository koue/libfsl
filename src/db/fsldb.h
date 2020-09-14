/*
** Copyright (c) 2019-2020 Nikola Kolev <koue@chaosophia.net>
** Copyright (c) 2006 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the Simplified BSD License (also
** known as the "2-Clause License" or "FreeBSD License".)

** This program is distributed in the hope that it will be useful,
** but without any warranty; without even the implied warranty of
** merchantability or fitness for a particular purpose.
**
** Author contact information:
**   drh@hwaci.com
**   http://www.hwaci.com/drh/
**
*******************************************************************************
*/

#ifndef _FSLDB_H
#define _FSLDB_H

#include <sqlite3.h>

typedef struct Global Global;
typedef struct Stmt Stmt;

struct Global {
  sqlite3 *db;
  FILE *sqltrace;
  int dbIgnoreErrors;
};
extern Global g;

/*
** DB
*/

/*
** An single SQL statement is represented as an instance of the following
** structure.
*/
struct Stmt {
  Blob sql;               /* The SQL for this statement */
  sqlite3_stmt *pStmt;    /* The results of sqlite3_prepare_v2() */
  Stmt *pNext, *pPrev;    /* List of all unfinalized statements */
  int nStep;              /* Number of sqlite3_step() calls */
  int rc;                 /* Error from db_vprepare() */
};

typedef sqlite3_int64 i64;

#define DB_PREPARE_IGNORE_ERROR  0x001  /* Suppress errors */
#define DB_PREPARE_PERSISTENT    0x002  /* Stmt will stick around for a while */

char *db_text(const char *zDefault, const char *zSql, ...);
int db_finalize(Stmt *pStmt);
void db_close(int reportErrors);
int db_vprepare(Stmt *pStmt, int flags, const char *zFormat, va_list ap);
int db_step(Stmt *pStmt);
void db_end_transaction(int rollbackFlag);
i64 db_int64(i64 iDflt, const char *zSql, ...);
int db_int(int iDflt, const char *zSql, ...);
int db_multi_exec(const char *zSql, ...);
int db_column_int(Stmt *pStmt, int N);
i64 db_column_int64(Stmt *pStmt, int N);
int db_database_slot(const char *zLabel);
const char *db_column_text(Stmt *pStmt, int N);
int db_prepare_ignore_error(Stmt *pStmt, const char *zFormat, ...);
int db_prepare(Stmt *pStmt, const char *zFormat, ...);
void db_init_database(const char *zFileName, const char *zSchema, ...);
int db_sql_trace(unsigned m, void *notUsed, void *pP, void *pX);
int db_exec_sql(const char *z);

#endif
