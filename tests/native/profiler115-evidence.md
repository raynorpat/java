# Heap profiler high-address adaptation

The Phase 5 Linux Debug full suite exposed an intermittent failure in
Profiler115 during the self-hosted compiler replay. An isolated run reproduced
the failure on its fifth attempt, after the first profiling report. GDB with
address randomization enabled showed corrupted method metadata; a later call
targeted eight bytes into invokeJavaMethod.

The cause was profHandles hashing a Class handle as a signed long. A handle at
0xf6e13000 produced table index -529. A hardware watchpoint confirmed the object
counter being incremented from zero to one outside the allocated table. Both
normal-object and object-array branches used the same signed hash.

Sun javai_g.dll profHandles at 0x1002D4DD also uses arithmetic shift and signed
division by 1023. The two unsigned casts are therefore an explicit adaptation
for host heaps placing class handles above 2 GB, not a claim of exact original
arithmetic. Low-address inputs retain the original hash; all 32-bit addresses
now produce indices within the allocated 1023-entry table.

ProfilerHeap115 retains one object and one four-element object array, then
checks that both rows appear in three actual heap reports separated by profiling
resets. Against the old Linux runtime the bounded test stalled in the corrupted
report path. After the fix, this fixture and Profiler115 each passed ten runs in
both Linux configurations. Both fixtures pass on rebuilt Windows Debug/Release;
the new heap fixture also passes on the unchanged Sun Debug/Release VMs. The
self-hosted compiler suite automatically recompiles and replays the new fixture.

Diagnostic logs and the captured core are local artifacts under
out/profiler115-diagnose; the reproducible regression is committed under tests/vm.
