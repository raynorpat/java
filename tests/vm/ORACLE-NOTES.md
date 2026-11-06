# JDK 1.1.5 fixture integration

All adjacent Java classes were compiled with `vendor/jdk1.1.5/bin/javac.exe`
from `tests/vm`. Expected output is from that vendor's `java.exe`, never from
the reconstructed VM. Helper and generated inner `.class` files are inputs too.
No CMake registrations or production changes are included here.

Run Props115, Unicode, Reflect, Serial, ClinitFail, InnerAndOld, Interrupt and
Exec with the normal tests/vm + vendor classes.zip classpath. Pass
`-Djava.home=<absolute vendor/jdk1.1.5>`: the original launcher otherwise uses
the host's JAVA_HOME, which here names JDK 21. Serial then reports missing
security properties. Serial.expected was captured with the explicit vendor
home and contains no warning. Other standard captures used
`tools/capture-expected.cmake`; compare CRLF-normalized output.

Unicode uses explicit UTF8 and UnicodeBig conversions. Its stdout check writes
explicit UTF8 bytes through System.out; PrintStream.print(String) otherwise
uses platform-native conversion even with `-Dfile.encoding=UTF8` on this VM.
It round-trips UTF8 data in an accented filename and deletes its temporary file.
Chinese filename behavior and default-encoding byte output are not covered.

Resources needs `tests/vm/resources115.zip` added to the classpath. Its expected
file was captured with Python subprocess byte capture (stderr merged, exit zero,
CRLF normalized). The zip contains vm115.properties and ResourcePayload.bin,
the latter copied unchanged from the compiled ResourcePayload.class. Sun's
resource lookup returned null for ResourcePayload.class; the .bin name works.
The custom loader explicitly delegates resource lookup, defines its own payload
class, resolves it, and checks the defining loader. Rebuild the archive by
copying ResourcePayload.class to resources/ResourcePayload.bin, then running
Sun jar from resources: `jar cf ../resources115.zip vm115.properties ResourcePayload.bin`.

ClassPathForms requires separate launcher invocations, not main-method ARGS.
All these Windows forms were executed against Sun and matched
ClassPathForms.expected (cwd = tests/vm; LIB = absolute vendor lib/classes.zip;
DIR = absolute tests/vm):

- `-classpath "DIR;LIB" ClassPathForms`
- `-classpath "DIR/classpath115.zip;LIB" ClassPathForms`
- `-classpath "DIR/missing;DIR/classpath115.zip;LIB;" ClassPathForms`
- `-classpath ";LIB" ClassPathForms` (leading empty element)
- `-classpath "DIR;;LIB" ClassPathForms` (interior empty element)
- No -classpath, with CLASSPATH=`DIR/classpath115.zip;LIB`.
- No -classpath or CLASSPATH, with JAVA_HOME set to vendor/jdk1.1.5 (default).

Replace separators with colons on Unix. classpath115.zip contains only the
compiled ClassPathForms.class plus Sun jar's manifest. The zip-only form proves
the launcher loads the main class from an archive. Rebuild with Sun jar from
tests/vm: `jar cf classpath115.zip ClassPathForms.class`.

Oracle deviations from the plan:

- ClinitFail: first direct getstatic throws ExceptionInInitializerError; second
  reads zero. First new of a separate failing class throws the same error;
  second new succeeds. NoClassDefFoundError is NOT this oracle's observed result.
- Reflect checks Integer.TYPE, the public entry to primitive-class lookup;
  private Class.getPrimitiveClass cannot be invoked through ordinary Java access.
- InnerAndOld covers inner, local, and anonymous classes. Its optional argument
  loads and reports an old class, but the old-class fixture is NOT supplied:
  `git ls-tree 07baa55 classsrc/classes.zip` finds no file at the plan's source.
  Its default expected output covers only the new inner classes.
- BadStack malformed verification fixture is not implemented.

Interrupt uses readiness monitors and a short scheduling allowance before
interrupting sleep/wait/join. Pending interrupts also satisfy the same contract;
Java 1.1 has no thread-state API to prove the exact blocking instant. Bounded
joins fail if interruption hangs, and the Sun capture matched five repeated runs.
Exec checks child stdout, EOF, exit status, and empty stderr, choosing cmd on
Windows and /bin/echo on Unix. Only the Windows oracle was executed here.
