/*
 * Copyright (c) 2019-2020 Nikola Kolev <koue@chaosophia.net>
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
#include "cez_test.h"

typedef struct array {
	char *type;
	char *before;
	char *after;
} array;

static const array fmt[] = {
//	{ "%d", "black sheep wall", "black sheep wall" },
//	{ "%g", "black sheep wall", "black sheep wall" },
//	{ "%z", "black sheep wall", "black sheep wall" },
	{ "%q", "black'sheep'wall", "black''sheep''wall" },
	{ "%Q", "black sheep wall", "'black sheep wall'" },
//	{ "%b", "black sheep wall", "black sheep wall" },
//	{ "%B", "black sheep wall", "black sheep wall" },
//	{ "%W", "black sheep wall", "black sheep wall" },
//	{ "%h", "black sheep wall", "black sheep wall" },
//	{ "%R", "black sheep wall", "black sheep wall" },
//	{ "%t", "black sheep wall", "black sheep wall" },
//	{ "%T", "black sheep wall", "black sheep wall" },
	{ "%w", "black sheep wall", "black sheep wall" },
//	{ "%F", "black sheep wall", "black sheep wall" },
	{ "%S", "black sheep wall", "black shee" },
//	{ "%j", "black sheep wall", "black sheep wall" },
//	{ "%c", "black sheep wall", "black sheep wall" },
//	{ "%o", "black sheep wall", "black sheep wall" },
//	{ "%u", "black sheep wall", "black sheep wall" },
//	{ "%x", "black sheep wall", "black sheep wall" },
//	{ "%X", "black sheep wall", "black sheep wall" },
//	{ "%f", "black sheep wall", "black sheep wall" },
//	{ "%e", "black sheep wall", "black sheep wall" },
//	{ "%E", "black sheep wall", "black sheep wall" },
//	{ "%G", "black sheep wall", "black sheep wall" },
//	{ "%i", "black sheep wall", "black sheep wall" },
//	{ "%n", "black sheep wall", "black sheep wall" },
	{ "%%", "black sheep wall", "%" },
//	{ "%p", "black sheep wall", "black sheep wall" },
	{ "%/", "black sheep wall", "black sheep wall" },
};

int
main(void)
{
	char *z;

	cez_test_start();
	for (int i = 0; i < sizeof(fmt)/24; i++) {
		z = mprintf(fmt[i].type, fmt[i].before);
		assert(strcmp(z, fmt[i].after) == 0);
		free(z);
	}

	return (0);
}
