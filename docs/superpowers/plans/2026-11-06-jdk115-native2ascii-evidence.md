# native2ascii launcher evidence

Primary: vendor/jdk1.1.5/bin/native2ascii.exe, SHA256
EDDAFB077B93E8A1AA9ABD186F6FA96AD434CF7C4EB3FF726E792012C79CA3C2.
IDA main at 00401000 allocates an argument vector, prefixes java, -ms8m and
sun.tools.native2ascii.Main, copies every user argument unchanged, and returns
java_main's status. It neither edits CLASSPATH nor removes -J prefixes.
The class and heap strings are at 00403020 and 00403018 respectively.

The existing tool_main.c TOOL_SIMPLE path implements precisely this argument
shape. This slice adds a CMake executable definition and image installation;
no new runtime exports, native converter or Java replacement is needed.

Actual Sun execution verified the retained harness before implementation:

- UTF8 forward conversion of `café 中` produces `caf\u00e9 \u4e2d`.
- Reverse conversion reproduces UTF8 input; both file paths contain spaces.
- Standard-input conversion gives the same escaped output on stdout.
- `-J-ms16m` is a filename, not a VM flag: exit 1, stdout
  `java.lang.Exception: -J-ms16m can not be read.` and empty stderr.
- Missing -encoding argument exits 1, printing its diagnostic and usage to
  stdout, with empty stderr.

The reconstructed harness initially failed because native2ascii.exe did not
exist. That failure preceded the CMake implementation. The harness uses Sun's
captured expected text and exit statuses and normalizes CRLF only. It has a
30-second limit per process. Source/fixture binaries from Sun remain ignored.

Serialver was also inspected, but is not implemented by this slice: its main
at 00401130 processes JAVA_HOME, CLASSPATH and -J and sets java.home. Treating
it as another TOOL_SIMPLE launcher would be incorrect. It is the next console
tool investigation under the Phase 2 roadmap.

Review initially questioned CRLF fixture handling. A direct CMake READ/HEX probe
showed its text reads normalize CRLF; replaying all five cases against Sun with
both committed fixtures temporarily converted to CRLF also passed. The reviewer
withdrew that finding. No source change was justified by that concern.

Final validation: Windows Release/Debug 84/84 each and Linux x86 Release/Debug
79/79 each, no excluded tests. Logs: out/phase2-tests-{win,linux}-{release,debug}.log.
The five-case oracle harness also passed against freshly installed Release
images at out/phase2-image-win and out/phase2-image-linux, including standalone
classpath/home discovery. No remaining review findings. Phase 2 as a whole
remains open.
