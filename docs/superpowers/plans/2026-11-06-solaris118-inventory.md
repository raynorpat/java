# Solaris JDK 1.1.8 reconstruction intake

The owner supplied `C:/Users/raynorpat/Downloads/solaris-jdk-1.1.8` and chose
**X11 AWT first, then full Solaris JDK**. Phase 3 of the existing JDK 1.1.5
plan continues. This document records inspection and the proposed integration
boundary, not completion of Solaris or AWT reconstruction.

## Material identified

`SUNWjvrt/pkginfo`: Intel/i386, version `1.1.8,REV=2002.08.08.11.58`, product
`1.1.8/16`. README.i386 identifies the native-thread release with a default
JIT, for Solaris 2.5.1, 2.6, 7 or 8. Five packages provide runtime, development,
JIT, demos and manuals. Preserve their LICENSE/COPYRIGHT and package metadata.
No original binaries or class archives are added to source control.

The development package has 640 Java source files under `src/java`, VM/native
headers (including Solaris/native-thread platform headers), and debug native
libraries. No native C implementation source was found in this package.
The full distribution has 53 headers. The class archive contains the Motif
peer classes absent from our Windows 1.1.5 archive. The existing reconstructed
Debug javap successfully inspected `sun.awt.motif.MButtonPeer`, including its
native `create(MComponentPeer)` and `setLabel(String)` declarations. This is
classfile inspection, not proof that the peers run on the 1.1.5 VM.

Paths below are relative to the supplied directory:

| Artifact | SHA-256 |
| --- | --- |
| SUNWjvrt/reloc/usr/java1.1/lib/libawt.so | 69f8400b7f960b62e4db39c29628c337f69fb588a862ab7f4419cc6bdef80a18 |
| SUNWjvdev/reloc/usr/java1.1/lib/libawt_g.so | 52b9f449ae6f7075d89ecef95e71c162f0a5609e978ef64205da96a99e7252bb |
| SUNWjvrt/reloc/usr/java1.1/lib/libjava.so | 190f476f951c83cc5f05e930918d1bd07bcc5e7b69f0f1546165234b7bd4edba |
| SUNWjvdev/reloc/usr/java1.1/lib/libjava_g.so | 89752e045e669784870ad3e54add1f02e5bbbde235f37dcc8bb53eb2c80a67f9 |
| SUNWjvrt/reloc/usr/java1.1/lib/classes.zip | 63459d23a40d1f64c7c37d7f4b87c0affaea2b873df2dc54db25274fdfbfab29 |

## Initial IDA findings

`libawt_g.so` is ELF32 i386, 855660 bytes; its release counterpart is 288312
bytes. IDA identified 1223 functions in the debug library and 202 exported
`sun_awt_motif_*` entries, plus nine `sun_awt_image_*` entries. These are
inventory counts, not a count of independently reconstructed functions.

The ELF dynamic dependencies directly name `libXm.so.3` and `libthread.so.1`.
Imports include Motif widget/text/list/font APIs and Xt operations. The ELF
retains `.symtab`, `.strtab`, `.stab.index` and `.stab.indexstr`; these findings
do not establish complete debug types or locals.

Useful starting addresses in this exact debug binary:

- `sun_awt_motif_MToolkit_init`: 0x416ac.
- `sun_awt_motif_MButtonPeer_create`: 0x440a0.
- `sun_awt_motif_MButtonPeer_setLabel`: 0x44570.
- Corresponding legacy stack-stub exports: 0x86a70, 0x87010, 0x87050.

The label setter shows VM monitorEnter/monitorExit around Motif updates,
Java Font lookup, makeMultiFontString/makeCString conversion, XmString
allocation/free, XtVaSetValues, and output flushing. Debug diagnostics retain
`../../../../src/solaris/sun/awt_Button.c` with source line numbers. Its trace
instrumentation must be separated from the release behavior before porting.

## Integration direction and gates

1. Inventory release/debug export pairs and recover native peer/VM layouts.
   Compare Solaris 1.1.8 classes and headers against 1.1.5 before reusing them.
2. Reconstruct X11/Motif AWT with a Linux host adaptation, retaining per-function
   Solaris provenance. Start with toolkit initialization, event dispatch,
   frame/canvas/button, painting and disposal; then the remaining peers,
   images, fonts, clipboard, printing and input methods required by demos.
3. Keep original classes.zip files separate and unchanged. Decide whether
   compatible peer-only classes can be supplied as a separate archive, or
   whether Java 1.1 peer reconstruction is required, from the compatibility
   audit. Do not silently replace the 1.1.5 class library with 1.1.8.
4. Verify native contracts, Xvfb interaction/event/lifetime tests and unchanged
   demos, followed by visual checks. Linux success is not Solaris validation.
5. Expand to the full Solaris 1.1.8 runtime/tools using its own image and
   version baseline. Audit the VM/HPI differences rather than treating the
   existing 1.1.5 build as 1.1.8. A native Solaris test environment and the
   scope of the supplied JIT still need to be established; the existing
   1.1.5 project's JIT exclusion is unchanged by this intake.

The new material supersedes the original spec's assumption that no Sun Unix
AWT binary or Motif peer classes are available. It does not remove the need
for a host port, ABI verification, or the current Phase 3 thread work.
