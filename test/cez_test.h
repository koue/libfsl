/*
** Copyright (c) 2018 Nikola Kolev <koue@chaosophia.net>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the Simplified BSD License (also
** known as the "2-Clause License" or "FreeBSD License".)

** This program is distributed in the hope that it will be useful,
** but without any warranty; without even the implied warranty of
** merchantability or fitness for a particular purpose.
**
*******************************************************************************
*/

#ifndef CEZ_TEST_H
#define CEZ_TEST_H

#include <assert.h>

#define CEZ_TEST_BLOCK(block) do {\
	block\
} while(0)
#define cez_test_start() CEZ_TEST_BLOCK(\
	printf("Run %30s\n", __FILE__);\
)

#endif
