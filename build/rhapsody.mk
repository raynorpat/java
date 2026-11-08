# Rhapsody DR2 Intel: run /bin/gnumake -f build/rhapsody.mk from the source root.
# All outputs remain private; this never installs over JavaVM.framework.
CC = cc
BUILD = out/dr2
JDK115_DIR = vendor/jdk1.1.5
CFLAGS = -g -O0 -traditional-cpp -fno-common
CPPFLAGS = -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD
INCLUDES = -Isrc/share/java/include -Isrc/share/java/include/_gen -Isrc/unix/java/include -Isrc/share/java/runtime -Ithird_party/zlib-1.0.4 -I$(BUILD)/generated

RUNTIME_SOURCES = \
  src/share/java/runtime/agent.c \
  src/share/java/runtime/arena.c \
  src/share/java/runtime/class.c \
  src/share/java/runtime/date.c \
  src/share/java/runtime/debug.c \
  src/share/java/runtime/globals.c \
  src/share/java/runtime/io.c \
  src/share/java/runtime/math.c \
  src/share/java/runtime/primitive.c \
  src/share/java/runtime/system115.c \
  src/share/java/runtime/resource115.c \
  src/share/java/runtime/reflect.c \
  src/share/java/runtime/numeric115.c \
  src/share/java/runtime/conversions115.c \
  src/share/java/runtime/serialization.c \
  src/share/java/runtime/core115.c \
  src/share/java/runtime/corepath115.c \
  src/share/java/runtime/string115.c \
  src/share/java/runtime/stringhelpers115.c \
  src/share/java/runtime/exports115.c \
  src/share/java/runtime/hpi115.c \
  src/share/java/runtime/jni.c \
  src/share/java/runtime/jni-call.c \
  src/share/java/runtime/jni-invoke.c \
  src/share/java/runtime/fileextras115.c \
  src/share/java/runtime/breakpoints115.c \
  src/share/java/runtime/jcov115.c \
  src/share/java/runtime/object.c \
  src/share/java/runtime/opnames.c \
  src/share/java/runtime/profiler.c \
  src/share/java/runtime/runtime.c \
  src/share/java/runtime/security.c \
  src/share/java/runtime/string.c \
  src/share/java/runtime/stubs.c \
  src/share/java/runtime/system.c \
  src/share/java/runtime/thread.c \
  src/share/java/runtime/threads.c \
  src/share/java/runtime/utf.c \
  src/share/java/runtime/util.c \
  src/share/java/runtime/zip.c \
  src/share/java/runtime/classinitialize.c \
  src/share/java/runtime/classresolver.c \
  src/share/java/runtime/classhash115.c \
  src/share/java/runtime/common_exceptions.c \
  src/share/java/runtime/threadruntime.c \
  src/share/java/runtime/compiler.c \
  src/share/java/runtime/check_class.c \
  src/share/java/runtime/check_code.c \
  src/share/java/runtime/classloader.c \
  src/share/java/runtime/classruntime.c \
  src/share/java/runtime/gifdecoder.c \
  src/share/java/runtime/exception.c \
  src/share/java/runtime/exceptions.c \
  src/share/java/runtime/filesystem.c \
  src/share/java/runtime/interpreter.c \
  src/share/java/runtime/gc.c \
  src/share/java/runtime/finalize.c \
  src/share/java/runtime/monitor.c \
  src/share/java/runtime/monitor_cache.c

MD_SHARE = \
  src/share/java/runtime/agentthread.c \
  src/share/java/runtime/compiler_md.c \
  src/share/java/runtime/io_md.c \
  src/share/java/runtime/ionet_md.c \
  src/share/java/runtime/math_md.c \
  src/share/java/runtime/process_md.c \
  src/share/java/runtime/properties_md.c \
  src/share/java/runtime/stubs_md.c

MD_PLAT = \
  src/unix/java/runtime/dir_md.c \
  src/unix/java/runtime/gc_md.c \
  src/unix/java/runtime/misc_md.c \
  src/unix/java/runtime/path_md.c \
  src/unix/java/runtime/system_md.c \
  src/unix/java/runtime/net_md.c \
  src/unix/java/runtime/thread_cthreads.c \
  src/unix/java/runtime/mon_cthreads.c \
  src/unix/java/runtime/condition_md.c \
  src/unix/java/runtime/condvar_md.c \
  src/unix/java/runtime/link_cthreads.c \
  src/unix/java/runtime/process_cthreads.c

ZLIB = \
  third_party/zlib-1.0.4/adler32.c \
  third_party/zlib-1.0.4/infblock.c \
  third_party/zlib-1.0.4/infcodes.c \
  third_party/zlib-1.0.4/inffast.c \
  third_party/zlib-1.0.4/inflate.c \
  third_party/zlib-1.0.4/inftrees.c \
  third_party/zlib-1.0.4/infutil.c \
  third_party/zlib-1.0.4/zutil.c

