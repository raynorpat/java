/* Inject only this process's VM imports, then restore them before returning. */
#include <windows.h>
#include <string.h>
#include "StubPreamble.h"
static char localeName[256];
static BOOL nullLocale;
static WCHAR zoneName[32];
static DWORD zoneStatus;
static char *__cdecl locale(int category,const char *name) { return nullLocale ? NULL:localeName; }
static DWORD WINAPI timezone(LPTIME_ZONE_INFORMATION info) {
    ZeroMemory(info,sizeof(*info));CopyMemory(info->StandardName,zoneName,sizeof(zoneName));return zoneStatus;
}
struct patch { DWORD *slot,previous; };
static BOOL replace(HMODULE module,const char *name,void *function,struct patch *saved)
{
    BYTE *base=(BYTE *)module;
    IMAGE_NT_HEADERS *nt=(IMAGE_NT_HEADERS *)(base+((IMAGE_DOS_HEADER *)base)->e_lfanew);
    IMAGE_IMPORT_DESCRIPTOR *imports=(IMAGE_IMPORT_DESCRIPTOR *)(base+nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    for(;imports->Name;imports++) {
        IMAGE_THUNK_DATA *names,*slots;
        if(!imports->OriginalFirstThunk)continue;
        names=(IMAGE_THUNK_DATA *)(base+imports->OriginalFirstThunk);slots=(IMAGE_THUNK_DATA *)(base+imports->FirstThunk);
        for(;names->u1.AddressOfData;names++,slots++) if(!IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal)) {
            IMAGE_IMPORT_BY_NAME *entry=(IMAGE_IMPORT_BY_NAME *)(base+names->u1.AddressOfData);
            if(!strcmp((char *)entry->Name,name)) {
                DWORD protection;
                if(!VirtualProtect(&slots->u1.Function,sizeof(DWORD),PAGE_READWRITE,&protection))return FALSE;
                saved->slot=&slots->u1.Function;saved->previous=*saved->slot;
                InterlockedExchange((LONG volatile *)saved->slot,(LONG)function);
                VirtualProtect(saved->slot,sizeof(DWORD),protection,&protection);return TRUE;
            }
        }
    }
    return FALSE;
}
static void restore(struct patch *saved)
{
    if(saved->slot) {
        DWORD protection;
        VirtualProtect(saved->slot,sizeof(DWORD),PAGE_READWRITE,&protection);
        InterlockedExchange((LONG volatile *)saved->slot,saved->previous);
        VirtualProtect(saved->slot,sizeof(DWORD),protection,&protection);
    }
}
__declspec(dllexport) stack_item *Java_LocaleProperties115_defaults_stub(stack_item *sp,struct execenv *ee)
{
    typedef stack_item *(__cdecl *Init)(stack_item *,struct execenv *);
    HMODULE module=GetModuleHandleA("javai.dll");
    Init initialize;
    struct patch localePatch={0},zonePatch={0};
    HObject *props;
    stack_item args[1];
    unicode chars[256];int i,length;
    if(!module)module=GetModuleHandleA("javai_g.dll");
    initialize=(Init)GetProcAddress(module,"Java_java_lang_System_initProperties_stub");
    nullLocale=sp[0].p==NULL;
    if(!nullLocale) {
        length=javaStringLength((struct Hjava_lang_String *)sp[0].p);
        if(length>=sizeof(localeName)) {SignalError(ee,"java/lang/RuntimeException","locale fixture too long");return sp+1;}
        javaString2unicode((struct Hjava_lang_String *)sp[0].p,chars,length);
        for(i=0;i<length;i++)localeName[i]=(char)chars[i];localeName[length]=0;
    }
    length=javaStringLength((struct Hjava_lang_String *)sp[1].p);
    if(length>=32) {SignalError(ee,"java/lang/RuntimeException","timezone fixture too long");return sp+1;}
    ZeroMemory(zoneName,sizeof(zoneName));javaString2unicode((struct Hjava_lang_String *)sp[1].p,(unicode *)zoneName,length);
    zoneStatus=sp[2].i;
    props=execute_java_constructor(ee,"java/util/Properties",NULL,"()");
    if(!props)return sp+1;
    if(!initialize || !replace(module,"setlocale",locale,&localePatch) || !replace(module,"GetTimeZoneInformation",timezone,&zonePatch)) {
        restore(&zonePatch);restore(&localePatch);
        SignalError(ee,"java/lang/RuntimeException","locale API fixture incomplete");return sp+1;
    }
    args[0].p=props;initialize(args,ee);
    restore(&zonePatch);restore(&localePatch);sp[0].p=args[0].p;return sp+1;
}
