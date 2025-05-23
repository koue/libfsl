/*
** Copyright (c) 2017-2025 Nikola Kolev <koue@chaosophia.net>
** Copyright (c) 2006 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the Simplified BSD License (also
** known as the "2-Clause License" or "FreeBSD License".)
**
** This program is distributed in the hope that it will be useful,
** but without any warranty; without even the implied warranty of
** merchantability or fitness for a particular purpose.
**
** Author contact information:
**   drh@hwaci.com
**   http://www.hwaci.com/drh/
**
*******************************************************************************
**
** Code for interfacing to the various databases.
**
*/

#include "fslbase.h"
#include "fsldb.h"

/*
** All static variable that a used by only this file are gathered into
** the following structure.
*/
static struct DbLocalData {
  int nBegin;               /* Nesting depth of BEGIN */
  int doRollback;           /* True to force a rollback */
  int nCommitHook;          /* Number of commit hooks */
  Stmt *pAllStmt;           /* List of all unfinalized statements */
  int nPrepare;             /* Number of calls to sqlite3_prepare_v2() */
  int nDeleteOnFail;        /* Number of entries in azDeleteOnFail[] */
  struct sCommitHook {
    int (*xHook)(void);         /* Functions to call at db_end_transaction() */
    int sequence;               /* Call functions in sequence order */
  } aHook[5];
  char *azDeleteOnFail[3];  /* Files to delete on a failure */
  char *azBeforeCommit[5];  /* Commands to run prior to COMMIT */
  int nBeforeCommit;        /* Number of entries in azBeforeCommit */
  int nPriorChanges;        /* sqlite3_total_changes() at transaction start */
  const char *zStartFile;   /* File in which transaction was started */
  int iStartLine;           /* Line of zStartFile where transaction started */
} db = {0, 0, 0, 0, 0, 0, };

/*
** Execute a query.  Return the first column of the first row
** of the result set as a string.  Space to hold the string is
** obtained from malloc().  If the result set is empty, return
** zDefault instead.
*/
char *db_text(const char *zDefault, const char *zSql, ...){
  va_list ap;
  Stmt s;
  char *z;
  va_start(ap, zSql);
  db_vprepare(&s, 0, zSql, ap);
  va_end(ap);
  if( db_step(&s)==SQLITE_ROW ){
    z = fossil_strdup_nn((const char*)sqlite3_column_text(s.pStmt, 0));
  }else{
    z = fossil_strdup(zDefault);
  }
  db_finalize(&s);
  return z;
}

#if 0 /* libfsl */
/*
** Print warnings if a query is inefficient.
*/
static void db_stats(Stmt *pStmt){
#ifdef FOSSIL_DEBUG
  int c1, c2, c3;
  const char *zSql = sqlite3_sql(pStmt->pStmt);
  if( zSql==0 ) return;
  c1 = sqlite3_stmt_status(pStmt->pStmt, SQLITE_STMTSTATUS_FULLSCAN_STEP, 1);
  c2 = sqlite3_stmt_status(pStmt->pStmt, SQLITE_STMTSTATUS_AUTOINDEX, 1);
  c3 = sqlite3_stmt_status(pStmt->pStmt, SQLITE_STMTSTATUS_SORT, 1);
  if( c1>pStmt->nStep*4 && strstr(zSql,"/*scan*/")==0 ){
    fossil_warning("%d scan steps for %d rows in [%s]", c1, pStmt->nStep, zSql);
  }else if( c2 ){
    fossil_warning("%d automatic index rows in [%s]", c2, zSql);
  }else if( c3 && strstr(zSql,"/*sort*/")==0 && strstr(zSql,"/*scan*/")==0 ){
    fossil_warning("sort w/o index in [%s]", zSql);
  }
  pStmt->nStep = 0;
#endif
}
#endif /* libfsl */

