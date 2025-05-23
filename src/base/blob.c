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
** A Blob is a variable-length containers for arbitrary string
** or binary data.
*/

#include "fslbase.h"

const Blob empty_blob = BLOB_INITIALIZER;
typedef long long int i64;
typedef long long int sqlite3_int64;

/*
** This routine is called if a blob operation fails because we
** have run out of memory.
*/
static void blob_panic(void){
  static const char zErrMsg[] = "out of memory\n";
  fputs(zErrMsg, stderr);
#if 0 /* libfsl */
  fossil_exit(1);
#else
  exit(1);
#endif /* libfsl */
}

/*
** Maximum size of a Blob's managed memory. This is ~2GB, largely for
** historical reasons.
**
*/
#define MAX_BLOB_SIZE 0x7fff0000

/*
** If n >= MAX_BLOB_SIZE, calls blob_panic(),
** else this is a no-op.
*/
static void blob_assert_safe_size(i64 n){
  if( n>=(i64)MAX_BLOB_SIZE ){
    blob_panic();
  }
}

/*
** A reallocation function for when the initial string is in unmanaged
** space.  Copy the string to memory obtained from malloc().
*/
static void blobReallocStatic(Blob *pBlob, unsigned int newSize){
  if( newSize==0 ){
    *pBlob = empty_blob;
  }else{
    char *pNew;
    blob_assert_safe_size((i64)newSize);
    pNew = fossil_malloc( newSize );
    if( pBlob->nUsed>newSize ) pBlob->nUsed = newSize;
    memcpy(pNew, pBlob->aData, pBlob->nUsed);
    pBlob->aData = pNew;
    pBlob->xRealloc = blobReallocMalloc;
    pBlob->nAlloc = newSize;
  }
}

/*
** Return a pointer to a null-terminated string for a blob.
*/
char *blob_str(Blob *p){
  blob_is_init(p);
  if( p->nUsed==0 ){
    blob_append_char(p, 0); /* NOTE: Changes nUsed. */
    p->nUsed = 0;
  }
  if( p->nUsed<p->nAlloc ){
    p->aData[p->nUsed] = 0;
  }else{
    blob_materialize(p);
  }
  return p->aData;
}

/*
** Make sure a blob is nul-terminated and is not a pointer to unmanaged
** space.  Return a pointer to the data.
*/
char *blob_materialize(Blob *pBlob){
  blob_resize(pBlob, pBlob->nUsed);
  return pBlob->aData;
}

/*
** Append text or data to the end of a blob.  Or, if pBlob==NULL, send
** the text to standard output in terminal mode, or to standard CGI output
** in CGI mode.
**
** If nData<0 then output all of aData up to the first 0x00 byte.
**
** Use the blob_append() routine in all application code.  The blob_append()
** routine is faster, but blob_append_full() handles all the corner cases.
** The blob_append() routine automatically calls blob_append_full() if
** necessary.
*/
static void blob_append_full(Blob *pBlob, const char *aData, int nData){
  sqlite3_int64 nNew;
  /* assert( aData!=0 || nData==0 ); // omitted for speed */
  /* blob_is_init(pBlob); // omitted for speed */
  if( nData<0 ) nData = strlen(aData);
  if( nData==0 ) return;
  if( pBlob==0 ){
#if 0 /* libfsl */
    if( g.cgiOutput ){
      pBlob = cgi_output_blob();
    }else{
#endif /* libfsl */
      fossil_puts(aData, 0, nData);
      return;
#if 0 /* libfsl */
    }
#endif
  }
  nNew = pBlob->nUsed;
  nNew += nData;
  if( nNew >= pBlob->nAlloc ){
    nNew += pBlob->nAlloc;
    nNew += 100;
    blob_assert_safe_size(nNew);
    pBlob->xRealloc(pBlob, (unsigned)nNew);
    if( pBlob->nUsed + nData >= pBlob->nAlloc ){
      blob_panic();
    }
  }
  memcpy(&pBlob->aData[pBlob->nUsed], aData, nData);
  pBlob->nUsed += nData;
  pBlob->aData[pBlob->nUsed] = 0;   /* Blobs are always nul-terminated */
}

/*
** Append text or data to the end of a blob.  Or, if pBlob==NULL, send
** the text to standard output in terminal mode, or to standard CGI output
** in CGI mode.
**
** If nData<0 then output all of aData up to the first 0x00 byte.
**
** Use the blob_append() routine in all application code.  The blob_append()
** routine is faster, but blob_append_full() handles all the corner cases.
** The blob_append() routine automatically calls blob_append_full() if
** necessary.
*/
void blob_append(Blob *pBlob, const char *aData, int nData){
  sqlite3_int64 nUsed;
  /* assert( aData!=0 || nData==0 ); // omitted for speed */
  if( nData<=0 || pBlob==0 || pBlob->nUsed + nData >= pBlob->nAlloc ){
    blob_append_full(pBlob, aData, nData);
    return;
  }
  nUsed = pBlob->nUsed;
  pBlob->nUsed += nData;
  pBlob->aData[pBlob->nUsed] = 0;
  memcpy(&pBlob->aData[nUsed], aData, nData);
}

