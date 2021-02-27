/*
** Copyright (c) 2019-2021 Nikola Kolev <koue@chaosophia.net>
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
** Bail out
*/
void fossil_panic(const char *c){
  fprintf(stderr, "%s\n", c);
  exit(1);
}

/*
** Malloc and free routines that cannot fail
*/
void *fossil_malloc(size_t n){
  void *p = malloc(n==0 ? 1 : n);
  if( p==0 ) fossil_panic("out of memory");
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
  if( p==0 ) fossil_panic("out of memory");
  return p;
}

/*
** We find that the built-in isspace() function does not work for
** some international character sets.  So here is a substitute.
*/
int fossil_isspace(char c){
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
