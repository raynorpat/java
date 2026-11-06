# Public export boundary and private native adapters

The authoritative Release manifest remains tools/exports-sun115.txt (286 Sun
names). Debug uses the separately captured tools/exports-sun115-debug.txt
(287 names), whose sole addition is _set_debugger_hooks@12. CTest selects the
manifest by build configuration; neither list is reduced to make tests pass.

The current .def records supported names present in the built runtime, retaining
the explicit JNI stdcall aliases. Three implemented JDB lock entries are also
included. Configuration generates a .def that adds the original decorated
hook setter in Debug only. No automatic runtime dllexport annotations bypass
this boundary: JAVA_INTERNAL_BUILD suppresses the legacy EXPORTED and
JAVA_PUBLIC_API export attributes for the javai_core object library only.
Native extension DLLs retain their normal export annotations.

The retained legacy Java_*_stub adapters are needed internally even when their
names are absent from Sun's public manifest. tools/builtin-natives.cmake reads
the actual stubs.c include list and generates a name/function table compiled
in that same translation unit. The initial table contains248 distinct adapters.
Preprocessor guards are preserved; missing legacy BREAKPTS source includes
remain disabled, exactly as in stubs.c. This registry is explicitly transitional
reconstruction glue, not a claim of recovered Sun source. dynoLink consults it
before retaining external-library lookup. JNI lookup is unchanged.

javai_core builds the runtime objects once. The DLL uses these objects and its
explicit export definition; interrupt-state links the objects directly, so
private HPI test access no longer widens the production DLL API.

Verification after refactor: Windows Debug javai and interrupt-state built;
interrupt-state passed. Isolated original JRE bootstrap passed Hello and JniAbi
with the newly hidden legacy adapters, both exit0. Strict Debug export checking
reports zero extras and five missing names: java_main and VM getState,
threadsSuspended, unsuspendSomeThreads, unsuspendThreads adapters. The gate
remains failing until those implementations land. No fake exports or renamed
allowlist were introduced. Linux paired build was still running at this entry.

Paired Linux Release core and interrupt-state builds completed; interrupt-state
passed. Linux JRE Hello and JniAbi also passed with expected output and exit0.
