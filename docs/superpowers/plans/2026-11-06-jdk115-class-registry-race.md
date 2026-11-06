# Class registry collection race

The unchanged VMReserve115 fixture reproduced a Windows Release access violation
within six runs (`out/reserve-crash.dmp`, `out/reserve-cdb.log`). The fault was
inside Locked_ResolveClass, frozen javai DLL RVA 0x15b42. Its field metadata had
already been freed (Windows heap fill FEEEFEEE). An earlier fault was in
Locked_InitializeClass, RVA 0x15516. Temporary collector diagnostics identified
java/util/VectorEnumerator being unloaded before the failure, with no direct
handle reference in any stopped thread stack or captured register. Those
diagnostics have been removed.

Two related lifetime gaps existed:

1. Registry readers cached the binclasses pointer/count while the collector
   removed entries without their lock. A paused reader could resume against a
   compacted registry and stale trailing entry, then resolve freed metadata.
2. The retained loader returned a boolean rather than the managed class handle.
   Between loading and the caller searching the registry again, the new class
   could be collected. With the registry lock fixed, this reproduced as the
   notifier's NoClassDefFoundError for VectorEnumerator rather than an AV.

Primary Sun Debug binary evidence (SHA256
1BE736EAD48839D0FD7312C8162113F414CA67BE4327AC5895E5C0B87D652BFC):

- AddBinClass 1000d670, DelBinClass 1000d797 and class lookup 100121fc acquire
  the separate BinClass monitor at global 10083580.
- GC lock acquisition 1002c2ff acquires heap first (asserted), then finalizer
  queues, thread queue, string hash, BinClass and JNI roots. Release 1002c4bc
  reverses the order. Collection 1002c679 retains these locks across world
  resume and pending-class metadata cleanup 1002c738.
- Import 1000d836 propagates the Class handle returned by file loader 1000d9a8
  or ZIP loader 1000dc66. FindClassFromClass retains that handle before invoking
  initialization/resolution, rather than performing another registry lookup.

The fix adds the original separate BinClass monitor and acquires it after the
thread queue before stopping the world. Registry lookups, mutation, JNI duplicate
checks, bootstrap table updates and diagnostic walks use this monitor. They do
not allocate managed objects while holding it, so there is no new registry-to-
heap inversion. The existing class-loading monitor remains separate. JCOV's
registry traversal is protected by the same monitor in its coordinated change.
The legacy agent.c body is excluded by its top-level BREAKPTS guard in the
current configurations; its registry API remains outside this fix. External
compiler registry pointers retain their existing contract.

File/ZIP import now returns ClassClass* through the resolver so a native stack
or register retains the object during handoff. Internal DoImport retains its
old SourceHint parameter and array compatibility entry; these are not claims of
an exact original signature. The obsolete OpenCode -1 compiler-hook case has no
managed class to return and returns NULL; compiled OpenCode implementations
return a descriptor or -2. javah's declaration was updated to the actual type.
Its build and JNI header gate pass. The DEBUG VerifyClasses diagnostic no longer
asserts that the registry size can never decrease; class unloading permits that.

No sticky-class workaround, notifier preload, or expected-output change was
made. Existing managed-class115 tests still verify unloading enabled/disabled.

Validation: Windows Release rebuilt javai, managed-class115 and javah; focused
managed-class115-0/-1, VMReserve115 and javah-jni pass (4/4). Windows Release
completed 200 repeated runs without AV/error after both fixes. Linux Release
rebuilt java and completed 200 runs with exact VMReserve115 expected output.
The parent independently repeated the unchanged fixture against Sun Release
and Debug 50 times each, all exact matches. Full configuration gates follow
in the parent integration run. Temporary debugger artifacts remain under out/.

Review caught a required companion to the direct handle return: imports must
validate the requested name. Primary file loader 1000d9a8 and ZIP loader
1000dc66 pass that name to createInternalClass. Both reconstructed paths now
forward it to the existing parser check. Before this companion patch, a file
Wrong115.class containing Hello115 ran Hello115; Sun rejected it. Afterward
both directory and compressed ZIP cases return exit1 and exactly
`Can't find class Wrong115\n`, matching Sun. The retained fixture
`tests/vm/resources/misnamed115.zip` contains the existing Hello115.class under
Wrong115.class; `tools/check-class-name115.cmake` checks both forms. Windows
Release passed this regression. Full paired builds after this final companion
patch are delegated to the parent; the 200-run counts above preceded only this
name-validation addition and source comments, not the registry/return fixes.
