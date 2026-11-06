# JDK 1.1.5 launcher evidence

## Sources and observed entry points

`src/share/java/jre/{jre.c,jre.h,jre_main.c}` and
`src/win32/java/jre/{jre_md.c,jre_md.h}` originate from the corresponding
`vendor/jdk1.1.5/demo/jre` sources. Sun's complete license notices are retained.
They use JNI invocation and JNIEnv calls, including DestroyJavaVM; they are
not aliases of the legacy interpreter bootstrap.

IDA of the shipped Windows executables establishes the tool argument vectors:

| Executable | Entry | Prefix before original arguments |
| --- | --- | --- |
| jar.exe | 0x401000 | java, -ms8m, sun.tools.jar.Main |
| javap.exe | 0x401000 | java, sun.tools.javap.JavaP |
| appletviewer.exe | 0x401130 | java, runtime -J options, java.home property, optional sun.tools.ttydebug.TTY, sun.applet.AppletViewer |
| javaw.exe | 0x401000, WinMain 0x401050 | java (replaces original argv[0]) |

Jar and javap forward all original arguments unchanged, including any -J
argument. Appletviewer removes -J prefixes and moves these options before the
class; -debug selects the Java debugger. Its 0x401050 helper appends the home
classes and lib/classes.zip directories to CLASSPATH; absent CLASSPATH does
not add the current directory. JAVA_HOME, if supplied, must denote a directory.
The wrapper class names and flags are from these binaries, not modern JDKs.

## Explicit adaptations

The generic tool and javaw wrappers call the reconstructed java_main entry.
Modern CRT startup supplies the environment instead of assigning the obsolete
imported __initenv variable. java_main exits through sysExit after waiting for
non-daemon threads, as the primary implementation does.
The Windows subsystem target suppresses creation of a console. Path discovery
uses the reconstruction's sysJavaHome; Unix uses native path separators.
Appletviewer diagnostics are equivalent but not claimed byte-identical.

Copied demo JRE adaptations: main returns int and zero after VM shutdown;
WinMain returns main's result without accessing obsolete __initenv; two local
forward declarations have static linkage matching their definitions; JNI
GetProcAddress results have typed JNICALL function pointers; the missing class
name argument in the original missing-main diagnostic is supplied. Remaining
option parsing, VM invocation, registry lookup, and classpath logic are original.

The Unix JRE platform interface is a new Linux port, explicitly not recovered
Sun source. It uses /proc/self/exe to locate bin/../lib/libjavai.so, with no
registry discovery, and dlopen/dlsym for the same JNI functions. This is not a
macOS implementation. Both JRE ports require actual JNI invocation support in
the reconstructed VM; successfully building a launcher does not establish that
JNI or the applet AWT implementation is complete.

## Canonical VM launcher

Primary javai_g.dll java_main at 100012d5 is cdecl `(int, char **)`, initializes
JDK1_1InitArgs, parses options, calls JNI_CreateJavaVM, finds the main method,
executes it, waits for threads and calls sysExit. The reconstruction now follows
that path instead of exporting the old 1.0 bootstrap. Class names are normalized
in writable argument storage; the applet debugger wrapper also supplies writable
storage. Main lookup at 100273e8 searches the superclass chain. Launcher115
captures inherited main with three arguments, including an option-like argument
and a space; the old launcher failed with "main ... is not defined".

GetDefaultJavaVMInitArgs delegates platform defaults to 100024ea/10002527:
128 KiB native stack, 400 KiB Java stack, 1/16 MiB heap bounds, and boot paths
classes, lib/classes.zip, lib/classes.jar, lib/rt.jar, lib/i18n.jar. The launcher
prepends CLASSPATH or dot; an explicit -classpath replaces the complete path.
Deviation: paths use normalized sysJavaHome and native separators rather than
literal Windows DLL-directory/.. spelling. The primordial stack bound is kept
in the live launcher frame after JNI initialization, preserving conservative
GC scanning. Legacy -argfile and source-compilation options are not retained;
they are absent from the recovered 1.1.5 option parser. Coverage profiling
(-prof:jcov) and monitor-count tuning (-nm) remain outstanding parser branches.

## Verification and integration

Standalone MSVC x86 builds of the Windows JRE console and windowed executables
passed. The generic appletviewer source compiled in C mode. Linux x86 JRE
compiled with -std=c89. Both console JRE binaries print the original usage on
-help. These isolated checks use out/launcher115-check and do not rebuild the
main CMake tree.

Generic wrappers use the same source list and links as javac. Definitions:
- javap: TOOL_CLASS="sun.tools.javap.JavaP", TOOL_SIMPLE, TOOL_HEAP=""
- jar: TOOL_CLASS="sun.tools.jar.Main", TOOL_SIMPLE
- appletviewer: TOOL_CLASS="sun.applet.AppletViewer", TOOL_APPLET, TOOL_HEAP=""

javaw uses src/win32/java/javai/javaw.c, a WIN32 executable, linked to javai.
JRE uses shared jre.c and jre_main.c plus the platform jre_md.c, private include
directories share/java/jre and platform/java/jre, VERSION="1.1.5", and advapi32
on Windows or dl on Linux. jrew uses the Windows JRE sources and WIN32 target.
Keep old javap/classsrc provenance; replacing the executable target does not
require removing those historic sources.
