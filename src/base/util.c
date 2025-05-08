/*
** Copyright (c) 2019-2025 Nikola Kolev <koue@chaosophia.net>
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
**
** This file contains code for miscellaneous utility routines.
*/

#include "fslbase.h"

/*
** Duplicate a string.
*/
char *fossil_strndup(const char *zOrig, ssize_t len){
  char *z = 0;
  if( zOrig ){
    if( len<0 ) len = strlen(zOrig);
    z = fossil_malloc( len+1 );
    memcpy(z, zOrig, len);
    z[len] = 0;
  }
  return z;
}

/*
** Duplicate a string.
*/
char *fossil_strdup(const char *zOrig){
  return fossil_strndup(zOrig, -1);
}

/*
** Duplicate a string.
*/
char *fossil_strdup_nn(const char *zOrig){
  if( zOrig==0 ) return fossil_strndup("", 0);
  return fossil_strndup(zOrig, -1);
}

/*
** Malloc and free routines that cannot fail
*/
void *fossil_malloc(size_t n){
  void *p = malloc(n==0 ? 1 : n);
#if 0 /* libfsl */
  if( p==0 ) fossil_fatal("out of memory");
#else
  if( p==0 ) {
    printf("out of memory\n");
    exit (1);
  }
#endif
  return p;
}

/*
** Malloc and free routines that cannot fail
*/
void fossil_free(void *p){
  free(p);
}

/*
** Malloc and free routines that cannot fail
*/
void *fossil_realloc(void *p, size_t n){
  p = realloc(p, n);
#if 0 /* libfsl */
  if( p==0 ) fossil_fatal("out of memory");
#else
  if( p==0 ) {
    printf("out of memory\n");
    exit (1);
  }
#endif
  return p;
}

/*
** We find that the built-in isspace() function does not work for
** some international character sets.  So here is a substitute.
** libfsl: blob.c source
*/
static int fossil_isspace(char c){
  return c==' ' || (c<='\r' && c>='\t');
}

/*
** Return true if the input string is NULL or all whitespace.
** Return false if the input string contains text.
*/
int fossil_all_whitespace(const char *z){
  if( z==0 ) return 1;
  while( fossil_isspace(z[0]) ){ z++; }
  return z[0]==0;
}