CORE = src/share/java/javai/javai.c src/share/java/javai/javai_dll.c $(RUNTIME_SOURCES) $(MD_SHARE) $(MD_PLAT) $(ZLIB)
# The 1.1 net dylib owns socket natives. The retained 1.0 socket_md.c exports
# collide with it in DR2's flat dyld namespace and are not part of this image.
OBJECTS = $(patsubst %.c,$(BUILD)/obj/%.o,$(CORE))
HEADERS = $(wildcard src/share/java/include/*.h src/share/java/include/_gen/*.h src/share/java/runtime/*.h src/unix/java/include/*.h src/unix/java/include/cthreads/*.h)

TOOLS = javac javap jar appletviewer native2ascii serialver javadoc javakey rmic rmiregistry jdb
TOOL_FLAGS_javac = -DTOOL_CLASS='"sun.tools.javac.Main"'
TOOL_FLAGS_javap = -DTOOL_CLASS='"sun.tools.javap.JavaP"' -DTOOL_SIMPLE -DTOOL_HEAP='""'
TOOL_FLAGS_jar = -DTOOL_CLASS='"sun.tools.jar.Main"' -DTOOL_SIMPLE
TOOL_FLAGS_appletviewer = -DTOOL_CLASS='"sun.applet.AppletViewer"' -DTOOL_APPLET -DTOOL_HEAP='""'
TOOL_FLAGS_native2ascii = -DTOOL_CLASS='"sun.tools.native2ascii.Main"' -DTOOL_SIMPLE
TOOL_FLAGS_serialver = -DTOOL_CLASS='"sun.tools.serialver.SerialVer"' -DTOOL_HOME -DTOOL_HEAP='""'
TOOL_FLAGS_javadoc = -DTOOL_CLASS='"sun.tools.javadoc.Main"' -DTOOL_SIMPLE -DTOOL_HEAP='""'
TOOL_FLAGS_javakey = -DTOOL_CLASS='"sun.security.provider.Main"' -DTOOL_SIMPLE
TOOL_FLAGS_rmic = -DTOOL_CLASS='"sun.rmi.rmic.Main"' -DTOOL_HOME -DTOOL_HEAP='""'
TOOL_FLAGS_rmiregistry = -DTOOL_CLASS='"sun.rmi.registry.RegistryImpl"' -DTOOL_HOME -DTOOL_REGISTRY -DTOOL_HEAP='""'
TOOL_FLAGS_jdb = -DTOOL_CLASS='"sun.tools.ttydebug.TTY"' -DTOOL_SIMPLE -DTOOL_HEAP='""'

.PHONY: all image check-types check-core
all: $(BUILD)/bin/java $(addprefix $(BUILD)/bin/,$(TOOLS)) $(BUILD)/lib/libnet.dylib

image: all
	mkdir -p $(BUILD)/lib/security
	cp $(JDK115_DIR)/lib/classes.zip $(BUILD)/lib/classes.zip
	cp $(JDK115_DIR)/lib/security/java.security $(BUILD)/lib/security/java.security
	cp $(JDK115_DIR)/lib/*.properties $(BUILD)/lib/

check-core: image
	/bin/sh tests/awt-dr2/check-core.sh $(BUILD)

$(BUILD)/obj/%.o: %.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD)/generated/builtin-natives.inc: src/share/java/runtime/stubs.c $(wildcard src/share/java/runtime/_stubs/*.c) tools/builtin-natives.awk
	mkdir -p $(@D)
	awk -f tools/builtin-natives.awk src/share/java/runtime/stubs.c > $@

$(BUILD)/obj/src/share/java/runtime/stubs.o: $(BUILD)/generated/builtin-natives.inc

$(BUILD)/lib/libjavai.dylib: $(OBJECTS)
	mkdir -p $(@D)
	$(CC) -dynamiclib -o $@ $(OBJECTS)

$(BUILD)/bin/java: $(BUILD)/obj/src/share/java/javai/java_stub.o $(BUILD)/lib/libjavai.dylib
	mkdir -p $(@D)
	$(CC) -o $@ $< -L$(BUILD)/lib -ljavai

$(BUILD)/bin/%: src/share/java/javai/tool_main.c $(BUILD)/lib/libjavai.dylib
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $(TOOL_FLAGS_$(@F)) $< -L$(BUILD)/lib -ljavai -o $@

NET_SOURCES = $(wildcard src/share/java/net/*.c)
NET_OBJECTS = $(patsubst %.c,$(BUILD)/obj/%.o,$(NET_SOURCES))
$(BUILD)/obj/src/share/java/net/%.o: src/share/java/net/%.c $(HEADERS) src/share/java/net/net.h
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -Isrc/share/java/net $(INCLUDES) -c $< -o $@

$(BUILD)/lib/libnet.dylib: $(NET_OBJECTS) $(BUILD)/lib/libjavai.dylib
	$(CC) -dynamiclib -o $@ $(NET_OBJECTS) -L$(BUILD)/lib -ljavai

check-types:
	mkdir -p $(BUILD)/tests
	$(CC) $(CFLAGS) $(INCLUDES) tests/native/mach115-types.c -o $(BUILD)/tests/types
	$(BUILD)/tests/types
