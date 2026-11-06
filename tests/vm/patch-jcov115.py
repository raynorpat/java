"""Normalize source metadata in a Sun javac -prof=jcov JCov115.class.

This edits the class fixture, not oracle output: the original VM therefore
writes reproducible paths and timestamps on every platform.
"""
from pathlib import Path
import struct
import sys

directory = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).parent
path = directory / "JCov115.class"
data = bytearray(path.read_bytes())

def u2(offset):
    return struct.unpack_from(">H", data, offset)[0]

def u4(offset):
    return struct.unpack_from(">I", data, offset)[0]

strings = {}
locations = {}
offset = 10
index = 1
while index < u2(8):
    tag = data[offset]
    offset += 1
    if tag == 1:
        length = u2(offset)
        strings[index] = bytes(data[offset + 2:offset + 2 + length])
        locations[index] = (offset, offset + 2 + length)
        offset += 2 + length
    elif tag in (3, 4, 9, 10, 11, 12):
        offset += 4
    elif tag in (5, 6):
        offset += 8
        index += 1
    elif tag in (7, 8):
        offset += 2
    else:
        raise RuntimeError("Unexpected classfile constant " + str(tag))
    index += 1

offset += 6
offset += 2 + 2 * u2(offset)
for members in range(2):
    count = u2(offset)
    offset += 2
    for member in range(count):
        attributes = u2(offset + 6)
        offset += 8
        for attribute in range(attributes):
            offset += 6 + u4(offset + 2)

attributes = u2(offset)
offset += 2
source_index = None
timestamp_found = False
for attribute in range(attributes):
    name = strings[u2(offset)]
    length = u4(offset + 2)
    if name == b"AbsoluteSourcePath" and length == 2:
        source_index = u2(offset + 6)
    elif name == b"TimeStamp" and length == 8:
        data[offset + 6:offset + 14] = struct.pack(">Q", 1)
        timestamp_found = True
    offset += 6 + length
if source_index is None or not timestamp_found:
    raise RuntimeError("Compile with Sun javac -prof=jcov before patching")
start, end = locations[source_index]
source = b"JCov115.java"
data[start:end] = struct.pack(">H", len(source)) + source
path.write_bytes(data)
