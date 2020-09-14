/*
 * Copyright (c) 2017-2020 Nikola Kolev <koue@chaosophia.net>
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
#include "fslcson.h"
#include "cez_test.h"

int
main(void)
{
	Blob json_list = empty_blob; /* json list of github user info */
	char *command = "cat sample.json";
	FILE *pf;
	cson_parse_opt popt = cson_parse_opt_empty;
	cson_parse_info pinfo = cson_parse_info_empty;
	cson_value * cson_root = NULL;
	cson_object * cson_obj = NULL;
	cson_value * obj_value = NULL;

	cez_test_start();
	pf = popen(command, "r");
	blob_read_from_channel(&json_list, pf, -1);
	pclose(pf);
	assert(cson_parse_string(&cson_root, blob_str(&json_list),
	    strlen(blob_str(&json_list)), &popt, &pinfo) == 0);
	assert((cson_obj = cson_value_get_object(cson_root)) != NULL);
	assert((obj_value = cson_object_get(cson_obj, "after")) != NULL);
	assert(cson_string_cstr(cson_value_get_string(obj_value)));
	assert(cson_object_get(cson_obj, "notexist") == NULL);
	cson_value_free(cson_root);
	blob_reset(&json_list);

	return (0);
}
