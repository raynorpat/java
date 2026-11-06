# Independent JPEG Group release 6

Unmodified library C and header files from `jpegsrc.v6.tar.gz`, release
6 (2-Aug-95), Thomas G. Lane / Independent JPEG Group. README contains the
distribution conditions and must accompany these files. This software is
based in part on the work of the Independent JPEG Group.

Downloaded from the historical IJG mirror:
https://ftp.icm.edu.pl/packages/jpeg/jpegsrc.v6.tar.gz

Archive SHA-256:
`6311cbbb24b57bc7fb68bd788a9876bb63673302fded219070b39147fb124de7`

The original C library sources and headers are unchanged. CMake copies the
original ANSI `jconfig.doc` to a build-only `jconfig.h`. jmemnobs supplies
memory allocation without backing files, as in Sun's DLL (which imports
malloc/free but no temporary-file APIs). Compressor sources are used only
by the deterministic fixture generator; production jpeg.dll links the
decompression routines needed by Sun's glue. No upstream binary test data
is committed. Sun glue lives separately in src/share/sun/awt/image.
