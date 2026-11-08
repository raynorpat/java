# Private DR2 framework entry point. The standalone JDK rules remain unchanged.
.PHONY: framework framework-install framework-layout
framework:

include build/rhapsody.mk

FRAMEWORK_BUILD = out/dr2-framework
ifeq ($(DEBUG),1)
FRAMEWORK_BUILD = out/dr2-framework-debug
endif
FRAMEWORK = $(FRAMEWORK_BUILD)/JavaVM.framework
FRAMEWORK_LIB = $(FRAMEWORK)/Libraries
FRAMEWORK_VERSION = $(FRAMEWORK)/Versions/A
APPLE_JAVA_CLASSES = /System/Library/Java
FRAMEWORK_INSTALL = $(FRAMEWORK_BUILD)/installed/JavaVM.framework
BRIDGE_SOURCES = $(wildcard src/rhapsody/java/bridge/*.m src/rhapsody/java/bridge/*.c)
BRIDGE_OBJECTS = $(patsubst %.m,$(FRAMEWORK_BUILD)/obj/%.o,$(filter %.m,$(BRIDGE_SOURCES))) $(patsubst %.c,$(FRAMEWORK_BUILD)/obj/%.o,$(filter %.c,$(BRIDGE_SOURCES)))
FRAMEWORK_HEADERS = $(wildcard src/rhapsody/java/framework/*.h)
FRAMEWORK_NATIVE_NAMES = javai net zip math sysresource jpeg agent awt mmedia ObjCJava
FRAMEWORK_TOOLS = java javah jre $(TOOLS) $(DEBUG_TOOLS)
FRAMEWORK_BIN = $(FRAMEWORK)/Home/libexec
FRAMEWORK_CORE_OBJECTS = $(filter-out $(BUILD)/obj/src/share/java/runtime/runtime.o $(BUILD)/obj/src/unix/java/runtime/path_md.o $(BUILD)/obj/src/unix/java/runtime/link_cthreads.o,$(OBJECTS)) $(FRAMEWORK_BUILD)/obj/framework/path_md.o $(FRAMEWORK_BUILD)/obj/framework/link_cthreads.o $(FRAMEWORK_BUILD)/obj/framework/framework-home.o $(FRAMEWORK_BUILD)/obj/framework/runtime.o

$(FRAMEWORK_BUILD)/obj/framework/runtime.o: src/share/java/runtime/runtime.c $(HEADERS) build/rhapsody-framework.mk
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -DJDK115_FRAMEWORK_HOME -c $< -o $@
$(FRAMEWORK_BUILD)/obj/framework/path_md.o: src/unix/java/runtime/path_md.c $(HEADERS) build/rhapsody-framework.mk
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -DJDK115_FRAMEWORK_HOME -c $< -o $@
$(FRAMEWORK_BUILD)/obj/framework/link_cthreads.o: src/unix/java/runtime/link_cthreads.c $(HEADERS) build/rhapsody-framework.mk
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -DJDK115_FRAMEWORK_HOME -c $< -o $@
$(FRAMEWORK_BUILD)/obj/framework/framework-home.o: src/rhapsody/java/framework/framework-home.c build/rhapsody-framework.mk
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

framework: image $(addprefix $(FRAMEWORK_LIB)/lib,$(addsuffix .dylib,$(FRAMEWORK_NATIVE_NAMES))) $(FRAMEWORK_VERSION)/JavaVM $(addprefix $(FRAMEWORK_BIN)/,$(FRAMEWORK_TOOLS)) $(FRAMEWORK)/Classes/classes.zip
	/bin/sh tools/rhapsody-framework-layout.sh "$(FRAMEWORK)" "$(BUILD)" "$(JDK115_DIR)" $(FRAMEWORK_TOOLS)

$(FRAMEWORK_BUILD)/obj/%.o: %.m $(HEADERS) $(wildcard src/rhapsody/java/bridge/*.h) $(FRAMEWORK_HEADERS) build/rhapsody-framework.mk
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -c $< -o $@

$(FRAMEWORK_BUILD)/obj/%.o: %.c $(HEADERS) $(wildcard src/rhapsody/java/bridge/*.h) $(FRAMEWORK_HEADERS) build/rhapsody-framework.mk
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -c $< -o $@

# Commands resolve the core leaf name through their private launcher path.
# Embedding preloads absolute images; extensions use flat namespace imports.
$(FRAMEWORK_LIB)/libjavai.dylib: $(FRAMEWORK_CORE_OBJECTS) build/rhapsody-framework.mk
	mkdir -p $(@D)
	$(CC) -dynamiclib -Wl,-install_name,libjavai.dylib -o $@ $(FRAMEWORK_CORE_OBJECTS) -framework Foundation

$(FRAMEWORK_LIB)/libObjCJava.dylib: $(BRIDGE_OBJECTS) $(FRAMEWORK_LIB)/libjavai.dylib
	$(CC) -dynamiclib -Wl,-undefined,suppress -Wl,-install_name,libObjCJava.dylib -o $@ $(BRIDGE_OBJECTS)  -framework Foundation

$(FRAMEWORK_VERSION)/JavaVM: src/rhapsody/java/framework/NSJavaConfiguration.m src/rhapsody/java/framework/NSJavaVirtualMachine.m $(FRAMEWORK_HEADERS) build/rhapsody-framework.mk
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -dynamiclib -Wl,-install_name,JavaVM.framework/Versions/A/JavaVM src/rhapsody/java/framework/NSJavaConfiguration.m src/rhapsody/java/framework/NSJavaVirtualMachine.m -framework Foundation -o $@

$(FRAMEWORK_LIB)/libnet.dylib: $(NET_OBJECTS) $(FRAMEWORK_LIB)/libjavai.dylib
	$(CC) -dynamiclib -Wl,-undefined,suppress -Wl,-install_name,libnet.dylib -o $@ $(NET_OBJECTS)
$(FRAMEWORK_LIB)/libzip.dylib: $(ZIP_OBJECTS) $(FRAMEWORK_LIB)/libjavai.dylib
	$(CC) -dynamiclib -Wl,-undefined,suppress -Wl,-install_name,libzip.dylib -o $@ $(ZIP_OBJECTS)
$(FRAMEWORK_LIB)/libmath.dylib: $(MATH_OBJECTS) $(FRAMEWORK_LIB)/libjavai.dylib
	$(CC) -dynamiclib -Wl,-undefined,suppress -Wl,-install_name,libmath.dylib -o $@ $(MATH_OBJECTS)
$(FRAMEWORK_LIB)/libsysresource.dylib: $(BUILD)/obj/src/share/sun/net/www/protocol/systemresource/SystemResourceManager.o $(FRAMEWORK_LIB)/libjavai.dylib
	$(CC) -dynamiclib -Wl,-undefined,suppress -Wl,-install_name,libsysresource.dylib -o $@ $<
$(FRAMEWORK_LIB)/libjpeg.dylib: $(JPEG_OBJECTS) $(FRAMEWORK_LIB)/libjavai.dylib
	$(CC) -dynamiclib -Wl,-undefined,suppress -Wl,-install_name,libjpeg.dylib -o $@ $(JPEG_OBJECTS)
$(FRAMEWORK_LIB)/libagent.dylib: $(AGENT_OBJECTS) $(FRAMEWORK_LIB)/libjavai.dylib
	$(CC) -dynamiclib -Wl,-undefined,suppress -Wl,-install_name,libagent.dylib -o $@ $(AGENT_OBJECTS)
$(FRAMEWORK_LIB)/libawt.dylib: $(AWT_TOOLKIT_SOURCES) $(AWT_TOOLKIT_HEADERS) $(BUILD)/awtgen/.stamp $(BUILD)/obj/awt-gif.o $(FRAMEWORK_LIB)/libjavai.dylib
	$(CC) $(CFLAGS) -Isrc/share/java/include -Isrc/unix/java/include -I$(BUILD)/awtgen -dynamiclib -Wl,-undefined,suppress -Wl,-install_name,libawt.dylib $(AWT_TOOLKIT_SOURCES) $(BUILD)/obj/awt-gif.o -framework AppKit -framework Foundation  -o $@
$(FRAMEWORK_LIB)/libmmedia.dylib: src/rhapsody/sun/audio/audio.m src/rhapsody/sun/audio/audio.h $(FRAMEWORK_LIB)/libawt.dylib
	$(CC) $(CFLAGS) -dynamiclib -Wl,-undefined,suppress -Wl,-install_name,libmmedia.dylib -Isrc/share/java/include -Isrc/unix/java/include $< -framework SoundKit -framework AppKit -framework Foundation -o $@

$(FRAMEWORK_BIN)/java $(FRAMEWORK_BIN)/java_g: $(BUILD)/obj/src/share/java/javai/java_stub.o $(FRAMEWORK_LIB)/libjavai.dylib
	mkdir -p $(@D)
	$(CC) $(JVM_LDFLAGS) -o $@ $< -L$(FRAMEWORK_LIB) -ljavai
$(FRAMEWORK_BIN)/javah: $(BUILD)/bin/javah
	mkdir -p $(@D)
	cp $< $@
$(FRAMEWORK_BIN)/jre: $(BUILD)/bin/jre
	mkdir -p $(@D)
	cp $< $@
$(FRAMEWORK_BIN)/%: src/share/java/javai/tool_main.c $(FRAMEWORK_LIB)/libjavai.dylib build/rhapsody-framework.mk
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $(TOOL_FLAGS_$(@F)) $< -L$(FRAMEWORK_LIB) -ljavai $(JVM_LDFLAGS) -o $@

$(FRAMEWORK)/Classes/classes.zip: $(JDK115_DIR)/lib/classes.zip $(BUILD)/bin/javac $(BUILD)/bin/jar src/rhapsody/classes/com/apple/yellow/foundation/NSRuntime.java tools/rhapsody-framework-classes.sh
	/bin/sh tools/rhapsody-framework-classes.sh "$(FRAMEWORK_BUILD)" "$(FRAMEWORK)" "$(BUILD)" "$(JDK115_DIR)" "$(APPLE_JAVA_CLASSES)"

framework-install: framework
	test -n "$(FRAMEWORK_INSTALL)"
	mkdir -p "$(FRAMEWORK_INSTALL)"
	(cd "$(FRAMEWORK)" && tar cf - .) | (cd "$(FRAMEWORK_INSTALL)" && tar xf -)

framework-layout: framework
	/bin/sh tests/framework-dr2/framework-layout.sh "$(FRAMEWORK)" "$(FRAMEWORK_BUILD)/layout-test"


.PHONY: framework-relocate
framework-relocate: framework
	/bin/sh tests/framework-dr2/framework-layout-relocate.sh "$(FRAMEWORK)" "$(FRAMEWORK_BUILD)/relocation-test"



.PHONY: framework-check
framework-check: framework
	/bin/sh tests/framework-dr2/check-framework.sh "$(FRAMEWORK)" "$(FRAMEWORK_BUILD)/acceptance" "$(DESKTOP_PID)"
