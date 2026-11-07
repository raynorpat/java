"""Check independent Ghostscript rasterization of the Solaris Java print path."""
from pathlib import Path
import sys

for page in (1, 2):
    with (Path(sys.argv[1]) / ("Print115-%d.ppm" % page)).open("rb") as stream:
        assert stream.readline().strip() == b"P6"
        tokens = []
        while len(tokens) < 3:
            line = stream.readline()
            assert line, "truncated header"
            if not line.startswith(b"#"):
                tokens.extend(line.split())
        width, height, maximum = map(int, tokens)
        assert (width, height, maximum) == (612, 792, 255)
        pixels = stream.read()
        assert len(pixels) == width * height * 3
        for x, y, color in [(20, 20, b"\xff\0\0"), (12, 82, b"\xff\0\0"),
                            (25, 82, b"\0\xff\0"), (12, 95, b"\0\0\xff"),
                            (25, 95, b"\xff\xff\xff")]:
            offset = (y * width + x) * 3
            assert pixels[offset:offset + 3] == color, (page, x, y, pixels[offset:offset + 3])
        assert any(pixels[(y * width + x) * 3:(y * width + x) * 3 + 3] == b"\0\0\0"
                   for y in range(48, 61) for x in range(10, 55)), "missing text"
