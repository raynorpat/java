# JNI native-return debugger exceptions

IDA decompilation of the unchanged Windows JDK 1.1.5 `javai_g.dll` shows:

- `0x100131C0`: restore the JNI local table base/top, update the Java result
  stack, then notify the exception hook with the native entry PC and pending
  exception. Return success according to the exception state after the hook.
- `0x100135F7`: the same sequence, with `monitorExit` before notification.

The JNI-only probe dynamically loads the chosen VM, enables debugging with
`debugPort = -1` (no debugger agent/server), installs the original debugger hook
ABI, and registers three natives. Ten repetitions check one callback each for
plain and synchronized throwing natives, the same pending exception identity,
and no callback for a normal native return. During the synchronized callback,
JNI `MonitorExit` must raise `IllegalMonitorStateException`, proving the method
monitor has already been released. The probe restores the original exception;
it also restores ownership if an incorrect invoker still held the monitor.
Sun's JNI `MonitorExit` returns zero even on ownership failure, so the probe
checks the pending exception instead of that return value.

On 2026-11-07 the unchanged Sun Debug VM passed this probe; the reconstructed VM
before the fix failed its first callback-count assertion. After separating JNI
invocation cleanup from the two notifying wrappers, both pass. Release variants
also pass: exceptions propagate but the debugger hook export is absent.

Verification in `out/phase5-odbc` used both Debug and Release configurations:

```
ctest --test-dir out/phase5-odbc -C Debug -R "^(debugger115-jni(-sun)?|jni-layout|vm-JniHello|vm-JniAbi|jni-invocation|javah-jni|odbc-.*)$" --output-on-failure
ctest --test-dir out/phase5-odbc -C Release -R "^(debugger115-jni(-sun)?|jni-layout|vm-JniHello|vm-JniAbi|jni-invocation|javah-jni|odbc-.*)$" --output-on-failure
```

All nine tests passed in each configuration. Full-platform and debugger-session
integration suites remain separate gates owned by the Phase 5 integration work.
