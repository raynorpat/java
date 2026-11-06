/* Isolated ID-number regression. SUN_ORACLE targets only the supplied Sun
 * Win32 Debug javai_g.dll: its private Str2ID is at RVA 0x4e541. It uses a
 * fresh local table, not VM globals. The normal variant links classhash115,
 * util and arena. Monitor stubs intentionally exclude concurrency coverage;
 * unrelated util entry points abort if unexpectedly reached. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef SUN_ORACLE
#include <windows.h>
typedef unsigned short (__cdecl *str_id_fn)(void **, char *, void ***, int);
static str_id_fn strid;
static void *table;
static unsigned int NameAndTypeToHash(char *name, char *type) {
 unsigned short a=strid(&table,name,0,1),b=strid(&table,type,0,1);
 if(!a||!b) abort();
 return ((unsigned int)a<<16)+b;
}
#else
#include "tree.h"
#include "interpreter.h"
#include "classhash115.h"
int sysMonitorSizeof(void) { return 4; }
void monitorRegister(sys_mon_t *m,char *name) {(void)m;(void)name;}
int sysMonitorEnter(sys_mon_t *m) {(void)m;return 0;}
int sysMonitorExit(sys_mon_t *m) {(void)m;return 0;}
void SignalError(ExecEnv *ee,char *name,char *detail) {(void)ee;(void)name;(void)detail;abort();}
void sysAbort(void) {abort();}
void ll2str(int64_t n,char *s,char *limit) {(void)n;(void)s;(void)limit;abort();}
unicode next_utf2unicode(char **s) {(void)s;abort();return 0;}
#endif
int main(int argc,char **argv) {
 unsigned int i,id,first,digest=2166136261u;
 char name[40];
#ifdef SUN_ORACLE
 HMODULE module;
 if(argc!=2)return 2;
 module=LoadLibraryA(argv[1]); if(!module)return 3;
 strid=(str_id_fn)((char*)module+0x4e541);
#else
 (void)argc;(void)argv;
 if(!initializeClassHash115())return 4;
#endif
 first=NameAndTypeToHash("alpha","(I)I");
 printf("first=%08x repeat=%08x type=%08x\n",first,NameAndTypeToHash("alpha","(I)I"),NameAndTypeToHash("alpha","(J)J"));
 for(i=0;i<5000;i++) {
  sprintf(name,"member_%04u",i);
  id=NameAndTypeToHash(name,(i&1)?"(I)I":"(J)J");
  digest=(digest^id)*16777619u;
 }
 printf("digest=%08x first=%08x utf=%08x\n",digest,NameAndTypeToHash("alpha","(I)I"),NameAndTypeToHash("\xc3\xa9", "()V"));
#ifndef SUN_ORACLE
 if(strcmp(IDToType(first),"(I)I")||strcmp(IDToNameAndTypeString(first),"alpha (I)I"))return 5;
#endif
 return 0;
}

