#ifdef ENABLE_LIBPY
/* This must be first; hopefully nothing else must be first too */
#include <Python.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf2any.h"

/*
 * Python translation from BF, runs at about 18,000,000 instructions per second.
 *
 * There is a limit on the number of nested loops was 20 now 100.
 */

/* #define USEOS // else USESYS */

int ind = 0;
#define I fprintf(ofd, "%*s", ind*4, "")
#define oputs(str) fprintf(ofd, "%s\n", (str))
int tapelen = 0;
int do_dump = 0;

FILE * ofd;
char * pycode = 0;
size_t pycodesize = 0;

int
check_arg(const char * arg)
{
    if (strcmp(arg, "-O") == 0) return 1;
    if (strcmp(arg, "-d") == 0) {
	do_dump = 1;
	return 1;
    } else
#ifdef ENABLE_LIBPY
    if (strcmp(arg, "-r") == 0) {
	do_dump = 0;
	return 1;
    } else
#endif
    if (strncmp(arg, "-M", 2) == 0) {
	tapelen = strtoul(arg+2, 0, 10) + BOFF;
	return 1;
    }
    return 0;
}

void
outcmd(int ch, int count)
{
    switch(ch) {
    case '!':
#ifdef ENABLE_LIBPY
        if (!do_dump)
	    ofd = open_memstream(&pycode, &pycodesize);
	else
#endif
	    ofd = stdout;

	fprintf(ofd, "#!/usr/bin/python\n");
#ifndef USEOS
	fprintf(ofd, "import sys\n");
#else
	fprintf(ofd, "import os\n");
#endif
	if (tapelen>0) {
	    fprintf(ofd, "m = [0] * %d\n", tapelen);
	} else {
	    /* Dynamic arrays are 20% slower! */
	    fprintf(ofd, "from collections import defaultdict\n");
	    fprintf(ofd, "m = defaultdict(int)\n");
	}
	fprintf(ofd, "p = %d\n", BOFF);
	break;

    case '=': I; fprintf(ofd, "m[p] = %d\n", count); break;
    case 'B':
	if(bytecell) { I; fprintf(ofd, "m[p] &= 255\n"); }
	I; fprintf(ofd, "v = m[p]\n");
	break;
    case 'M': I; fprintf(ofd, "m[p] = m[p]+v*%d\n", count); break;
    case 'N': I; fprintf(ofd, "m[p] = m[p]-v*%d\n", count); break;
    case 'S': I; fprintf(ofd, "m[p] = m[p]+v\n"); break;
    case 'Q': I; fprintf(ofd, "if (v != 0) : m[p] = %d\n", count); break;
    case 'm': I; fprintf(ofd, "if (v != 0) : m[p] = m[p]+v*%d\n", count); break;
    case 'n': I; fprintf(ofd, "if (v != 0) : m[p] = m[p]-v*%d\n", count); break;
    case 's': I; fprintf(ofd, "if (v != 0) : m[p] = m[p]+v\n"); break;

    case 'X': I; fprintf(ofd, "raise Exception('Aborting infinite loop')\n"); break;

    case '+': I; fprintf(ofd, "m[p] += %d\n", count); break;
    case '-': I; fprintf(ofd, "m[p] -= %d\n", count); break;
    case '<': I; fprintf(ofd, "p -= %d\n", count); break;
    case '>': I; fprintf(ofd, "p += %d\n", count); break;
    case '[':
	if(bytecell) { I; fprintf(ofd, "m[p] &= 255\n"); }
	I; fprintf(ofd, "while m[p] :\n"); ind++; break;
    case ']':
	if(bytecell) {
	    I; fprintf(ofd, "m[p] &= 255\n");
	}
	ind--;
	break;

#ifndef USEOS
    case '.': I; fprintf(ofd, "sys.stdout.write(chr(m[p]))\n");
	      I; fprintf(ofd, "sys.stdout.flush()\n");
	      break;
    case ',':
	I; fprintf(ofd, "c = sys.stdin.read(1);\n");
	I; fprintf(ofd, "if c != '' :\n");
	ind++; I; ind--; fprintf(ofd, "m[p] = ord(c)\n");
	break;
#else
    case '.': I; fprintf(ofd, "os.write(1, chr(m[p]).encode('ascii'))\n"); break;
    case ',':
	I; fprintf(ofd, "c = os.read(0, 1);\n");
	I; fprintf(ofd, "if c != '' :\n");
	ind++; I; ind--; fprintf(ofd, "m[p] = ord(c)\n");
	break;
#endif
    }

#ifdef ENABLE_LIBPY
    if (!do_dump && ch == '~') {
	fclose(ofd);

	/* The bare interpreter method. Works fine for BF code.
	 */
	Py_Initialize();
	PyRun_SimpleString(pycode);
	Py_Finalize();
    }
#endif
}