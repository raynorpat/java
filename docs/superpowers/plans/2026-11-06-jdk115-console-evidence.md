# Phase 2 console tool evidence

Primary analysis used the original Windows executables in vendor/jdk1.1.5/bin.
The recovered entry points and argument vectors are recorded in the companion
console-tools plan. All execution comparisons use the unchanged 1.1.5 classes.

Implemented serialver, javadoc, javakey, rmic, rmiregistry and java-rmi.cgi.
The home-aware wrappers validate JAVA_HOME and set java.home independently
of the executable-relative CLASSPATH suffix. Registry also moves bare -D
options ahead of its class. Javadoc and javakey forward arguments unchanged.
The CGI executable searches PATH for java and forwards eighteen environment
properties; POSIX exec uses actual argument boundaries instead of CRT quotes.

Console oracle coverage:

- String and Integer serial UIDs, serialver -J heap option, invalid JAVA_HOME,
  rmic invalid flag and registry invalid port. Sun emits the latter two errors
  on stdout. Missing reconstructed launcher established the failing baseline.
- Javadoc generates class documentation, authors/version, member descriptions
  and navigation pages from Documented115.java.
- Javakey help runs through its executable. A test bridge sets a private
  identity.database security property before calling the unchanged tool class;
  create, detail-list and remove persist across separate VM processes. It does
  not touch the user's identity database. DSA 512-bit key generation and its
  persisted public/private keys are checked after the math library is loaded.
- CGI ping and interpreter-not-found response, including ignored launcher
  arguments, exact body/header text and exit status. Original oracle passed;
  absent reconstructed CGI executable supplied the failing baseline.
- Functional RMI is covered with net: generated stub/skeleton, real registry,
  lookup and remote invocation over loopback, followed by local cleanup.

The first integrated Windows Release and Debug suites passed 97/97 each.
Subsequent audio/JPEG integration and the complete Linux matrix are separate
gates, recorded in the parent ledger when finished.

I18n115 compares number/date formatting, parsing, decimal patterns, message
formatting, primary collation and word/sentence boundaries with Sun. Dates use
GMT and locales are explicit. Non-ASCII number separators are printed as code
points so console encoding cannot change the oracle. This covers underlying
APIs; the five unchanged AWT i18n demos still require the later AWT milestones.

Javaverify was inventoried (native verifier executable, not a Java wrapper).
It remains part of the overall reconstruction scope; it is not listed in the
design's Phase 2 milestone and is not claimed implemented here.
