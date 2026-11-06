# Serialization reconstruction evidence

Primary source: vendor/jdk1.1.5/bin/javai_g.dll, explicitly opened IDA instance
1493265a537f. Native entry points and helpers were decompiled and the
ObjectStreamField constructor arguments checked in disassembly.

| Operation | Address |
| --- | --- |
| outputClassFields / invokeObjectWriter | 100515A0 / 10051997 |
| inputClassFields / private hook lookup | 10051A40 / 100520EF |
| invokeObjectReader | 1005220A |
| allocateNewObject / allocateNewArray | 100522AA / 100523F3 |
| loadClass0 | 100526F5 |
| getFields0 / getSerialVersionUID | 10052850 / 10052AA9 |
| hasWriteObject / getClassAccess | 10052B7C / 10052BD7 |
| getMethodSignatures / getMethodAccess | 10052BFA / 10052DE4 |
| getFieldSignatures / getFieldAccess | 10052F29 / 100530E8 |

Metadata signatures join name and descriptor with one space. Access flags are
masked by 0xfff. Stream fields exclude static and transient fields; the integer
called offset in ObjectStreamField is actually the declaring class field index.
The field read/write sequence contains alternating signature character and field
index entries. Missing input fields are still consumed. Reference writes check
the declared type, while primitive values use the existing VM's field slots.

Custom hooks must be private, nonstatic, and match the exact stream descriptor.
Object allocation invokes the public no-argument constructor of the specified
nonserializable superclass on an instance of the serializable subclass. The
subclass constructor is bypassed. Class resolution walks frames to the first
non-bootstrap loader when no explicit context is supplied.

Transitional mappings: current ClassClass pointers replace Sun's class handles;
field addresses use this runtime's resolved offsets. Existing Java invocation
helpers replace Sun's explicit stack checks. Array construction reuses the
reflection component resolver/allocator, including its negative-length check;
malformed-stream negative lengths have not been compared against Sun. The
current interpreter has no compiled frames, so loadClass0 uses ordinary frame
links rather than Sun's compiled-frame adapter. Signature buffers include space
and terminator in their allocation size. Allocation failure and malformed-stream
behavior remain outside these fixture comparisons.

Oracle verification uses Sun javac and capture-expected.cmake with explicit
`-Djava.home=D:/raynorpat/java/vendor/jdk1.1.5`; a modern host JAVA_HOME otherwise
causes Sun's security-properties warning. No warning text is filtered.

- Serial verifies cycles, repeated references, hooks, and explicit/computed UIDs.
- SerialValues verifies all primitive field types, wide bit patterns, null and
  transient fields, constructor bypass, primitive arrays, multidimensional
  reference arrays, and negative-zero double bits.
- SerialHooks verifies Externalizable, read/write hook exception propagation,
  and rejection of a nonserializable object.

SerialValues and SerialHooks each exit zero and match the Sun oracle byte for
byte after CRLF normalization on the Release Windows VM. These tests establish
the listed cases, not complete serialization conformance or cross-platform
verification. The adapter file is included by stubs.c, not compiled separately.
