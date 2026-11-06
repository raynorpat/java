# JDK 1.1.5 process natives

Primary `vendor/jdk1.1.5/bin/javai_g.dll` SHA-256:
`1BE736EAD48839D0FD7312C8162113F414CA67BE4327AC5895E5C0B87D652BFC`.
Functions inspected with IDA; Java constructor inspected with javap against
the shipped classes.zip. The Win32Process header and native stubs were generated
by the reconstructed javah against that same Java class.

| VA | Function / evidence |
| --- | --- |
| `1003c370` | Win32Process.create(String command, String env): three inheritable 512-byte pipes, STARTF_USESTDHANDLES, CreateProcessA with DETACHED_PROCESS (8), closes child ends and thread handle, saves process handle and fd+1 parent pipe descriptors. |
| `1003c5b8` | exitValue: GetExitCodeProcess; STILL_ACTIVE signals IllegalThreadStateException. |
| `1003c60b` | waitFor: infinite WaitForSingleObject then GetExitCodeProcess. |
| `1003c647` | destroy: TerminateProcess(handle, 1). |
| `1003c662` | close: CloseHandle. |
| `1003c67b` | Runtime.execInternal validates command array non-null and constructs Win32Process with both string arrays. |
| `10062d42` through `10062de4` | Native stack adapters for exitValue, waitFor, destroy, create and close. |

Win32Process Java fields are handle at byte 0; stdin/stdout/stderr descriptor
handles at 4/8/12; stream handles at 16/20/24. Its Java constructor joins argv
with spaces, builds a NUL-separated environment string, initializes descriptors,
calls native create, then constructs buffered stdin/stdout and unbuffered stderr.
The native implementation retains this constructor and command-line behavior.

## Explicit port differences

Windows parent-only pipe ends have inheritance disabled so the child cannot
hold its own input writer open. Partial resource failures clean up handles;
the original create failure paths do not clean up all partial allocations.
Modern CRT descriptors use explicit binary read/write flags and retain this
port's existing fd+1 representation. close clears the stored handle. Command
and environment conversion currently uses the port's single-byte conversion,
not the primary's selected file.encoding converter at `1004b10c`.

Linux uses the supplied Windows classes.zip and therefore exposes the same Java
Win32Process interface. This is a declared platform mapping, not recovered Sun
Linux source. Runtime.execInternal preserves the original argv and env arrays,
uses posix_spawnp with three close-on-exec pipes and dup2 file actions, and builds
the matching Java descriptors/streams directly. It does not pass a command
string through a shell. Native state caches waitpid status behind a pthread
mutex; waitFor polls that state, exitValue remains nonblocking, destroy sends
SIGTERM, and an unreachable running process is reaped by a detached native
thread after its Java handle closes. Native create(String,String) is unused on
Linux and rejects attempts to apply Windows command-line parsing there.

## Verification

Both Windows MSVC x86 and Linux GCC -m32 source compilation passed. Windows
Release `vm-Exec` passed: stdout, EOF, exit status and stderr match the Sun oracle.
The additional ExecLifecycle test passes on reconstructed Windows Release,
covering repeated wait/exit status, missing-command IOException, live-process
exitValue, destroy and a custom environment. The combined reference execution
crashed with access violation 0xc0000005 after its first two checks. Each of the
four independent mode executions then passed on the Sun oracle (exit 0); the
checked-in ExecLifecycle-0 through -3 expected files contain those actual
reference outputs. No combined golden file is retained.
