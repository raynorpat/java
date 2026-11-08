# DR2 native entry boundary

The reconstructed Objective-C boundary begins in src/rhapsody/sun/awt. It is
tested separately and is not yet an installed libawt or a working toolkit.

| Donor address | Recovered behavior |
| --- | --- |
| libawt 54003310 / 54003354 | AwtAutoreleasePool creates a real child pool on the first autorelease; subsequent objects enter that child. |
| libawt 5401d3e0 | NativeMethodExit clears the active flag and drains a nonempty deferred pool after eleven exits. Empty exits do not advance its counter. |
| libawt 5400ec20 / 5400ed44 | AwtThreadContext is retained in NSThread's dictionary under `AwtThreadContext Key`. It uses NSDPSContext.currentContext or NSDPSServerContext.contextForSecondaryThread. |
| libawt 5401d308 | NativeMethodEnter caches that context on the supplied JNI environment's thread. It returns no exit token for nested calls or an application-owned autorelease pool. |
| libjava 5c138064 / 5c13807c | get_awt_cookie/set_awt_cookie follow the JNI environment to its Java thread and native PrivateInfo; they do not use the current OS thread. |

The private Foundation selector `autoreleasePoolExists` resolves at 4261eb32;
AppKit's `contextForSecondaryThread` is at 436263e3. Both are present in the DR2
system frameworks but omitted from their public headers. Decompilation and
disassembly agree on the context dictionary ownership and deferred-release fields.

`autorelease115.m` passes unchanged against the original donor and reconstructed
code: no premature release, release on exit eleven, counter reset for the next
batch, and explicit nested Foundation pool release. `context115.m` also passes
both: existing main-thread pool, distinct secondary context/graphics context,
nested entry with no token, and token/context reuse after exit. Raw cthread_fork
alone does not put Foundation into multithreaded mode: the fixture first starts a
Foundation thread through its public API. Toolkit startup must establish that mode
before Java-created cthreads use Foundation services.

From the guest source root, run:

```
/bin/gnumake -f build/rhapsody.mk check-autorelease
/bin/gnumake -f build/rhapsody.mk check-context DESKTOP_PID=<current-Workspace-pid>
```

The context check adopts the selected desktop namespace only in its own process
and has a 30-second timeout. The standalone donor equivalents link the same test
source against the system JavaVM.framework/Libraries libawt, without reconstructed
Objective-C sources or DYLD_LIBRARY_PATH. Never mix donor and reconstructed AWT
classes in one process.

The cookie contract test passes on Windows and Linux OS doubles, including a
supplied environment belonging to a different thread. Native DR2 core fixtures
still pass after adding those VM exports. Actual JNI callback/exception/GC tests,
AppKit handoff, and complete resource teardown remain required before Task 3 is
complete; these focused tests do not establish them.