/*
** Call this routine when a database error occurs.
** This routine throws a fatal error.  It does not return.
*/
static void db_err(const char *zFormat, ...){
  va_list ap;
  char *z;
  va_start(ap, zFormat);
  z = vmprintf(zFormat, ap);
  va_end(ap);
#if 0 /* libfsl */
#ifdef FOSSIL_ENABLE_JSON
  if( g.json.isJsonMode!=0 ){
    /*
    ** Avoid calling into the JSON support subsystem if it
    ** has not yet been initialized, e.g. early SQLite log
    ** messages, etc.
    */
    json_bootstrap_early();
    json_err( 0, z, 1 );
  }
  else
#endif /* FOSSIL_ENABLE_JSON */
  if( g.xferPanic && g.cgiOutput==1 ){
    cgi_reset_content();
    @ error Database\serror:\s%F(z)
    cgi_reply();
  }
  fossil_fatal("Database error: %s", z);
#else
  fprintf(stderr, "%s: %s\n", __func__, z);
  free(z);
  db_close(1);
  exit(1);
#endif /* libfsl */
}

/*
** Check a result code.  If it is not SQLITE_OK, print the
** corresponding error message and exit.
*/
static void db_check_result(int rc, Stmt *pStmt){
  if( rc!=SQLITE_OK ){
    db_err("SQL error (%d,%d: %s) while running [%s]",
       rc, sqlite3_extended_errcode(g.db),
       sqlite3_errmsg(g.db), blob_str(&pStmt->sql));
  }
}

/*
** Reset or finalize a statement.
*/
int db_finalize(Stmt *pStmt){
  int rc;
  if( pStmt->pNext ){
    pStmt->pNext->pPrev = pStmt->pPrev;
  }
  if( pStmt->pPrev ){
    pStmt->pPrev->pNext = pStmt->pNext;
  }else if( db.pAllStmt==pStmt ){
    db.pAllStmt = pStmt->pNext;
  }
  pStmt->pNext = 0;
  pStmt->pPrev = 0;
#if 0 /* libfsl */
  if( g.fSqlStats ){ db_stats(pStmt); }
#endif /* libfsl */
  blob_reset(&pStmt->sql);
  rc = sqlite3_finalize(pStmt->pStmt);
  db_check_result(rc, pStmt);
  pStmt->pStmt = 0;
  return rc;
}

