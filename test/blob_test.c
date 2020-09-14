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

int
main(void)
{
	Blob mystr = empty_blob;
	const char teststr[] = "black sheep wall";

	cez_test_start();
	assert(blob_size(&mystr) == 0);
	blob_append_char(&mystr, 88);
	assert(blob_size(&mystr) == 1);
	assert(strcmp(blob_str(&mystr), "X") == 0);
	blob_zero(&mystr);
	assert(blob_size(&mystr) == 0);
	blob_append(&mystr, teststr, strlen(teststr));
	assert(blob_size(&mystr) == 16);
	assert(strcmp(blob_str(&mystr), teststr) == 0);
	blob_reset(&mystr);
	assert(blob_size(&mystr) == 0);
	blob_init(&mystr, "", 100);
	assert(blob_size(&mystr) == 100);
	blob_append(&mystr, teststr, strlen(teststr));
	assert(blob_size(&mystr) == 116);
	assert(strlen(blob_str(&mystr)) == 0);
	blob_resize(&mystr, 10);
	assert(blob_size(&mystr) == 10);
	assert(strlen(blob_str(&mystr)) == 0);
	blob_zero(&mystr);
	blob_append(&mystr, teststr, strlen(teststr));
	blob_resize(&mystr, 5);
	assert(strcmp(blob_str(&mystr), "black") == 0);
	blob_zero(&mystr);
	assert(blob_size(&mystr) == 0);

	return (0);
}
