# Rhapsody DR2 Intel: run /bin/gnumake -f build/rhapsody.mk from the source root.
# All outputs remain private; this never installs over JavaVM.framework.
CC = cc
# Bind the real Objective-C runtime and Foundation before loading JNI classes.
JVM_LDFLAGS = -u _objc_msgSend -framework Foundation
BUILD = out/dr2
JDK115_DIR = vendor/jdk1.1.5
CFLAGS = -g -O0 -traditional-cpp -fno-common
ifeq ($(DEBUG),1)
BUILD = out/dr2-debug
CFLAGS += -DDEBUG -DTRACING -DJCOV
DEBUG_TOOLS = java_g
endif
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
OBJECTS = $(patsubst %.c,$(BUILD)/obj/%.o,$(CORE)) $(BUILD)/obj/src/unix/java/runtime/foundation_md.o
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

.PHONY: all image check-types check-core check-libraries
all: $(BUILD)/bin/java $(BUILD)/bin/javah $(BUILD)/bin/jre $(addprefix $(BUILD)/bin/,$(TOOLS) $(DEBUG_TOOLS)) $(addprefix $(BUILD)/lib/lib,net.dylib zip.dylib math.dylib sysresource.dylib jpeg.dylib agent.dylib)

image: all
	mkdir -p $(BUILD)/lib/security
	cp $(JDK115_DIR)/lib/classes.zip $(BUILD)/lib/classes.zip
	cp $(JDK115_DIR)/lib/security/java.security $(BUILD)/lib/security/java.security
	cp $(JDK115_DIR)/lib/*.properties $(BUILD)/lib/
	mkdir -p $(BUILD)/legal/bnlib-1.1 $(BUILD)/legal/zlib-1.0.4
	cp src/share/java/math/bn-1.1/COPYING src/share/java/math/bn-1.1/legal.c src/share/java/math/bn-1.1/PROVENANCE.md $(BUILD)/legal/bnlib-1.1/
	cp third_party/zlib-1.0.4/README third_party/zlib-1.0.4/zlib.h third_party/zlib-1.0.4/ORIGIN.md $(BUILD)/legal/zlib-1.0.4/
	mkdir -p $(BUILD)/legal/jpeg-6
	cp third_party/jpeg-6/README third_party/jpeg-6/PROVENANCE.md $(BUILD)/legal/jpeg-6/

check-core: image
	/bin/sh tests/awt-dr2/check-core.sh $(BUILD)

$(BUILD)/generated/builtin-natives.inc: src/share/java/runtime/stubs.c $(wildcard src/share/java/runtime/_stubs/*.c) tools/builtin-natives.awk
	mkdir -p $(@D)
	awk -f tools/builtin-natives.awk src/share/java/runtime/stubs.c > $@

$(BUILD)/obj/src/share/java/runtime/stubs.o: $(BUILD)/generated/builtin-natives.inc

$(BUILD)/lib/libjavai.dylib: $(OBJECTS)
	mkdir -p $(@D)
	$(CC) -dynamiclib -o $@ $(OBJECTS) -framework Foundation

$(BUILD)/obj/src/unix/java/runtime/foundation_md.o: src/unix/java/runtime/foundation_md.m
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/bin/java $(BUILD)/bin/java_g: $(BUILD)/obj/src/share/java/javai/java_stub.o $(BUILD)/lib/libjavai.dylib build/rhapsody.mk
	mkdir -p $(@D)
	$(CC) $(JVM_LDFLAGS) -o $@ $< -L$(BUILD)/lib -ljavai

$(BUILD)/bin/%: src/share/java/javai/tool_main.c $(BUILD)/lib/libjavai.dylib build/rhapsody.mk
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $(TOOL_FLAGS_$(@F)) $< -L$(BUILD)/lib -ljavai $(JVM_LDFLAGS) -o $@

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

ZIP_SOURCES = src/share/java/zip/zip.c $(addprefix third_party/zlib-1.0.4/,deflate.c trees.c crc32.c)
ZIP_OBJECTS = $(patsubst %.c,$(BUILD)/obj/%.o,$(ZIP_SOURCES))
$(BUILD)/lib/libzip.dylib: $(ZIP_OBJECTS) $(BUILD)/lib/libjavai.dylib
	$(CC) -dynamiclib -o $@ $(ZIP_OBJECTS) -L$(BUILD)/lib -ljavai

MATH_SOURCES = src/share/java/math/BigInteger.c $(addprefix src/share/java/math/bn-1.1/,bn.c bn16.c bninit16.c lbn16.c lbnmem.c prime.c sieve.c legal.c)
MATH_OBJECTS = $(patsubst %.c,$(BUILD)/obj/%.o,$(MATH_SOURCES))
$(BUILD)/obj/src/share/java/math/%.o: src/share/java/math/%.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -Isrc/share/java/math/bn-1.1 -c $< -o $@

$(BUILD)/lib/libmath.dylib: $(MATH_OBJECTS) $(BUILD)/lib/libjavai.dylib
	$(CC) -dynamiclib -o $@ $(MATH_OBJECTS) -L$(BUILD)/lib -ljavai

$(BUILD)/lib/libsysresource.dylib: $(BUILD)/obj/src/share/sun/net/www/protocol/systemresource/SystemResourceManager.o $(BUILD)/lib/libjavai.dylib
	$(CC) -dynamiclib -o $@ $< -L$(BUILD)/lib -ljavai

JPEG_NAMES = jcomapi jutils jerror jmemmgr jmemnobs \
  jcapimin jcapistd jctrans jcparam jdatadst jcinit jcmaster jcmarker jcmainct \
  jcprepct jccoefct jccolor jcsample jchuff jcphuff jcdctmgr jfdctfst jfdctflt jfdctint \
  jdapimin jdapistd jdtrans jdatasrc jdmaster jdinput jdmarker jdhuff jdphuff \
  jdmainct jdcoefct jdpostct jddctmgr jidctfst jidctflt jidctint jidctred \
  jdsample jdcolor jquant1 jquant2 jdmerge
JPEG_SOURCES = $(addprefix third_party/jpeg-6/,$(addsuffix .c,$(JPEG_NAMES))) src/share/sun/awt/image/jpegdecoder.c src/share/sun/awt/image/jpegstubs.c
JPEG_OBJECTS = $(patsubst %.c,$(BUILD)/obj/%.o,$(JPEG_SOURCES))
JPEG_INCLUDES = -Ithird_party/jpeg-6 -I$(BUILD)/jpegconfig
$(BUILD)/jpegconfig/jconfig.h: third_party/jpeg-6/jconfig.doc
	mkdir -p $(@D)
	cp $< $@

$(BUILD)/obj/third_party/jpeg-6/%.o: third_party/jpeg-6/%.c $(BUILD)/jpegconfig/jconfig.h
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(JPEG_INCLUDES) -c $< -o $@

$(BUILD)/obj/src/share/sun/awt/image/%.o: src/share/sun/awt/image/%.c $(HEADERS) $(BUILD)/jpegconfig/jconfig.h
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $(JPEG_INCLUDES) -c $< -o $@

$(BUILD)/lib/libjpeg.dylib: $(JPEG_OBJECTS) $(BUILD)/lib/libjavai.dylib
	$(CC) -dynamiclib -o $@ $(JPEG_OBJECTS) -L$(BUILD)/lib -ljavai

AGENT_SOURCES = $(addprefix src/share/java/debug/,agent115.c agent115_stack.c agent115_metadata.c agent115_events.c)
AGENT_STUBS = $(BUILD)/agentgen/sun_tools_debug_Agent.c $(BUILD)/agentgen/sun_tools_debug_BreakpointHandler.c
AGENT_STUB_OBJECTS = $(BUILD)/agentobj/sun_tools_debug_Agent.o $(BUILD)/agentobj/sun_tools_debug_BreakpointHandler.o
AGENT_OBJECTS = $(patsubst %.c,$(BUILD)/obj/%.o,$(AGENT_SOURCES)) $(AGENT_STUB_OBJECTS)
$(BUILD)/agentgen/.stamp: $(BUILD)/bin/javah $(JDK115_DIR)/lib/classes.zip
	mkdir -p $(@D)
	$(BUILD)/bin/javah -classpath $(JDK115_DIR)/lib/classes.zip -d $(@D) sun.tools.debug.Agent sun.tools.debug.AgentConstants sun.tools.debug.BreakpointHandler sun.tools.debug.BreakpointQueue sun.tools.debug.Field sun.tools.debug.StackFrame sun.tools.debug.LocalVariable sun.tools.debug.LineNumber sun.tools.java.Constants
	$(BUILD)/bin/javah -stubs -classpath $(JDK115_DIR)/lib/classes.zip -d $(@D) sun.tools.debug.Agent sun.tools.debug.BreakpointHandler
	touch $@

$(AGENT_STUBS): $(BUILD)/agentgen/.stamp

$(BUILD)/obj/src/share/java/debug/%.o: src/share/java/debug/%.c $(HEADERS) $(BUILD)/agentgen/.stamp
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -I$(BUILD)/agentgen $(INCLUDES) -c $< -o $@

$(AGENT_STUB_OBJECTS): $(BUILD)/agentobj/%.o: $(BUILD)/agentgen/%.c $(BUILD)/agentgen/.stamp $(HEADERS)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -I$(BUILD)/agentgen $(INCLUDES) -c $< -o $@

$(BUILD)/lib/libagent.dylib: $(AGENT_OBJECTS) $(BUILD)/lib/libjavai.dylib
	$(CC) -dynamiclib -o $@ $(AGENT_OBJECTS) -L$(BUILD)/lib -ljavai

# GNU Make 3.74 chooses the first matching pattern, not the shortest stem.
$(BUILD)/obj/third_party/zlib-1.0.4/%.o: third_party/zlib-1.0.4/%.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -UDEBUG $(INCLUDES) -c $< -o $@

$(BUILD)/obj/%.o: %.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -c $< -o $@

JAVAH_SOURCES = src/share/java/javah/javah.c \
  $(addprefix src/share/java/runtime/,arena.c classloader.c classresolver.c classhash115.c classresolver_stubs.c conversions115.c globals.c runtime_stubs.c utf.c util.c zip.c stubs_md.c) \
  src/unix/java/runtime/path_md.c src/unix/java/runtime/system_md.c
JAVAH_OBJECTS = $(patsubst %.c,$(BUILD)/javahobj/%.o,$(JAVAH_SOURCES))
$(BUILD)/javahobj/%.o: %.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(filter-out -DJNI115_ENABLED,$(CPPFLAGS)) $(INCLUDES) -c $< -o $@

$(BUILD)/bin/javah: $(JAVAH_OBJECTS) $(patsubst %.c,$(BUILD)/obj/%.o,$(ZLIB))
	mkdir -p $(@D)
	$(CC) -o $@ $^

$(BUILD)/tests/jpeg-fixtures: tests/native/jpeg115-fixtures.c $(BUILD)/lib/libjpeg.dylib
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(JPEG_INCLUDES) $< -L$(BUILD)/lib -ljpeg -o $@

$(BUILD)/lib/libjpeg115probe.dylib: tests/native/jpeg115-bridge.c $(BUILD)/lib/libjpeg.dylib
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -dynamiclib $< -L$(BUILD)/lib -ljpeg -o $@

$(BUILD)/tests/agent-status: tests/native/mach115-agent-status.c $(BUILD)/lib/libagent.dylib
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $< -L$(BUILD)/lib -lagent -ljavai -o $@

$(BUILD)/tests/library-path: tests/native/mach115-library-path.c src/unix/java/runtime/link_cthreads.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

check-libraries: image $(BUILD)/tests/jpeg-fixtures $(BUILD)/lib/libjpeg115probe.dylib $(BUILD)/tests/agent-status $(BUILD)/tests/library-path
	/bin/sh tests/awt-dr2/check-libraries.sh $(BUILD)

JRE_SOURCES = src/share/java/jre/jre.c src/share/java/jre/jre_main.c src/unix/java/jre/jre_md.c
JRE_OBJECTS = $(patsubst %.c,$(BUILD)/jreobj/%.o,$(JRE_SOURCES))
$(BUILD)/jreobj/%.o: %.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -Isrc/share/java/jre -Isrc/unix/java/jre -DVERSION='"1.1.5"' -c $< -o $@

$(BUILD)/bin/jre: $(JRE_OBJECTS) build/rhapsody.mk
	mkdir -p $(@D)
	$(CC) $(JVM_LDFLAGS) -o $@ $(JRE_OBJECTS)

.PHONY: install check-install
install: image
	test -n "$(INSTALL_DIR)"
	mkdir -p "$(INSTALL_DIR)/bin" "$(INSTALL_DIR)/libexec" "$(INSTALL_DIR)/lib/security" "$(INSTALL_DIR)/include/unix"
	for name in java javah jre $(TOOLS) $(DEBUG_TOOLS); do cp "$(BUILD)/bin/$$name" "$(INSTALL_DIR)/libexec/$$name"; cp tools/rhapsody-launcher.sh "$(INSTALL_DIR)/bin/$$name"; chmod +x "$(INSTALL_DIR)/bin/$$name"; done
	for name in javai net zip math sysresource jpeg agent; do cp "$(BUILD)/lib/lib$$name.dylib" "$(INSTALL_DIR)/lib/"; done
	cp "$(BUILD)/lib/classes.zip" "$(INSTALL_DIR)/lib/"
	cp $(BUILD)/lib/*.properties "$(INSTALL_DIR)/lib/"
	cp "$(BUILD)/lib/security/java.security" "$(INSTALL_DIR)/lib/security/"
	cp -R src/share/java/include/. "$(INSTALL_DIR)/include/"
	cp -R src/unix/java/include/. "$(INSTALL_DIR)/include/unix/"
	cp -R "$(BUILD)/legal" "$(INSTALL_DIR)/"

check-install: install
	/bin/sh tests/awt-dr2/check-install.sh "$(INSTALL_DIR)"

$(BUILD)/tests/autorelease: tests/awt-dr2/autorelease115.m src/rhapsody/sun/awt/awt_AutoreleasePool.m src/rhapsody/sun/awt/awt_AutoreleasePool.h
	mkdir -p $(@D)
	$(CC) $(CFLAGS) tests/awt-dr2/autorelease115.m src/rhapsody/sun/awt/awt_AutoreleasePool.m -framework Foundation -o $@

.PHONY: check-autorelease
check-autorelease: $(BUILD)/tests/autorelease
	$(BUILD)/tests/autorelease

$(BUILD)/tests/context: tests/awt-dr2/context115.m src/rhapsody/sun/awt/awt_ThreadContext.m src/rhapsody/sun/awt/awt_ThreadContext.h src/rhapsody/sun/awt/awt_AutoreleasePool.m src/rhapsody/sun/awt/awt_AutoreleasePool.h $(BUILD)/lib/libjavai.dylib
	mkdir -p $(@D)
	$(CC) $(CFLAGS) tests/awt-dr2/context115.m src/rhapsody/sun/awt/awt_ThreadContext.m src/rhapsody/sun/awt/awt_AutoreleasePool.m -L$(BUILD)/lib -ljavai -framework AppKit -framework Foundation -o $@

$(BUILD)/tests/desktop-launch: tests/awt-dr2/desktop-launch.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $< -o $@

.PHONY: check-context
check-context: $(BUILD)/tests/context $(BUILD)/tests/desktop-launch
	test -n "$(DESKTOP_PID)"
	/bin/sh tests/awt-dr2/check-context.sh "$(BUILD)" "$(DESKTOP_PID)"

$(BUILD)/tests/libexceptionbridge115.dylib: tests/awt-dr2/exceptionbridge115.m src/rhapsody/sun/awt/awt_Exception.m src/rhapsody/sun/awt/awt_Exception.h
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/exceptionbridge115.m src/rhapsody/sun/awt/awt_Exception.m -framework Foundation -o $@

.PHONY: check-exceptions
check-exceptions: image $(BUILD)/tests/libexceptionbridge115.dylib
	/bin/sh tests/awt-dr2/check-exceptions.sh "$(BUILD)"

$(BUILD)/tests/libnativeentry115.dylib: tests/awt-dr2/nativeentry115.m src/rhapsody/sun/awt/awt_ThreadContext.m src/rhapsody/sun/awt/awt_ThreadContext.h src/rhapsody/sun/awt/awt_AutoreleasePool.m src/rhapsody/sun/awt/awt_AutoreleasePool.h $(BUILD)/lib/libjavai.dylib
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/nativeentry115.m src/rhapsody/sun/awt/awt_ThreadContext.m src/rhapsody/sun/awt/awt_AutoreleasePool.m -framework AppKit -framework Foundation -L$(BUILD)/lib -ljavai -o $@

.PHONY: check-native-entry
check-native-entry: image $(BUILD)/tests/libnativeentry115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-native-entry.sh "$(BUILD)" "$(DESKTOP_PID)"

AWT_DISPATCH_SOURCES = src/rhapsody/sun/awt/awt_Application.m src/rhapsody/sun/awt/awt_Dispatcher.m src/rhapsody/sun/awt/awt_Proxy.m src/rhapsody/sun/awt/awt_DispatchHandler.m src/rhapsody/sun/awt/awt_Exception.m src/rhapsody/sun/awt/awt_Event.m
AWT_DISPATCH_HEADERS = src/rhapsody/sun/awt/awt_View.h src/rhapsody/sun/awt/awt_Application.h src/rhapsody/sun/awt/awt_Dispatcher.h src/rhapsody/sun/awt/awt_DispatchHandler.h src/rhapsody/sun/awt/awt_Exception.h src/rhapsody/sun/awt/awt_Event.h

$(BUILD)/tests/handoff: tests/awt-dr2/handoff115.m $(AWT_DISPATCH_SOURCES) $(AWT_DISPATCH_HEADERS) $(BUILD)/lib/libjavai.dylib
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include tests/awt-dr2/handoff115.m $(AWT_DISPATCH_SOURCES) -framework AppKit -framework Foundation -L$(BUILD)/lib -ljavai -o $@

.PHONY: check-handoff
check-handoff: $(BUILD)/tests/handoff $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-handoff.sh "$(BUILD)" "$(DESKTOP_PID)"

$(BUILD)/tests/libdispatcher115.dylib: tests/awt-dr2/dispatcher115.m $(AWT_DISPATCH_SOURCES) $(AWT_DISPATCH_HEADERS) $(BUILD)/lib/libjavai.dylib
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/dispatcher115.m $(AWT_DISPATCH_SOURCES) -framework AppKit -framework Foundation -L$(BUILD)/lib -ljavai -o $@

.PHONY: check-dispatch
check-dispatch: image $(BUILD)/tests/libdispatcher115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-dispatch.sh "$(BUILD)" "$(DESKTOP_PID)"

RHAPSODY_PEERS = out/dr2-peers/rhapsody-peers.zip
AWT_TOOLKIT_SOURCES = $(AWT_DISPATCH_SOURCES) src/rhapsody/sun/awt/awt_ThreadContext.m src/rhapsody/sun/awt/awt_AutoreleasePool.m src/rhapsody/sun/awt/awt_Toolkit.m src/rhapsody/sun/awt/awt_EventQueue.m src/rhapsody/sun/awt/awt_Conversions.m src/rhapsody/sun/awt/awt_Font.m src/rhapsody/sun/awt/awt_View.m src/rhapsody/sun/awt/awt_Canvas.m
AWT_TOOLKIT_HEADERS = src/rhapsody/sun/awt/awt_Conversions.h $(AWT_DISPATCH_HEADERS) src/rhapsody/sun/awt/awt_ThreadContext.h src/rhapsody/sun/awt/awt_AutoreleasePool.h src/rhapsody/sun/awt/awt_View.h src/rhapsody/sun/awt/awt_Canvas.h

AWT_TOOLKIT_SOURCES += src/rhapsody/sun/awt/awt_Container.m src/rhapsody/sun/awt/awt_Window.m src/rhapsody/sun/awt/awt_Component.m src/rhapsody/sun/awt/awt_Graphics.m src/rhapsody/sun/awt/awt_Frame.m src/rhapsody/sun/awt/awt_WindowPeer.m src/rhapsody/sun/awt/awt_Dialog.m src/rhapsody/sun/awt/awt_FileDialog.m
AWT_TOOLKIT_SOURCES += src/rhapsody/sun/awt/awt_Button.m
AWT_TOOLKIT_SOURCES += src/rhapsody/sun/awt/awt_TextField.m
AWT_TOOLKIT_SOURCES += src/rhapsody/sun/awt/awt_PopUpButton.m
AWT_TOOLKIT_SOURCES += src/rhapsody/sun/awt/awt_ScrollingText.m
AWT_TOOLKIT_SOURCES += src/rhapsody/sun/awt/awt_Scroller.m
AWT_TOOLKIT_SOURCES += src/rhapsody/sun/awt/awt_ScrollView.m
AWT_TOOLKIT_SOURCES += src/rhapsody/sun/awt/awt_ScrollingList.m
AWT_TOOLKIT_SOURCES += src/rhapsody/sun/awt/awt_Menu.m src/rhapsody/sun/awt/awt_Clipboard.m
AWT_TOOLKIT_HEADERS += src/rhapsody/sun/awt/awt_FileDialog.h src/rhapsody/sun/awt/awt_Menu.h src/rhapsody/sun/awt/awt_Clipboard.h
AWT_TOOLKIT_HEADERS += src/rhapsody/sun/awt/awt_Container.h src/rhapsody/sun/awt/awt_Graphics.h src/rhapsody/sun/awt/awt_Window.h src/rhapsody/sun/awt/awt_Component.h
AWT_TOOLKIT_SOURCES += src/rhapsody/sun/awt/awt_Image.m src/rhapsody/sun/awt/awt_Image_stubs.c
AWT_TOOLKIT_HEADERS += src/rhapsody/sun/awt/awt_Image.h

$(BUILD)/awtgen/.stamp: $(BUILD)/bin/javah $(JDK115_DIR)/lib/classes.zip $(RHAPSODY_PEERS)
	mkdir -p $(@D)
	$(BUILD)/bin/javah -classpath $(RHAPSODY_PEERS):$(JDK115_DIR)/lib/classes.zip -d $(@D) sun.awt.image.ImageRepresentation sun.awt.image.OffScreenImageSource java.awt.Color java.awt.image.IndexColorModel java.awt.image.DirectColorModel com.apple.rhapsody.awt.RGraphics
	touch $@

# Partial AWT is confined to tests until the complete peer surface is recovered.
$(BUILD)/tests/toolkit/libawt.dylib: $(AWT_TOOLKIT_SOURCES) $(AWT_TOOLKIT_HEADERS) $(BUILD)/lib/libjavai.dylib $(BUILD)/awtgen/.stamp build/rhapsody.mk
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -I$(BUILD)/awtgen -dynamiclib $(AWT_TOOLKIT_SOURCES) -framework AppKit -framework Foundation -L$(BUILD)/lib -ljavai -o $@

$(BUILD)/tests/toolkit/libtoolkitprobe115.dylib: tests/awt-dr2/dispatcher115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/dispatcher115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-toolkit
check-toolkit: image $(BUILD)/tests/toolkit/libtoolkitprobe115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-toolkit.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/libvalues115.dylib: tests/awt-dr2/values115.m src/rhapsody/sun/awt/awt_Conversions.m src/rhapsody/sun/awt/awt_Conversions.h src/rhapsody/sun/awt/awt_Exception.m src/rhapsody/sun/awt/awt_Exception.h
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/values115.m src/rhapsody/sun/awt/awt_Conversions.m src/rhapsody/sun/awt/awt_Exception.m -framework AppKit -framework Foundation -o $@

.PHONY: check-values
check-values: image $(BUILD)/tests/libvalues115.dylib
	/bin/sh tests/awt-dr2/check-values.sh "$(BUILD)"

$(BUILD)/tests/toolkit/libfontvalues115.dylib: tests/awt-dr2/fontvalues115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/fontvalues115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-font-values
check-font-values: image $(BUILD)/tests/toolkit/libfontvalues115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-font-values.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libcursors115.dylib: tests/awt-dr2/cursors115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/cursors115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-cursors
check-cursors: image $(BUILD)/tests/toolkit/libcursors115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-cursors.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/view: tests/awt-dr2/view115.m $(BUILD)/tests/toolkit/libawt.dylib
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include tests/awt-dr2/view115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-view
check-view: $(BUILD)/tests/view $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-view.sh "$(BUILD)" "$(DESKTOP_PID)"

$(BUILD)/tests/libeventcover115.dylib: tests/awt-dr2/eventcover115.m src/rhapsody/sun/awt/awt_Event.m src/rhapsody/sun/awt/awt_Event.h src/rhapsody/sun/awt/awt_Exception.m src/rhapsody/sun/awt/awt_Exception.h
	@mkdir -p $(BUILD)/tests
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/eventcover115.m src/rhapsody/sun/awt/awt_Event.m src/rhapsody/sun/awt/awt_Exception.m -framework AppKit -framework Foundation -o $@

.PHONY: check-event-cover
check-event-cover: image $(BUILD)/tests/libeventcover115.dylib
	/bin/sh tests/awt-dr2/check-event-cover.sh "$(BUILD)"

$(BUILD)/tests/toolkit/libpaintcover115.dylib: tests/awt-dr2/paintcover115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/paintcover115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-paint-cover
check-paint-cover: image $(BUILD)/tests/toolkit/libpaintcover115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-paint-cover.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libcanvasdraw115.dylib: tests/awt-dr2/canvasdraw115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/canvasdraw115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-canvas-draw
check-canvas-draw: image $(BUILD)/tests/toolkit/libcanvasdraw115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-canvas-draw.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

.PHONY: check-container-draw
check-container-draw: image $(BUILD)/tests/toolkit/libcanvasdraw115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-container-draw.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/windowstate: tests/awt-dr2/windowstate115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include tests/awt-dr2/windowstate115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-window-state
check-window-state: $(BUILD)/tests/windowstate $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-window-state.sh "$(BUILD)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libwindowcallbacks115.dylib: tests/awt-dr2/windowcallbacks115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/windowcallbacks115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-window-callbacks
check-window-callbacks: image $(BUILD)/tests/toolkit/libwindowcallbacks115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-window-callbacks.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libwindowlifetime115.dylib: tests/awt-dr2/windowlifetime115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/windowlifetime115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

$(BUILD)/tests/toolkit/libraster115.dylib: tests/awt-dr2/raster115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/raster115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-window
check-window: $(BUILD)/tests/toolkit/libraster115.dylib image $(BUILD)/tests/toolkit/libwindowlifetime115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-window.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libmousevalues115.dylib: tests/awt-dr2/mousevalues115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/mousevalues115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-mouse-values
check-mouse-values: image $(BUILD)/tests/toolkit/libmousevalues115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-mouse-values.sh "$(BUILD)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libinput115.dylib: tests/awt-dr2/input115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/input115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-input
check-input: image $(BUILD)/tests/toolkit/libinput115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-input.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libkeyvalues115.dylib: tests/awt-dr2/keyvalues115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/keyvalues115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-key-values
check-key-values: image $(BUILD)/tests/toolkit/libkeyvalues115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-key-values.sh "$(BUILD)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libkeyboard115.dylib: tests/awt-dr2/keyboard115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/keyboard115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-keyboard
check-keyboard: image $(BUILD)/tests/toolkit/libkeyboard115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-keyboard.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libsyntheticvalues115.dylib: tests/awt-dr2/syntheticvalues115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/syntheticvalues115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-synthetic-values check-synthetic-input
check-synthetic-values: image $(BUILD)/tests/toolkit/libsyntheticvalues115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-synthetic-values.sh "$(BUILD)" "$(DESKTOP_PID)"

check-synthetic-input: image $(BUILD)/tests/toolkit/libsyntheticvalues115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-synthetic-input.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libqueueread115.dylib: tests/awt-dr2/queueread115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/queueread115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-queue-read
check-queue-read: image $(BUILD)/tests/toolkit/libqueueread115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-queue-read.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libchildviews115.dylib: tests/awt-dr2/childviews115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/childviews115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-child-views
check-child-views: image $(BUILD)/tests/toolkit/libchildviews115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-child-views.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libcomponentstate115.dylib: tests/awt-dr2/componentstate115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/componentstate115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-component-state
check-component-state: image $(BUILD)/tests/toolkit/libcomponentstate115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-component-state.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libbutton115.dylib: tests/awt-dr2/button115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/button115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-button
check-button: image $(BUILD)/tests/toolkit/libbutton115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-button.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libcheckbox115.dylib: tests/awt-dr2/checkbox115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/checkbox115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-checkbox
check-checkbox: image $(BUILD)/tests/toolkit/libcheckbox115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-checkbox.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/liblabel115.dylib: tests/awt-dr2/label115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/label115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-label
check-label: image $(BUILD)/tests/toolkit/liblabel115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-label.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libtextfield115.dylib: tests/awt-dr2/textfield115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/textfield115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-textfield
check-textfield: image $(BUILD)/tests/toolkit/libtextfield115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-textfield.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libchoice115.dylib: tests/awt-dr2/choice115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/choice115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-choice
check-choice: image $(BUILD)/tests/toolkit/libchoice115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-choice.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libtextarea115.dylib: tests/awt-dr2/textarea115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/textarea115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-textarea
check-textarea: image $(BUILD)/tests/toolkit/libtextarea115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-textarea.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libscrollbar115.dylib: tests/awt-dr2/scrollbar115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/scrollbar115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-scrollbar
check-scrollbar: image $(BUILD)/tests/toolkit/libscrollbar115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-scrollbar.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libscrollpane115.dylib: tests/awt-dr2/scrollpane115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/scrollpane115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-scrollpane
check-scrollpane: image $(BUILD)/tests/toolkit/libscrollpane115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-scrollpane.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/liblist115.dylib: tests/awt-dr2/list115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/list115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-list
check-list: image $(BUILD)/tests/toolkit/liblist115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-list.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libmenu115.dylib: tests/awt-dr2/menu115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/menu115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-menu
check-menu: image $(BUILD)/tests/toolkit/libmenu115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-menu.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libclipboard115.dylib: tests/awt-dr2/clipboard115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/clipboard115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-clipboard
check-clipboard: image $(BUILD)/tests/toolkit/libclipboard115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-clipboard.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libbarewindow115.dylib: tests/awt-dr2/barewindow115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/barewindow115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-barewindow
check-barewindow: image $(BUILD)/tests/toolkit/libbarewindow115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-barewindow.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libdialog115.dylib: tests/awt-dr2/dialog115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/dialog115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-dialog
check-dialog: image $(BUILD)/tests/toolkit/libdialog115.dylib $(BUILD)/tests/toolkit/libbarewindow115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-dialog.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libfiledialog115.dylib: tests/awt-dr2/filedialog115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/filedialog115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-filedialog
check-filedialog: image $(BUILD)/tests/toolkit/libfiledialog115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-filedialog.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

.PHONY: check-fonts
check-fonts: image $(BUILD)/tests/toolkit/libawt.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-fonts.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libprimitives115.dylib: tests/awt-dr2/primitives115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/primitives115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-primitives
check-primitives: image $(BUILD)/tests/toolkit/libprimitives115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-primitives.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

$(BUILD)/tests/toolkit/libimage115.dylib: tests/awt-dr2/image115.m $(BUILD)/tests/toolkit/libawt.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -dynamiclib tests/awt-dr2/image115.m -framework AppKit -framework Foundation -L$(BUILD)/tests/toolkit -lawt -o $@

.PHONY: check-image
check-image: image $(BUILD)/tests/toolkit/libimage115.dylib $(BUILD)/tests/toolkit/libraster115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-image.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

.PHONY: check-producer-image
check-producer-image: image $(BUILD)/tests/toolkit/libimage115.dylib $(BUILD)/tests/toolkit/libraster115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-producer-image.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"

.PHONY: check-scaling-image
check-scaling-image: image $(BUILD)/tests/toolkit/libimage115.dylib $(BUILD)/tests/toolkit/libprimitives115.dylib $(BUILD)/tests/desktop-launch
	/bin/sh tests/awt-dr2/check-scaling-image.sh "$(BUILD)" "$(RHAPSODY_PEERS)" "$(DESKTOP_PID)"