/*
** Close the database connection.
**
** Check for unfinalized statements and report errors if the reportErrors
** argument is true.  Ignore unfinalized statements when false.
*/
void db_close(int reportErrors){
  sqlite3_stmt *pStmt;
  if( g.db==0 ) return;
  sqlite3_set_authorizer(g.db, 0, 0);
#if 0 /* libfsl */
  if( g.fSqlStats ){
    int cur, hiwtr;
    sqlite3_db_status(g.db, SQLITE_DBSTATUS_LOOKASIDE_USED, &cur, &hiwtr, 0);
    fprintf(stderr, "-- LOOKASIDE_USED         %10d %10d\n", cur, hiwtr);
    sqlite3_db_status(g.db, SQLITE_DBSTATUS_LOOKASIDE_HIT, &cur, &hiwtr, 0);
    fprintf(stderr, "-- LOOKASIDE_HIT                     %10d\n", hiwtr);
    sqlite3_db_status(g.db, SQLITE_DBSTATUS_LOOKASIDE_MISS_SIZE, &cur,&hiwtr,0);
    fprintf(stderr, "-- LOOKASIDE_MISS_SIZE               %10d\n", hiwtr);
    sqlite3_db_status(g.db, SQLITE_DBSTATUS_LOOKASIDE_MISS_FULL, &cur,&hiwtr,0);
    fprintf(stderr, "-- LOOKASIDE_MISS_FULL               %10d\n", hiwtr);
    sqlite3_db_status(g.db, SQLITE_DBSTATUS_CACHE_USED, &cur, &hiwtr, 0);
    fprintf(stderr, "-- CACHE_USED             %10d\n", cur);
    sqlite3_db_status(g.db, SQLITE_DBSTATUS_SCHEMA_USED, &cur, &hiwtr, 0);
    fprintf(stderr, "-- SCHEMA_USED            %10d\n", cur);
    sqlite3_db_status(g.db, SQLITE_DBSTATUS_STMT_USED, &cur, &hiwtr, 0);
    fprintf(stderr, "-- STMT_USED              %10d\n", cur);
    sqlite3_status(SQLITE_STATUS_MEMORY_USED, &cur, &hiwtr, 0);
    fprintf(stderr, "-- MEMORY_USED            %10d %10d\n", cur, hiwtr);
    sqlite3_status(SQLITE_STATUS_MALLOC_SIZE, &cur, &hiwtr, 0);
    fprintf(stderr, "-- MALLOC_SIZE                       %10d\n", hiwtr);
    sqlite3_status(SQLITE_STATUS_MALLOC_COUNT, &cur, &hiwtr, 0);
    fprintf(stderr, "-- MALLOC_COUNT           %10d %10d\n", cur, hiwtr);
    sqlite3_status(SQLITE_STATUS_PAGECACHE_OVERFLOW, &cur, &hiwtr, 0);
    fprintf(stderr, "-- PCACHE_OVFLOW          %10d %10d\n", cur, hiwtr);
    fprintf(stderr, "-- prepared statements    %10d\n", db.nPrepare);
  }
#endif /* libfsl */
  while( db.pAllStmt ){
    db_finalize(db.pAllStmt);
  }
  if( db.nBegin ){
    if( reportErrors ){
#if 0 /* libfsl */
      fossil_warning("Transaction started at %s:%d never commits",
                     db.zStartFile, db.iStartLine);
#else
      fprintf(stderr, "Transaction started at %s:%d never commits\n",
                     db.zStartFile, db.iStartLine);
#endif /* libfsl */
    }
    db_end_transaction(1);
  }
  pStmt = 0;
  sqlite3_busy_timeout(g.db, 0);
  g.dbIgnoreErrors++; /* Stop "database locked" warnings */
  sqlite3_exec(g.db, "PRAGMA optimize", 0, 0, 0);
  g.dbIgnoreErrors--;
#if 0 /* libfsl */
  db_close_config();
#endif /* libfsl */

  /* If the localdb has a lot of unused free space,
  ** then VACUUM it as we shut down.
  */
  if( db_database_slot("localdb")>=0 ){
    int nFree = db_int(0, "PRAGMA localdb.freelist_count");
    int nTotal = db_int(0, "PRAGMA localdb.page_count");
    if( nFree>nTotal/4 ){
#if 0 /* libfsl */
      db_unprotect(PROTECT_ALL);
#endif /* libfsl */
      db_multi_exec("VACUUM localdb;");
#if 0 /* libfsl */
      db_protect_pop();
#endif /* libfsl */
    }
  }

  if( g.db ){
    int rc;
    sqlite3_wal_checkpoint(g.db, 0);
    rc = sqlite3_close(g.db);
#if 0 /* libfsl */
    if( g.fSqlTrace ) fossil_trace("-- sqlite3_close(%d)\n", rc);
#endif /* libfsl */
    if( rc==SQLITE_BUSY && reportErrors ){
      while( (pStmt = sqlite3_next_stmt(g.db, pStmt))!=0 ){
#if 0 /* libfsl */
        fossil_warning("unfinalized SQL statement: [%s]", sqlite3_sql(pStmt));
#else
        fprintf(stderr, "unfinalized SQL statement: [%s]\n", sqlite3_sql(pStmt));
#endif /* libfsl */
      }
    }
    g.db = 0;
  }
#if 0 /* libfsl */
  g.repositoryOpen = 0;
  g.localOpen = 0;
  db.bProtectTriggers = 0;
  assert( g.dbConfig==0 );
  assert( g.zConfigDbName==0 );
  backoffice_run_if_needed();
#endif /* libfsl */
}