/*
** Append a single character to the blob.  If pBlob is zero then the
** character is written directly to stdout.
*/
void blob_append_char(Blob *pBlob, char c){
  if( pBlob==0 || pBlob->nUsed+1 >= pBlob->nAlloc ){
    blob_append_full(pBlob, &c, 1);
  }else{
    pBlob->aData[pBlob->nUsed++] = c;
  }
}

/*
** A reallocation function that assumes that aData came from malloc().
** This function attempts to resize the buffer of the blob to hold
** newSize bytes.
**
** No attempt is made to recover from an out-of-memory error.
** If an OOM error occurs, an error message is printed on stderr
** and the program exits.
*/
void blobReallocMalloc(Blob *pBlob, unsigned int newSize){
  if( newSize==0 ){
    free(pBlob->aData);
    pBlob->aData = 0;
    pBlob->nAlloc = 0;
    pBlob->nUsed = 0;
    pBlob->iCursor = 0;
    pBlob->blobFlags = 0;
  }else if( newSize>pBlob->nAlloc || newSize+4000<pBlob->nAlloc ){
    char *pNew;
    blob_assert_safe_size((i64)newSize);
    pNew = fossil_realloc(pBlob->aData, newSize);
    pBlob->aData = pNew;
    pBlob->nAlloc = newSize;
    if( pBlob->nUsed>pBlob->nAlloc ){
      pBlob->nUsed = pBlob->nAlloc;
    }
  }
}

/*
** Attempt to resize a blob so that its internal buffer is
** nByte in size.  The blob is truncated if necessary.
*/
void blob_resize(Blob *pBlob, unsigned int newSize){
  pBlob->xRealloc(pBlob, newSize+1);
  pBlob->nUsed = newSize;
  pBlob->aData[newSize] = 0;
}

/*
** Initialize a blob to the data on an input channel.  Return
** the number of bytes read into the blob.  Any prior content
** of the blob is discarded, not freed.
*/
int blob_read_from_channel(Blob *pBlob, FILE *in, int nToRead){
  size_t n;
  blob_zero(pBlob);
  if( nToRead<0 ){
    char zBuf[10000];
    while( !feof(in) ){
      n = fread(zBuf, 1, sizeof(zBuf), in);
      if( n>0 ){
        blob_append(pBlob, zBuf, n);
      }
    }
  }else{
    blob_resize(pBlob, nToRead);
    n = fread(blob_buffer(pBlob), 1, nToRead, in);
    blob_resize(pBlob, n);
  }
  return blob_size(pBlob);
}

/*
** Initialize a blob to an empty string.
*/
void blob_zero(Blob *pBlob){
  static const char zEmpty[] = "";
  assert_blob_is_reset(pBlob);
  pBlob->nUsed = 0;
  pBlob->nAlloc = 1;
  pBlob->aData = (char*)zEmpty;
  pBlob->iCursor = 0;
  pBlob->blobFlags = 0;
  pBlob->xRealloc = blobReallocStatic;
}

/*
** Do printf-style string rendering and append the results to a blob.  Or
** if pBlob==0, do printf-style string rendering directly to stdout.
**
** The blob_appendf() version sets the BLOBFLAG_NotSQL bit in Blob.blobFlags
** whereas blob_append_sql() does not.
*/
void blob_vappendf(Blob *pBlob, const char *zFormat, va_list ap){
  vxprintf(pBlob, zFormat, ap);
}

/*
** Reset a blob to be an empty container.
*/
void blob_reset(Blob *pBlob){
  blob_is_init(pBlob);
  pBlob->xRealloc(pBlob, 0);
}

/*
** Initialize a blob to a string or byte-array constant of a specified length.
** Any prior data in the blob is discarded.
*/
void blob_init(Blob *pBlob, const char *zData, int size){
  assert_blob_is_reset(pBlob);
  if( zData==0 ){
    *pBlob = empty_blob;
  }else{
    if( size<=0 ) size = strlen(zData);
    pBlob->nUsed = pBlob->nAlloc = size;
    pBlob->aData = (char*)zData;
    pBlob->iCursor = 0;
    pBlob->blobFlags = 0;
    pBlob->xRealloc = blobReallocStatic;
  }
}

/*
** Do printf-style string rendering and append the results to a blob.  Or
** if pBlob==0, do printf-style string rendering directly to stdout.
**
** The blob_appendf() version sets the BLOBFLAG_NotSQL bit in Blob.blobFlags
** whereas blob_append_sql() does not.
*/
void blob_append_sql(Blob *pBlob, const char *zFormat, ...){
  va_list ap;
  va_start(ap, zFormat);
  vxprintf(pBlob, zFormat, ap);
  va_end(ap);
}

/*
** Return a pointer to a null-terminated string for a blob that has
** been created using blob_append_sql() and not blob_appendf().  If
** text was ever added using blob_appendf() then throw an error.
*/
char *blob_sql_text(Blob *p){
  blob_is_init(p);
  if( (p->blobFlags & BLOBFLAG_NotSQL) ){
#if 0 /* libfsl */
    fossil_panic("use of blob_appendf() to construct SQL text");
#else
    printf("use of blob_appendf() to construct SQL text\n");
    exit (1);
#endif
  }
  return blob_str(p);
}

