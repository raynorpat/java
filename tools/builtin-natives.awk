# Native DR2 equivalent of builtin-natives.cmake. Run from the source root.
BEGIN { print "/* Generated from stubs.c; do not edit. */" }
/^#include "_stubs\// {
    path = $2
    gsub(/"/, "", path)
    path = "src/share/java/runtime/" path
    source = ""
    while ((getline line < path) > 0) source = source line "\n"
    close(path)
    while (match(source, /Java_[A-Za-z0-9_]+_stub[ \t\r\n]*\(/)) {
        name = substr(source, RSTART, RLENGTH)
        sub(/[ \t\r\n]*\($/, "", name)
        print "    { \"" name "\", " name " },"
        source = substr(source, RSTART + RLENGTH)
    }
    next
}
/^#(ifdef|ifndef|if|else|endif)/ { print }