/*
** Prepare a Stmt.  Assume that the Stmt is previously uninitialized.
** If the input string contains multiple SQL statements, only the first
** one is processed.  All statements beyond the first are silently ignored.
*/
int db_vprepare(Stmt *pStmt, int flags, const char *zFormat, va_list ap){
  int rc;
  int prepFlags = 0;
  char *zSql;
  const char *zExtra = 0;
  blob_zero(&pStmt->sql);
  blob_vappendf(&pStmt->sql, zFormat, ap);
  va_end(ap);
  zSql = blob_str(&pStmt->sql);
  db.nPrepare++;
#if 0 /* libfsl */
  db_append_dml(zSql);
#endif /* libfsl */
  if( flags & DB_PREPARE_PERSISTENT ){
    prepFlags = SQLITE_PREPARE_PERSISTENT;
  }
  rc = sqlite3_prepare_v3(g.db, zSql, -1, prepFlags, &pStmt->pStmt, &zExtra);
  if( rc!=0 && (flags & DB_PREPARE_IGNORE_ERROR)==0 ){
    db_err("%s\n%s", sqlite3_errmsg(g.db), zSql);
  }else if( zExtra && !fossil_all_whitespace(zExtra) ){
    db_err("surplus text follows SQL: \"%s\"", zExtra);
  }
  pStmt->pNext = db.pAllStmt;
  pStmt->pPrev = 0;
  if( db.pAllStmt ) db.pAllStmt->pPrev = pStmt;
  db.pAllStmt = pStmt;
  pStmt->nStep = 0;
  pStmt->rc = rc;
  return rc;
}

/*
** Step the SQL statement.  Return either SQLITE_ROW or an error code
** or SQLITE_OK if the statement finishes successfully.
*/
int db_step(Stmt *pStmt){
  int rc;
  if( pStmt->pStmt==0 ) return pStmt->rc;
  rc = sqlite3_step(pStmt->pStmt);
  pStmt->nStep++;
  return rc;
}

/* End a transaction previously started using db_begin_transaction()
** or db_begin_write().
*/
void db_end_transaction(int rollbackFlag){
  if( g.db==0 ) return;
  if( db.nBegin<=0 ){
#if 0 /* libfsl */
    fossil_warning("Extra call to db_end_transaction");
#else
    fprintf(stderr, "Extra call to db_end_transaction\n");
#endif /* libfsl */
    return;
  }
  if( rollbackFlag ){
    db.doRollback = 1;
#if 0 /* libfsl */
    if( g.fSqlTrace ) fossil_trace("-- ROLLBACK by request\n");
#endif /* libfsl */
  }
  db.nBegin--;
  if( db.nBegin==0 ){
    int i;
    if( db.doRollback==0 && db.nPriorChanges<sqlite3_total_changes(g.db) ){
      i = 0;
#if 0 /* libfsl */
      db_protect_only(PROTECT_SENSITIVE);
#endif /* libfsl */
      while( db.nBeforeCommit ){
        db.nBeforeCommit--;
        sqlite3_exec(g.db, db.azBeforeCommit[i], 0, 0, 0);
        sqlite3_free(db.azBeforeCommit[i]);
        i++;
      }
#if 0 /* libfsl */
      leaf_do_pending_checks();
      db_protect_pop();
#endif /* libfsl */
    }
    for(i=0; db.doRollback==0 && i<db.nCommitHook; i++){
      int rc = db.aHook[i].xHook();
      if( rc ){
        db.doRollback = 1;
#if 0 /* libfsl */
        if( g.fSqlTrace ) fossil_trace("-- ROLLBACK due to aHook[%d]\n", i);
#endif /* libfsl */
      }
    }
    while( db.pAllStmt ){
      db_finalize(db.pAllStmt);
    }
    db_multi_exec("%s", db.doRollback ? "ROLLBACK" : "COMMIT");
    db.doRollback = 0;
  }
}

/*
** Execute a query and return a single integer value.
*/
i64 db_int64(i64 iDflt, const char *zSql, ...){
  va_list ap;
  Stmt s;
  i64 rc;
  va_start(ap, zSql);
  db_vprepare(&s, 0, zSql, ap);
  va_end(ap);
  if( db_step(&s)!=SQLITE_ROW ){
    rc = iDflt;
  }else{
    rc = db_column_int64(&s, 0);
  }
  db_finalize(&s);
  return rc;
}

/*
** Execute a query and return a single integer value.
*/
int db_int(int iDflt, const char *zSql, ...){
  va_list ap;
  Stmt s;
  int rc;
  va_start(ap, zSql);
  db_vprepare(&s, 0, zSql, ap);
  va_end(ap);
  if( db_step(&s)!=SQLITE_ROW ){
    rc = iDflt;
  }else{
    rc = db_column_int(&s, 0);
  }
  db_finalize(&s);
  return rc;
}

