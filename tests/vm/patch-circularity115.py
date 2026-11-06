"""Run after Sun javac Circularity115.java in this directory.

Replace one superclass UTF8 constant per class, preserving CP indexes. Java
source cannot express these cycles; the retained class files are patched.
"""
from pathlib import Path
import struct
import sys

directory = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).parent

for filename, superclass in (("Cycle115A.class", "Cycle115B"),
                             ("Cycle115Self.class", "Cycle115Self")):
    path = directory / filename
    original = b"Cycle115Base"
    replacement = superclass.encode("ascii")
    needle = b"\x01" + struct.pack(">H", len(original)) + original
    data = path.read_bytes()
    if data.count(needle) != 1:
        raise RuntimeError("Compile pristine fixture before patching " + filename)
    path.write_bytes(data.replace(needle, b"\x01" +
                                  struct.pack(">H", len(replacement)) + replacement))
