/*
 * libfdt - Flat Device Tree manipulation
 *	Testcase common utility functions
 * Copyright (C) 2006 David Gibson, IBM Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#define _GNU_SOURCE /* for strsignal() */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include <libfdt.h>

#include "tests.h"

int verbose_test = 1;
char *test_name;

void  __attribute__((weak)) cleanup(void)
{
}

static void sigint_handler(int signum, siginfo_t *si, void *uc)
{
	cleanup();
	fprintf(stderr, "%s: %s (pid=%d)\n", test_name,
		strsignal(signum), getpid());
	exit(RC_BUG);
}

void test_init(int argc, char *argv[])
{
	int err;
	struct sigaction sa_int = {
		.sa_sigaction = sigint_handler,
	};

	test_name = argv[0];

	err = sigaction(SIGINT, &sa_int, NULL);
	if (err)
		FAIL("Can't install SIGINT handler");

	if (getenv("QUIET_TEST"))
		verbose_test = 0;

	verbose_printf("Starting testcase \"%s\", pid %d\n",
		       test_name, getpid());
}


struct errtabent {
	const char *str;
};

#define ERRTABENT(val) \
	[(val)] = { .str = #val, }

static struct errtabent errtable[] = {
	ERRTABENT(FDT_ERR_OK),
	ERRTABENT(FDT_ERR_BADMAGIC),
	ERRTABENT(FDT_ERR_BADVERSION),
	ERRTABENT(FDT_ERR_BADPOINTER),
	ERRTABENT(FDT_ERR_BADHEADER),
	ERRTABENT(FDT_ERR_BADSTRUCTURE),
	ERRTABENT(FDT_ERR_BADOFFSET),
	ERRTABENT(FDT_ERR_NOTFOUND),
	ERRTABENT(FDT_ERR_BADPATH),
	ERRTABENT(FDT_ERR_TRUNCATED),
	ERRTABENT(FDT_ERR_NOSPACE),
	ERRTABENT(FDT_ERR_BADSTATE),
	ERRTABENT(FDT_ERR_SIZE_MISMATCH),
	ERRTABENT(FDT_ERR_INTERNAL),
};

#define ERRTABSIZE	(sizeof(errtable) / sizeof(errtable[0]))

const char *fdt_strerror(int errval)
{
	if ((errval >= 0) && (errval < ERRTABSIZE))
		return errtable[errval].str;
	else
		return "Unknown FDT error code";
}

void check_property(struct fdt_header *fdt, int nodeoffset, const char *name,
		    int len, const void *val)
{
	int offset;
	const struct fdt_property *prop;
	uint32_t tag, nameoff, proplen;
	const char *propname;
	int err;

	verbose_printf("Checking property \"%s\"...", name);
	offset = fdt_property_offset(fdt, nodeoffset, name);
	verbose_printf("offset %d...", offset);
	if ((err = fdt_offset_error(offset)))
		FAIL("fdt_property_offset(\"%s\"): %s", name,
		     fdt_strerror(err));

	prop = fdt_offset_ptr_typed(fdt, offset, prop);
	verbose_printf("pointer %p\n", prop);
	if (! prop)
		FAIL("NULL retreiving \"%s\" pointer", name);

	tag = fdt32_to_cpu(prop->tag);
	nameoff = fdt32_to_cpu(prop->nameoff);
	proplen = fdt32_to_cpu(prop->len);

	if (tag != FDT_PROP)
		FAIL("Incorrect tag 0x%08x on property \"%s\"", tag, name);

	propname = fdt_string(fdt, nameoff);
	if (!propname || !streq(propname, name))
		FAIL("Property name mismatch \"%s\" instead of \"%s\"",
		     propname, name);
	if (proplen != len)
		FAIL("Size mismatch on property \"%s\": %d insead of %d",
		     name, proplen, len);
	if (memcmp(val, prop->data, len) != 0)
		FAIL("Data mismatch on property \"%s\"", name);
	
}

void *check_getprop(struct fdt_header *fdt, int nodeoffset, const char *name,
		    int len, const void *val)
{
	void *propval;
	int proplen;
	int err;

	propval = fdt_getprop(fdt, nodeoffset, name, &proplen);
	if ((err = fdt_ptr_error(propval)))
		FAIL("fdt_getprop(\"%s\"): %s", name, fdt_strerror(err));

	if (proplen != len)
		FAIL("Size mismatch on property \"%s\": %d insead of %d",
		     name, proplen, len);
	if (memcmp(val, propval, len) != 0)
		FAIL("Data mismatch on property \"%s\"", name);

	return propval;
}
