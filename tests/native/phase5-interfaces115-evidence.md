# Phase 5 interface evidence

Recovered with IDA from the unchanged Windows JDK 1.1.5 binaries on 2026-11-07.

| Binary | SHA-256 |
| --- | --- |
| agent.dll | 1C091A2AD2483A3EF704DC0405EBBDCDB3A9609CC0201E1BF96B9A03F53EF686 |
| agent_g.dll | F9528D5E3DE005B02B39D13C7D61B34D2B4C46766F4E8B140244653306AB8B63 |
| JdbcOdbc.dll | FE1B7CB01978C4DB20CAF0E425F6B8B5031FD94ABE0B00028C8B1B36DBE2DD7A |
| JdbcOdbc_g.dll | 6F7381B57D5FD54B23C4363577CF7066E0B6145B1A430EEBE7FDC40D24AE6789 |
| jdb.exe | BB157ED2FEAB5FEE9B6F1964DCB4017DD4A49405C73FDB6E56DAF5802CC6CA2F |

## Native libraries

tools/exports-agent115.txt contains the 47 agent exports in ordinal order.
tools/exports-JdbcOdbc115.txt contains the 69 bridge exports in ordinal order.
IDA entry enumeration confirms identical export names/order between Release and
Debug for each library. The PE entry point is excluded from the manifests.

The debugger exports legacy Java_sun_tools_debug_Agent_*_stub functions and two
BreakpointHandler stubs, not JNI entry points. Agent Debug imports the VM's
_set_debugger_hooks@12, debugger locking, breakpoint, stepping, class lookup,
stack/line, monitor, and thread services. Release agent.dll imports javai
rather than javai_g and does not
import debugger-hook registration. Configuration-specific behavior must be
recovered before asserting working breakpoint support in a Release runtime.
Existing runtime/agent.c is guarded by BREAKPTS; its old source is not proof
of an implemented external agent DLL.
The bridge imports javai_g, ODBC32, MSVCRT, and KERNEL32 in its Debug build.
These manifests establish ABI scope only; the libraries remain to be rebuilt.

## jdb launcher

jdb.exe main at 0x401000 allocates argc+2 pointer slots, sets argv[0] to `java`,
copies the single class pointer from 0x403010 (string at 0x403014 is
`sun.tools.ttydebug.TTY`), appends all original arguments, and calls java_main
with argc+1. It adds no heap flag, JAVA_HOME property, classpath rewriting, or
-J preprocessing. Existing tool_main.c TOOL_SIMPLE with empty TOOL_HEAP matches
that argument construction; Linux uses the same class and argument behavior.

tests/tools/jdb-{help,version,passthrough}.{status,stdout,stderr} were captured
directly from the original jdb.exe using jdb115.cmake CAPTURE=ON. The passthrough
case is `-J-ms16m`, which Java rejects as an invalid debugger option. These three
bounded cases exercise the original Java entry class, exit status, and exact
output streams with only CRLF normalization. They do not open a debug session.
The Java entry class initializes networking even for help, so the test supplies
the reconstructed net library directory through PATH or LD_LIBRARY_PATH.

## First-task verification

tools-jdb115 passes all three cases on Windows Debug/Release and Linux
Debug/Release. The same cases pass from a fresh Windows Release installed image
at out/phase5-launcher-image. Windows tools-console115, javap and vm-Debug115
also pass in both configurations. The initial test failed with a missing jdb
executable before adding the target. Linux build logs are
out/phase5-jdb-linux{,-debug}-build.log; Windows build logs are
out/phase5-jdb-{release,debug}-build.log. Native debug-session and ODBC behavior
are not covered by these launcher checks and remain Phase 5 work.
