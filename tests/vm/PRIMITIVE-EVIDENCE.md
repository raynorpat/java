# Primitive Class reconstruction evidence

Primary IDA database: vendor/jdk1.1.5/bin/javai_g.dll, opened explicitly,
instance 1493265a537f. These addresses and decompilations were read directly:

- 10060BD6: Java_java_lang_Class_getPrimitiveClass_stub dispatches to 1000BCBE.
- 1000BCBE: null name signals NullPointerException; otherwise converts through
  javaString2CString into a 256-byte buffer and calls 1001272A.
- 1001272A: FindPrimitiveClass (name from shipped interpreter.h) scans nine
  records at 1006C3E8, returns null for an unknown name. Lazily creates the
  matching class, calls InitializeClass (100107A8), ResolveClass (100107E9),
  returns null on either error, and then saves the class in its cache slot.
- 1000ED18: createPrimitiveClass (shipped interpreter.h signature) calls the
  class allocator 10027BF3, sets version 45.3, strdup name, super_name
  java/lang/Object, null constantpool, zero constantpool count, null loader,
  access 0x411 (public/final/abstract), primitive flag 0x100, metadata bytes,
  then MakeClassSticky (1000D811, sets Sun sticky flag 0x400).
- 1000BA4A: isPrimitive simply tests flag 0x100, without a null-handle check.
- 1000B656: getSuperclass returns null for primitive OR interface; otherwise
  returns the internal superclass handle. Primitive internal superclass remains
  Object even though the public API returns null.
- 10010E9B: Locked_ResolveClass skips RuntimeInitClass for primitive and array
  classes. The current 1.0 resolver calls it unconditionally.
- 10027BF3: Sun's AllocClass uses a managed 0x70-byte object and sets its
  HandleToSelf. The current tree's AllocClass uses native memory and an arena.

The table at 1006C3E8 stores name pointer, four metadata bytes, and a pointer to
the cache slot. Bytes (signature, typecode, slot size, element size):

| Name | Signature | Typecode | Slot | Element |
| --- | --- | --- | --- | --- |
| void | V | 17 | 0 | 0 |
| boolean | Z | 4 | 4 | 1 |
| byte | B | 8 | 4 | 1 |
| char | C | 5 | 4 | 2 |
| short | S | 9 | 4 | 2 |
| int | I | 10 | 4 | 4 |
| long | J | 11 | 8 | 8 |
| float | F | 6 | 4 | 4 |
| double | D | 7 | 8 | 8 |

Required integration for primitive.c:

1. Add transitional CCF_IsPrimitive=0x800 (deviation: existing 0x100 means
   HasStatics). Append primitive_typecode (unsigned char), primitive_typesig
   (char), primitive_slotsize and primitive_elementsize (unsigned char) to
   current ClassClass. Define cbTypeCode, cbTypeSig, cbSlotSize, cbElementSize
   to access these fields through a ClassClass pointer. Sun's final layout
   instead overlays hashinfo.cbtypeinfo, per shipped oobj.h.
2. Compile primitive.c and register/export native getPrimitiveClass/isPrimitive
   stubs. Their C signatures are HClass *java_lang_Class_getPrimitiveClass(
   HClass *, HString *) and long java_lang_Class_isPrimitive(HClass *).
3. Skip RuntimeInitClass for primitives in current ResolveClass; adjust the
   public getSuperclass primitive/interface cases as above.
4. Keep primitive names out of normal class-name lookup. The transitional
   AddBinClass call roots these native-memory classes in current GC, unlike
   Sun's managed allocation. Without filtering, Class.forName("int") can
   incorrectly discover the registered pseudo-class.

Explicit implementation deviations: arena-owned name rather than strdup;
AddBinClass/Sticky GC rooting and balancing Resolving flag; allocation failure
signals OutOfMemoryError rather than Sun debug assertion. Existing initialization
and resolution are reused, so this is a transitional mapping, not completed
adoption of Sun's managed class layout. No Rhapsody decompilation was needed;
helper names are confirmed by Sun's shipped headers.

PrimitiveClasses.java was compiled with Sun 1.1.5 javac; expected output was
captured using capture-expected.cmake with vendor java.home. It exercises all
nine class literals and wrapper TYPE identities, names, primitive/interface
tests, null superclass/loader, zero interfaces, nonprimitive object/array,
Class.forName("int") rejection, and identity/name after System.gc(). C-runtime
integration and reconstructed-VM execution are delegated to the main agent.
