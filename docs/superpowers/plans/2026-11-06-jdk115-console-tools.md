# Remaining Phase 2 console tools

The approved Phase 2 design governs this work. Primary Windows launcher
disassembly distinguishes simple Java-class wrappers from home-aware wrappers.

1. Capture Sun outputs and failing reconstructed launcher tests.
2. Add serialver, rmic and rmiregistry using their recovered JAVA_HOME,
   CLASSPATH and argument ordering. Verify UIDs, runtime flags and errors.
3. Add javadoc and javakey simple wrappers, and the separate java-rmi.cgi
   interpreter-exec launcher. Verify generated documentation, key operations,
   CGI environment forwarding and failures.
4. Close functional RMI tests with the net library, then installed-image and
   Windows/Linux Release/Debug gates. Inspect standalone javaverify separately.

Recovered entry points: serialver/rmic/rmiregistry 0x401130; javadoc,
javakey and java-rmi.cgi 0x401000. Serialver/rmic set java.home and move -J
arguments before the class; rmiregistry also moves -D arguments. None sets
an initial heap size. Javadoc forwards arguments unchanged without a heap
option; javakey forwards unchanged after -ms8m. CGI execs java from PATH,
converts eighteen CGI environment variables to properties and ignores argv.
