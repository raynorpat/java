# Solaris Motif compatibility preflight

Phase 6 started from merged Phase 5 master 9beb8ab. This audit is an inventory
and structural reference check; no Linux Motif native implementation is claimed.

## Inputs and native entries

The supplied Solaris JDK 1.1.8/16 files remain unchanged. SHA-256 values:

| Input | SHA-256 |
| --- | --- |
| Windows 1.1.5 classes.zip | 750757205976f91cf54017dd4193af0f2abab7e9c829c6a9761389fe090fbb2d |
| Solaris 1.1.8 classes.zip | 63459d23a40d1f64c7c37d7f4b87c0affaea2b873df2dc54db25274fdfbfab29 |
| Solaris libawt.so | 69f8400b7f960b62e4db39c29628c337f69fb588a862ab7f4419cc6bdef80a18 |
| Solaris libawt_g.so | 52b9f449ae6f7075d89ecef95e71c162f0a5609e978ef64205da96a99e7252bb |

IDA entry-name enumeration, filtered to sun_awt_* and Java_sun_awt_*, produced
identical 421-name sets for release and debug. These comprise 209 legacy Java
stubs and 212 C entry names. tools/solaris118-awt-native-entries.txt records that
subset, not the library's entire ELF symbol table or a future Linux export policy.
Reproduce the enumeration with sorted(n for n in db.entries.get_names() if
n.startswith(('sun_awt_', 'Java_sun_awt_'))) in each original IDA database.

The Solaris archive contains 67 sun.awt.motif classes with 199 native method
declarations. Every declaration has its expected legacy stub name in both native
libraries. The reconstructed javap independently confirms MButtonPeer.create's
(Lsun/awt/motif/MComponentPeer;)V descriptor and setLabel's
(Ljava/lang/String;)V descriptor. Counts do not imply those routines are rebuilt.

## Peer-only overlay findings

Adding only the 67 Motif classes to the unchanged 1.1.5 class index leaves these
seven directly referenced support classes absent:

- sun.awt.X11DrawingSurface
- sun.awt.im.InputMethod and sun.awt.im.InputMethodAdapter
- sun.io.CharToByteEUC_CN, CharToByteEUC_KR, CharToByteEUC_TW and CharToByteISO8859_1

All examined member descriptors whose owners are present resolve through the
overlay's class/interface hierarchy. References whose owners are absent are
reported as missing classes, not counted again as missing members. Supporting
classes' transitive dependencies have not yet been selected or validated.

Seven referenced classes have different declared instance-field sequences:

| Class | Solaris additions relative to 1.1.5 |
| --- | --- |
| java.awt.Component | nameExplicitlySet, isPacked |
| java.awt.EventQueue | eventQueueListener |
| java.awt.MenuComponent | nameExplicitlySet |
| java.awt.Window | active, inputContext |
| sun.awt.EmbeddedFrame | isCursorAllowed |
| sun.io.CharToByteDoubleByte | highHalfZoneCode |
| sun.io.CharToByteJIS0208 | index2 |

This makes copying Solaris native field offsets unsafe. Generate the eventual
native headers from the selected peer archive plus the actual 1.1.5 baseline;
recover accesses against those layouts. The audit does not calculate C padding,
VM field offsets, access-control legality, bytecode verification, reflection,
dynamic class loading or native callback contracts.

The next step is to resolve the support-class closure and test actual peer loading
against 1.1.5. Use a separate peer/support archive or minimal reconstructed Java
adaptations; do not replace java.* classes with Solaris versions. Input-method
integration and drawing-surface access require particular care before toolkit
initialization can be claimed compatible.

## Reproduction and checks

Run from the repository root, substituting the supplied archive path as needed:

```powershell
python tests/awt118/audit-test.py vendor/jdk1.1.5/lib/classes.zip C:/Users/raynorpat/Downloads/solaris-jdk-1.1.8/SUNWjvrt/reloc/usr/java1.1/lib/classes.zip
python tools/awt118-audit.py vendor/jdk1.1.5/lib/classes.zip C:/Users/raynorpat/Downloads/solaris-jdk-1.1.8/SUNWjvrt/reloc/usr/java1.1/lib/classes.zip --output out/solaris118-awt/compatibility.json
```

Three tests check original native descriptors, inherited/absent members and
non-inherited constructors, and the pinned archive's complete audit inventory.
The initial run failed because the audit tool was absent; all pass with the
implementation. Both archives are read-only inputs, and their hashes are included
in every generated report. The JSON retains ordered declared-field sequences and
every Motif native descriptor for subsequent recovery tasks.
