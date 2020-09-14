/*
** Copyright (c) 2017-2020 Nikola Kolev <koue@chaosophia.net>
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

#ifndef _FSLBASE_H
#define _FSLBASE_H

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct Blob Blob;

/*
** PRINTF
*/

int vxprintf(Blob *pBlob, const char *fmt, va_list ap);
char *mprintf(const char *zFormat, ...);
char *vmprintf(const char *zFormat, va_list ap);

/*
** BLOB
*/

/*
** A Blob can hold a string or a binary object of arbitrary size.  The
** size changes as necessary.
*/
struct Blob {
  unsigned int nUsed;            /* Number of bytes used in aData[] */
  unsigned int nAlloc;           /* Number of bytes allocated for aData[] */
  unsigned int iCursor;          /* Next character of input to parse */
  unsigned int blobFlags;        /* One or more BLOBFLAG_* bits */
  char *aData;                   /* Where the information is stored */
  void (*xRealloc)(Blob*, unsigned int); /* Function to reallocate the buffer */
};

/*
** Make sure a blob does not contain malloced memory.
**
** This might fail if we are unlucky and x is uninitialized.  For that
** reason it should only be used locally for debugging.  Leave it turned
** off for production.
*/
#if 0  /* Enable for debugging only */
#define assert_blob_is_reset(x) assert(blob_is_reset(x))
#else
#define assert_blob_is_reset(x)
#endif

/*
** The current size of a Blob
*/
#define blob_size(X)  ((X)->nUsed)

/*
** The buffer holding the blob data
*/
#define blob_buffer(X)  ((X)->aData)

/*
** Make sure a blob is initialized
*/
#define blob_is_init(x) \
  assert((x)->xRealloc==blobReallocMalloc || (x)->xRealloc==blobReallocStatic)

#define BLOB_INITIALIZER  {0,0,0,0,0,blobReallocMalloc}

extern const Blob empty_blob;

char *blob_str(Blob *p);
char *blob_materialize(Blob *pBlob);
void blob_append_full(Blob *pBlob, const char *aData, int nData);
void blob_append(Blob *pBlob, const char *aData, int nData);
void blob_append_char(Blob *pBlob, char c);
void blobReallocMalloc(Blob *pBlob, unsigned int newSize);
void blob_resize(Blob *pBlob, unsigned int newSize);
int blob_read_from_channel(Blob *pBlob, FILE *in, int nToRead);
void blob_zero(Blob *pBlob);
void blob_vappendf(Blob *pBlob, const char *zFormat, va_list ap);
void blob_reset(Blob *pBlob);
void blob_init(Blob *pBlob, const char *zData, int size);

/*
** UTIL
*/
void fossil_panic(const char *c);
void *fossil_malloc(size_t n);
void fossil_free(void *p);
void *fossil_realloc(void *p, size_t n);

#endif
