# Reflection reconstruction evidence

Primary source: explicitly opened vendor/jdk1.1.5/bin/javai_g.dll, IDA instance
1493265a537f. Sun's shipped reflect.h supplies helper names and numeric widening
rules; shipped Java reflection classes confirm object fields and native signatures.

Observed addresses:

| Operation | javai_g.dll address |
| --- | --- |
| Class.isArray / getComponentType / getModifiers | 1000BA1A / 1000BB26 / 1000BB89 |
| Class fields/methods/constructors wrappers | 1000BBAC / 1000BBCC / 1000BBEC |
| Field enumeration / creation | 10031EB3 / 100324B6 |
| Method enumeration / creation | 10034FC7 / 100357C1 |
| Constructor enumeration / creation | 1003627F / 10036568 |
| Field / Method / Constructor modifiers | 10030BB0 / 10033EC0 / 10034C80 |
| Field address and access checking | 10030DBE |
| Primitive field setter and widening | 1003199C |
| Method.invoke / Constructor.newInstance | 10033F63 / 10034D23 |
| Invoke helper / pseudo-frame setup | 1003443E / 100345C3 |
| InvocationTargetException wrapping | 1003457E |
| Signature to Class conversion | 1003D170 |
| Access checking / array allocation | 1003D49E / 1003D5CC |
| Array.newArray / multiNewArray | 1003ED2D / 1003ED91 |

Native stack adapter targets were individually obtained from all exported
java.lang.reflect Field/Method/Constructor/Array stubs (10061975 through 100621EC).
New reflect115.c contains those adapters plus six Class member lookup adapters
and three Class type-inspection adapters. The adapters are intended for inclusion
by stubs.c, rather than duplicate separate compilation.

The object creators store clazz and slot first. Field adds name/type; Method
adds name/returnType/parameterTypes/exceptionTypes; Constructor adds parameterTypes
and exceptionTypes. Modifiers mask access with 0xfff. Primitive classes have no
members; arrays have no fields or constructors. Constructor enumeration includes
only `<init>`, method enumeration excludes special names. Public instance methods
come from the resolved method table, with static methods gathered through the
superclass chain. Public fields use the slot table plus interface fields.

Invocation validates receiver, access, parameter count, unboxes primitive wrappers
and permits widening only. It builds a pseudo-frame on the VM stack, invokes the
selected method with quick invoke bytecode, boxes the result, and wraps target
exceptions. Array primitive operations use the same widening rules; field writes
also reject final fields. Primitive array storage is packed; Java fields are not.

Transitional mappings and remaining verification needs:

- ClassClass pointers/handles use this tree's current representation. Member
  object Java layouts match the observed binary; GC-visible arguments live in
  the VM stack. Wrapper value fields are found by name instead of hard-coded
  offsets. InvocationTargetException.target likewise uses its resolved offset.
- Public interface closure is recovered from implements constant-pool entries
  rather than the missing Sun 1.1 imethodtable. Interface enumeration ordering
  across complicated diamonds needs a dedicated Sun comparison.
- Single public member lookup currently reuses the enumeration result rather
  than Sun's optimized lookup helpers. Hidden-field selection/order and complex
  inherited interface cases need dedicated oracle coverage.
- Method checked-exception metadata is now retained: methodblock fields
  `unsigned short nexceptions` and `unsigned short *exceptions` follow Sun's
  shipped oobj.h names. The Exceptions attribute is validated for length and
  Class constant-pool indexes and allocated in the existing class arena.
- Core implementation is C89. Cross-platform tests and complete reflection
  conformance remain integration responsibilities; implementing every exported
  adapter does not by itself establish full behavioral equivalence.

Existing Reflect oracle covers declared enumeration, construction, invoke,
private-field rejection, primitive/multidimensional arrays. ReflectExceptions
was compiled by Sun javac and captured by the Sun oracle; it covers checked
exception metadata for methods and constructors plus wrapped target exceptions.
