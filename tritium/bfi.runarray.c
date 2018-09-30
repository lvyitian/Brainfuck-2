
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include "bfi.tree.h"
#include "bfi.run.h"
#include "bfi.runarray.h"
#include "clock.h"

#define FNAME run_intprog
#define icell   unsigned int
#include "bfi.execloop.h"
#undef FNAME
#undef icell

#define FNAME run_charprog
#define icell   unsigned char
#include "bfi.execloop.h"
#undef FNAME
#undef icell

#define FNAME run_maskprog
#define icell   unsigned int
#define DYNAMIC_MASK
#include "bfi.execloop.h"
#undef FNAME
#undef icell
#undef DYNAMIC_MASK

void
convert_tree_to_runarray(void)
{
    struct bfi * n = bfprog;
    size_t arraylen = 0;
    int * progarray = 0;
    int * p;
    int last_offset = 0;

    only_uses_putch = 1;

    while(n)
    {
	switch(n->type)
	{
	case T_MOV:
	    break;

	case T_CALC: case T_CALCMULT:
	    arraylen += 7;
	    break;

	default:
	    arraylen += 3;
	    break;
	}
	n = n->next;
    }

    p = progarray = calloc(arraylen+2, sizeof*progarray);
    if (!progarray) { perror("calloc"); exit(1); }
    n = bfprog;

    last_offset = 0;
    while(n)
    {
#ifdef ARRAY_DUMP
	int * pp = p;
#endif

	if (n->type != T_MOV) {
	    *p++ = (n->offset - last_offset);
	    last_offset = n->offset;
	}
	else {
	    last_offset -= n->count;
	    n = n->next;
	    continue;
	}

	*p++ = n->type;
	switch(n->type)
	{
	case T_INP: case T_PRT:
	    break;

	case T_CHR: case T_ADD: case T_SET:
	    *p++ = n->count;
	    break;

	case T_MULT: case T_CMULT:
	    /* Note: for these I could generate multiply code for then enclosed
	     * T_ADD instructions, but that will only happen if the optimiser
	     * section is turned off.
	     */
	    /*FALLTHROUGH*/

	case T_WHL:
	    if (opt_level>=1) {
		struct bfi *n1, *n2=0, *n3=0, *n4=0;
		n1 = n->next;
		if (n1) n2 = n1->next;
		if (n2) n3 = n2->next;
		if (n3) n4 = n3->next;

		if (n1->type == T_MOV && n1->count != 0){
		    /* Look for [<<<] */

		    if (n2->type == T_END) {
			p[-1] = T_ZFIND;
			*p++ = n1->count;
			n = n->jmp;
			break;
		    }
		}

		if (n1->type == T_ADD && n2->type == T_MOV && n3->type == T_END) {
		    p[-1] = T_ADDWZ;
		    *p++ = n1->offset - last_offset;
		    *p++ = n1->count;
		    *p++ = n2->count;
		    n = n->jmp;
		    break;
		}

		if (n1->type == T_ADD && n2->type == T_ADD &&
		    n3->type == T_MOV && n4->type == T_END &&
		    n1->count == -1 && n2->count == 1 &&
		    n->offset == n1->offset &&
		    n->offset == n2->offset - n3->count
		) {
		    p[-1] = T_MFIND;
		    *p++ = n3->count;
		    n = n->jmp;
		    break;
		}
	    }

	    /* Storing the location of the instruction in the T_END's count
	     * field; it's not normally used */
	    n->jmp->count = p-progarray;
	    *p++ = 0;
	    break;

	case T_IF:
	    p[-1] = T_WHL;
	    n->jmp->count = p-progarray;
	    *p++ = 0;
	    break;

	case T_ENDIF:
	    if (p[-2] == 0) p -= 2;
	    progarray[n->count] = (p-progarray) - n->count -1;
	    break;

	case T_END:
	    progarray[n->count] = (p-progarray) - n->count;
	    *p++ = -progarray[n->count];
	    break;

	case T_CALC:
	    if (n->count3 == 0) {
		if (n->count == 0 && n->count2 == 1) {
		    /*  m[off] = m[off2] */
		    p[-1] = T_CALC4;
		    *p++ = n->offset2 - last_offset;
		} else {
		    /*  m[off] = m[off2]*count2 + count2 */
		    p[-1] = T_CALC2;
		    *p++ = n->count;
		    *p++ = n->offset2 - last_offset;
		    *p++ = n->count2;
		}
	    } else if ( n->count == 0 && n->count2 == 1 && n->offset == n->offset2 ) {
		if (n->count3 == 1) {
		    /*  m[off] += m[off3] */
		    p[-1] = T_CALC5;
		    *p++ = n->offset3 - last_offset;
		} else {
		    /*  m[off] += m[off3]*count3 */
		    p[-1] = T_CALC3;
		    *p++ = n->offset3 - last_offset;
		    *p++ = n->count3;
		}
	    } else {
		*p++ = n->count;
		*p++ = n->offset2 - last_offset;
		*p++ = n->count2;
		*p++ = n->offset3 - last_offset;
		*p++ = n->count3;
	    }
	    break;

	case T_CALCMULT:
	    *p++ = n->count;
	    *p++ = n->offset2 - last_offset;
	    *p++ = n->count2;
	    *p++ = n->offset3 - last_offset;
	    *p++ = n->count3;
	    break;

	case T_STOP: case T_NOP:
	    break;

	default:
	    fprintf(stderr, "Invalid node type found = %s\n", tokennames[n->type]);
	    exit(1);
	}

#ifdef ARRAY_DUMP
	if (verbose>1)
	{
	    if (pp[0])
		fprintf(stderr, "T_MOV(%d): ", pp[0]);

	    if (pp[1] >= 0 && pp[1] < TCOUNT)
		fprintf(stderr, "%s", tokennames[pp[1]]);
	    else
		fprintf(stderr, "TOKEN(%d)", pp[1]);

	    for(pp+=2; pp<p; pp++) fprintf(stderr, " %d", pp[0]);

	    if(n->line || n->col)
		fprintf(stderr, " @(%d,%d)", n->line, n->col);

	    fprintf(stderr, "\n");
	}
#endif

	n = n->next;
    }
    *p++ = 0;
    *p++ = T_STOP;

    delete_tree();
    start_runclock();
    if (cell_size == CHAR_BIT) {
	run_charprog(progarray, map_hugeram());
    } else if (cell_size == sizeof(int)*CHAR_BIT) {
	run_intprog(progarray, map_hugeram());
    } else {
	run_maskprog(progarray, map_hugeram());
    }
    finish_runclock(&run_time, &io_time);
    free(progarray);
}