/*
** Execute multiple SQL statements using printf-style formatting.
*/
int db_multi_exec(const char *zSql, ...){
  Blob sql;
  int rc;
  va_list ap;

  blob_init(&sql, 0, 0);
  va_start(ap, zSql);
  blob_vappendf(&sql, zSql, ap);
  va_end(ap);
  rc = db_exec_sql(blob_str(&sql));
  blob_reset(&sql);
  return rc;
}

/*
** Extract text, integer, or blob values from the N-th column of the
** current row.
*/
int db_column_int(Stmt *pStmt, int N){
  return sqlite3_column_int(pStmt->pStmt, N);
}

/*
** Extract text, integer, or blob values from the N-th column of the
** current row.
*/
i64 db_column_int64(Stmt *pStmt, int N){
  return sqlite3_column_int64(pStmt->pStmt, N);
}

/*
** Return the slot number for database zLabel.  The first database
** opened is slot 0.  The "temp" database is slot 1.  Attached databases
** are slots 2 and higher.
**
** Return -1 if zLabel does not match any open database.
*/
int db_database_slot(const char *zLabel){
  int iSlot = -1;
  int rc;
  Stmt q;
  if( g.db==0 ) return iSlot;
  rc = db_prepare_ignore_error(&q, "PRAGMA database_list");
  if( rc==SQLITE_OK ){
    while( db_step(&q)==SQLITE_ROW ){
#if 0 /* libfsl */
      if( fossil_strcmp(db_column_text(&q,1),zLabel)==0 ){
#else
      if( strcmp(db_column_text(&q,1),zLabel)==0 ){
#endif /* libfsl */
        iSlot = db_column_int(&q, 0);
        break;
      }
    }
  }
  db_finalize(&q);
  return iSlot;
}

/*
** Extract text, integer, or blob values from the N-th column of the
** current row.
*/
const char *db_column_text(Stmt *pStmt, int N){
  return (char*)sqlite3_column_text(pStmt->pStmt, N);
}

/*
** Prepare a Stmt.  Assume that the Stmt is previously uninitialized.
** If the input string contains multiple SQL statements, only the first
** one is processed.  All statements beyond the first are silently ignored.
*/
int db_prepare_ignore_error(Stmt *pStmt, const char *zFormat, ...){
  int rc;
  va_list ap;
  va_start(ap, zFormat);
  rc = db_vprepare(pStmt, DB_PREPARE_IGNORE_ERROR, zFormat, ap);
  va_end(ap);
  return rc;
}

/*
** Prepare a Stmt.  Assume that the Stmt is previously uninitialized.
** If the input string contains multiple SQL statements, only the first
** one is processed.  All statements beyond the first are silently ignored.
*/
int db_prepare(Stmt *pStmt, const char *zFormat, ...){
  int rc;
  va_list ap;
  va_start(ap, zFormat);
  rc = db_vprepare(pStmt, 0, zFormat, ap);
  va_end(ap);
  return rc;
}

/*
** Initialize a new database file with the given schema.  If anything
** goes wrong, call db_err() to exit.
**
** If zFilename is NULL, then create an empty repository in an in-memory
** database.
*/
void db_init_database(
  const char *zFileName,   /* Name of database file to create */
  const char *zSchema,     /* First part of schema */
  ...                      /* Additional SQL to run.  Terminate with NULL. */
){
  sqlite3 *xdb;
  int rc;
  const char *zSql;
  va_list ap;

#if 0 /* libfsl */
  xdb = db_open(zFileName ? zFileName : ":memory:");
#else
  if (sqlite3_open(zFileName, &xdb) != SQLITE_OK) {
    fprintf(stderr, "Cannot open database file: %s\n", zFileName);
    exit(1);
  }
#endif /* libfsl */
  sqlite3_exec(xdb, "BEGIN EXCLUSIVE", 0, 0, 0);
  rc = sqlite3_exec(xdb, zSchema, 0, 0, 0);
  if( rc!=SQLITE_OK ){
    db_err("%s", sqlite3_errmsg(xdb));
  }
  va_start(ap, zSchema);
  while( (zSql = va_arg(ap, const char*))!=0 ){
    rc = sqlite3_exec(xdb, zSql, 0, 0, 0);
    if( rc!=SQLITE_OK ){
      db_err("%s", sqlite3_errmsg(xdb));
    }
  }
  va_end(ap);
  sqlite3_exec(xdb, "COMMIT", 0, 0, 0);
#if 0 /* libfsl */
  if( zFileName || g.db!=0 ){
    sqlite3_close(xdb);
  }else{
    g.db = xdb;
  }
#else
  sqlite3_close(xdb);
#endif /* libfsl */
}

/*
** Callback for sqlite3_trace_v2();
*/
int db_sql_trace(unsigned m, void *notUsed, void *pP, void *pX){
  sqlite3_stmt *pStmt = (sqlite3_stmt*)pP;
  char *zSql;
  int n;
  const char *zArg = (const char*)pX;
  char zEnd[100];
  if( m & SQLITE_TRACE_CLOSE ){
    /* If we are tracking closes, that means we want to clean up static
    ** prepared statements. */
    while( db.pAllStmt ){
      db_finalize(db.pAllStmt);
    }
    return 0;
  }
  if( zArg[0]=='-' ) return 0;
  if( m & SQLITE_TRACE_PROFILE ){
    sqlite3_int64 nNano = *(sqlite3_int64*)pX;
    double rMillisec = 0.000001 * nNano;
    int nRun = sqlite3_stmt_status(pStmt, SQLITE_STMTSTATUS_RUN, 0);
    int nVmStep = sqlite3_stmt_status(pStmt, SQLITE_STMTSTATUS_VM_STEP, 1);
    sqlite3_snprintf(sizeof(zEnd),zEnd," /* %.3fms, %r run, %d vm-steps */\n",
        rMillisec, nRun, nVmStep);
  }else{
    zEnd[0] = '\n';
    zEnd[1] = 0;
  }
  zSql = sqlite3_expanded_sql(pStmt);
  n = (int)strlen(zSql);
#if 0 /* libfsl */
  fossil_trace("%s%s%s", zSql, (n>0 && zSql[n-1]==';') ? "" : ";", zEnd);
#else
  if (g.sqltrace != NULL) {
    fprintf(g.sqltrace, "%s%s\n", zSql, (n>0 && zSql[n-1]==';') ? "" : ";");
  } else {
    fprintf(stderr, "%s%s\n", zSql, (n>0 && zSql[n-1]==';') ? "" : ";");
  }
#endif /* libfsl */
  sqlite3_free(zSql);
  return 0;
}

/*
** Execute multiple SQL statements.  The input text is executed
** directly without any formatting.
*/
int db_exec_sql(const char *z){
  int rc = SQLITE_OK;
  sqlite3_stmt *pStmt;
  const char *zEnd;
  while( rc==SQLITE_OK && z[0] ){
    pStmt = 0;
    rc = sqlite3_prepare_v2(g.db, z, -1, &pStmt, &zEnd);
    if( rc ){
      db_err("%s: {%s}", sqlite3_errmsg(g.db), z);
    }else if( pStmt ){
      db.nPrepare++;
#if 0 /* libfsl */
      db_append_dml(sqlite3_sql(pStmt));
#endif /* libfsl */
      while( sqlite3_step(pStmt)==SQLITE_ROW ){}
      rc = sqlite3_finalize(pStmt);
      if( rc ) db_err("%s: {%.*s}", sqlite3_errmsg(g.db), (int)(zEnd-z), z);
    }
    z = zEnd;
  }
  return rc;
}

/* Prepare a statement using text placed inside a Blob
** using blob_append_sql().
*/
int db_prepare_blob(Stmt *pStmt, Blob *pSql){
  int rc;
  char *zSql;
  pStmt->sql = *pSql;
  blob_init(pSql, 0, 0);
  zSql = blob_sql_text(&pStmt->sql);
  db.nPrepare++;
  rc = sqlite3_prepare_v3(g.db, zSql, -1, 0, &pStmt->pStmt, 0);
  if( rc!=0 ){
    db_err("%s\n%s", sqlite3_errmsg(g.db), zSql);
  }
  pStmt->pNext = pStmt->pPrev = 0;
  pStmt->nStep = 0;
  pStmt->rc = rc;
  return rc;
}

