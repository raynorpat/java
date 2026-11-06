/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/*
 * @(#)profiler.c	1.17 95/11/29  
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

/*-
 *      code for collecting profiling statistics on Java code.
 */

#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>

#include "timeval.h"
#include "oobj.h"
#include "interpreter.h"
#include "sys_api.h"
#include "monitor.h"
#include "profile.h"

typedef struct {
    struct methodblock *caller, *callee;
    int32_t count;
    int32_t time;
} javamon_entry;


#define JAVAMON_TABLE_SIZE   10001
static javamon_entry *javamon_table;
static int javamon_table_count;
int java_monitor = 0;
/* Sun10039A31 defaults this to java.prof on the first report. */
char *prof_file;
extern sys_mon_t *_heap_lock;

void javamon(int i)
{
    java_monitor = i;
    if (java_monitor) {
	javamon_table = (javamon_entry *)sysMalloc(JAVAMON_TABLE_SIZE * sizeof(javamon_entry));
	memset((char *)javamon_table, 0, JAVAMON_TABLE_SIZE * sizeof(javamon_entry));
	javamon_table_count = 0;
    }
}

void
java_mon(struct methodblock *caller, struct methodblock *callee, int time)
{
    javamon_entry *tab, *p;
    /* Transitional mapping: serialize table reset with GC's existing lock. */
    sysMonitorEnter(_heap_lock);
    tab = javamon_table;
    p = &tab[((((unsigned long)caller)^((unsigned long)callee)) >> 2) % JAVAMON_TABLE_SIZE];

    while ((p->callee != 0) && ((p->caller != caller) || (p->callee != callee))) {
	if (p-- == tab) {
	    p = &tab[JAVAMON_TABLE_SIZE - 1];
	}
    }
    if (p->callee == 0) {
	if (++javamon_table_count == JAVAMON_TABLE_SIZE) {
	    fprintf(stderr, "profile table overflow");
	    sysExit(1);
	}
	p->caller = caller;
	p->callee = callee;
    }
    p->time += time;
    p->count++;
    sysMonitorExit(_heap_lock);
}

static void
java_mon_dump_to_file(FILE *fp)
{
    javamon_entry *tab = javamon_table;
    javamon_entry *p = &tab[JAVAMON_TABLE_SIZE];

    fprintf(fp, "# count callee caller time\n");
    while (p-- != tab) {
	if (p->callee != 0) {
	    fprintf(fp, "%d ", p->count);
	    if (p->callee == (struct methodblock *)-1) {
		fprintf(fp, "java/lang/System.gc()V ");
	    } else if ((strcmp(cbName(fieldclass(&p->callee->fb)), "java/lang/System") != 0) ||
		       (strcmp(fieldname(&p->callee->fb), "gc") != 0)) {
		fprintf(fp, "%s.%s%s ", cbName(fieldclass(&p->callee->fb)), 
			fieldname(&p->callee->fb),
			fieldsig(&p->callee->fb));
	    }
	    if (p->caller != 0) {
		fprintf(fp, "%s.%s%s ", cbName(fieldclass(&p->caller->fb)), 
					fieldname(&p->caller->fb),
		    			fieldsig(&p->caller->fb));
	    } else {
		fprintf(fp, "?.? ");
	    }
	    fprintf(fp, "%d\n", p->time);
	}
    }
}

void
java_mon_dump()
{
    extern void prof_heap(FILE *fp);
    FILE *fp;
    if (!prof_file) prof_file = "java.prof";
    fp = fopen(prof_file, "w");

    if (fp == 0) {
	fprintf(stderr, "Can't write to %s\n", prof_file);
	return;
    }

    sysMonitorEnter(_heap_lock);
    java_mon_dump_to_file(fp);
    prof_heap(fp);
    sysMonitorExit(_heap_lock);
    fclose(fp);
}

/* javai_g.dll10039D08/10039D39/10039D4E/10039D7A. */
void sun_misc_VM_resumeJavaMonitor(void *unused)
{
    sysMonitorEnter(_heap_lock);
    if (!javamon_table) javamon(1); else java_monitor = 1;
    sysMonitorExit(_heap_lock);
}
void sun_misc_VM_suspendJavaMonitor(void *unused)
{ java_monitor = 0; }
void sun_misc_VM_resetJavaMonitor(void *unused)
{
    int enabled;
    sysMonitorEnter(_heap_lock);
    enabled = java_monitor;
    if (javamon_table) sysFree(javamon_table);
    javamon(1);
    java_monitor = enabled;
    sysMonitorExit(_heap_lock);
}
void sun_misc_VM_writeJavaMonitorReport(void *unused)
{ java_mon_dump(); }
