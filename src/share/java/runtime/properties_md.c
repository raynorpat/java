/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
#include "typedefs.h"
#include <string.h>

#include <stdlib.h>
#if defined(XP_PC) && defined(_WIN32)
#include <windows.h>
#include <locale.h>
#include "locale115.h"
#endif
#ifdef XP_UNIX
#include <unistd.h>
#endif

#include "timeval.h"

#if !defined(XP_PC) && !defined(XP_MAC)
#include <sys/time.h>
#endif

#ifdef XP_MAC
#include <unistd.h>
#include <ConditionalMacros.h>
#include "prmacos.h"
#endif

#include "sys_api.h"
#include "oobj.h"
#include "java_lang_System.h"
#include "path_md.h"	/* sysJavaHome */

/*
 * This routine initializes the system properties with
 * all the system defined properties.
 *
 * REMIND: We need to obtain some of these values in a more
 * organized way.
 */

#define PUTPROP(props, key, val) \
    execute_java_dynamic_method(0, (HObject *)(props), \
				"put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;",\
				makeJavaString((key), strlen(key)),\
				makeJavaString((char *)(val), strlen(val)))

char **user_props = 0;

#if defined(XP_PC) && defined(_WIN32)
/* Windows locale parsing and timezone selection: javai_g 1003C6E0. */
static const char *
win32PropertyValue(const Win32PropertyMapping *mapping, const char *name,
                   int ignoreCase)
{
    for (; mapping->name; mapping++) {
        if (!(ignoreCase ? _stricmp(mapping->name, name) :
                           strcmp(mapping->name, name))) return mapping->value;
    }
    return NULL;
}

static void
win32LocaleProperties(struct Hjava_util_Properties *props)
{
    const char *source = setlocale(LC_CTYPE, "");
    const char *language = NULL, *region = NULL, *encoding = NULL, *zone = NULL;
    char localeName[256], localeKey[256], zoneName[33];
    char *separator, *dot;
    TIME_ZONE_INFORMATION information;
    DWORD status;
    int i;

    if (source && strlen(source) < sizeof(localeName)) {
        strcpy(localeName, source);
        separator = strchr(localeName, '_');
        if (separator) {
            *separator++ = 0;
            dot = strchr(separator, '.');
            if (dot) *dot = 0;
        }
        language = win32PropertyValue(win32LanguageMappings, localeName, 1);
        if (language) {
            if (separator) {
                region = win32PropertyValue(win32RegionMappings, separator, 1);
                if (!region) region = separator;
                PUTPROP(props, "user.region", region);
                sprintf(localeKey, "%s_%s", language, region);
            } else strcpy(localeKey, language);
            encoding = win32PropertyValue(win32EncodingMappings, localeKey, 0);
            if (!encoding) encoding = win32PropertyValue(win32EncodingMappings, language, 0);
        }
    }
    PUTPROP(props, "user.language", language ? language : "en");
    PUTPROP(props, "file.encoding", encoding ? encoding : "8859_1");

    memset(&information, 0, sizeof(information));
    status = GetTimeZoneInformation(&information);
    for (i = 0; i < 32; i++) zoneName[i] = (char)information.StandardName[i];
    zoneName[32] = 0;
    if (status != TIME_ZONE_ID_UNKNOWN)
        zone = win32PropertyValue(win32TimezoneMappings, zoneName, 0);
    PUTPROP(props, "user.timezone", zone ? zone : "GMT");
}
#endif

struct Hjava_util_Properties *
java_lang_System_initProperties(struct Hjava_lang_System *this, 
				struct Hjava_util_Properties *props) 
{
    char *v;
    char buf[512];

    /* java properties */
    PUTPROP(props, "java.version", "1.1.5");
    PUTPROP(props, "java.vendor", "Sun Microsystems Inc.");
    PUTPROP(props, "file.encoding.pkg", "sun.io");
    PUTPROP(props, "java.vendor.url", "http://www.sun.com/");
    PUTPROP(props, "java.home", sysJavaHome());
    sprintf(buf, "%d.%d", JAVA_VERSION, JAVA_MINOR_VERSION);
    PUTPROP(props, "java.class.version", buf);
    v = (char*)CLASSPATH();
    PUTPROP(props, "java.class.path", v ? v : "");

    /* os properties */
    sysOSInfo(buf, 256, buf + 256, sizeof(buf) - 256);
    PUTPROP(props, "os.name", buf);
    PUTPROP(props, "os.version", buf + 256);
    PUTPROP(props, "os.arch", "x86");

    /* file system properties */
#ifdef XP_UNIX
    PUTPROP(props, "file.separator", "/");
    PUTPROP(props, "path.separator", ":");
    PUTPROP(props, "line.separator", "\n");

    PUTPROP(props, "awt.font.symbol", "ZapfDingbats");
#endif
#ifdef XP_PC
    PUTPROP(props, "file.separator", "\\");
    PUTPROP(props, "path.separator", ";");
    PUTPROP(props, "line.separator", "\r\n");

    PUTPROP(props, "awt.font.symbol", "ZapfDingbats");
#endif
#ifdef XP_MAC
    PUTPROP(props, "file.separator", "/");
    PUTPROP(props, "path.separator", ":");
    PUTPROP(props, "line.separator", "\n");    /* XXX what's the right thing*/
#endif

    /* user properties */
    v = getenv("USER");
    PUTPROP(props, "user.name", v ? v : "?");
    v = getenv("HOME");
    PUTPROP(props, "user.home", v ? v : "/");
    getcwd(buf, sizeof(buf));
    PUTPROP(props, "user.dir", buf);

    /* toolkit properties */
#ifdef XP_UNIX
    PUTPROP(props, "awt.toolkit", "sun.awt.motif.MToolkit");
#endif
#ifdef XP_PC
    v = getenv("JAVA_AWT");
    PUTPROP(props, "awt.toolkit", v ? v : "sun.awt.windows.WToolkit");
#endif
#ifdef XP_MAC
    PUTPROP(props, "awt.toolkit", "sun.awt.macos.MToolkit");
#endif
    PUTPROP(props, "awt.appletWarning", "Unsigned Java Applet Window");

#if defined(XP_MAC) || (defined(XP_PC) && !defined(_WIN32))
    PUTPROP(props, "awt.imagefetchers", "1");
#else 
    PUTPROP(props, "awt.imagefetchers", "4");
#endif


#if defined(XP_PC) && defined(_WIN32)
    win32LocaleProperties(props);
#endif

    /* user defined properties, set by main */
    if (user_props != 0) {
	int i;
	for (i = 0 ; user_props[i] && user_props[i+1] ; i += 2) {
	    PUTPROP(props, user_props[i], user_props[i+1]);
	}
    }

    return props;
}
