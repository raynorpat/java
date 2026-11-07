# Native toolkit bootstrap recovery (in progress)

Sources are the unchanged Solaris binaries pinned in compatibility.md. This is
Task 3 evidence, not a completed native implementation or live-window claim.

| Routine | Debug address | Release address |
| --- | --- | --- |
| MToolkit.init | 0x416ac | 0x22894 |
| MToolkit event loop | 0x42b94 | 0x23174 |
| MButtonPeer.create | 0x440a0 | 0x239f4 |
| MButtonPeer.setLabel | 0x44570 | 0x23c58 |
| Button_callback | 0x44040 | not yet mapped |

IDA decompilation of both event loops agrees: acquire the toolkit object's VM
monitor, flush pending output, drain queued Xt events, notify monitor waiters,
release the monitor before polling the X connection for 250 ms, then reacquire
it before inspecting Xt state. Preserve this boundary in the Linux port. The
Solaris poll event mask is 64; use the host's symbolic poll constant, not that
numeric value. Dispatch includes a put-back queue, widget-liveness checks, input
filtering and a special scrollbar mouse-event path. Recover these helpers before
claiming callback/disposal correctness.

Debug initialization stores the toolkit handle as awt_lock, enters that monitor,
sets locale modifiers, initializes Xt and its application context, installs
fallback resources, and opens the display. Failure signals InternalError and
releases the monitor. Successful initialization chooses a 24-bit TrueColor visual
when available unless FORCEDEFVIS is set, otherwise uses the default visual. It
sets up colormap/colors, the mask GC, modifier mappings and Motif color handling.
Color allocation and fallback-resource helpers still need recovery.

Debug button creation allocates a 0x30-byte native peer record, creates an
xmPushButton widget under the parent peer, defers mapping and registers the
activation callback with the Java peer as client data. Its callback invokes
action()V through execute_java_dynamic_method and describes/clears exceptions.
Label updates create a Motif string, set the widget resource, free that string,
and schedule output flushing while holding the toolkit monitor. Solaris object
offsets must be translated through Task 2's generated baseline headers.

Next: recover frame/canvas lifetime, event-filter helpers, root/GC ownership and
font bootstrap; build the 32-bit X11/Motif target and add bounded Xvfb tests.

Host preparation: enabled WSL i386 packages and installed libmotif-dev:i386,
libx11-dev:i386, libxt-dev:i386 and xvfb (xauth was already present). A standalone
gcc -m32 probe linked with -lXm -lXt -lX11 successfully created, realized and
destroyed an XmPushButton under `timeout 15 xvfb-run -a`. This checks the host
toolchain/display dependencies only; it does not exercise the Java toolkit.

## First live-window checkpoint

The optional Linux target now links reconstructed toolkit, component, frame,
button, font and graphics units. `motif118-implemented.txt` selects 47 recovered
bindings from the full generated inventory; `--no-undefined` rejects missing
implementations. This is a subset, not a claim that all 199 natives are ready.

Additional recovery anchors in debug libawt_g.so are component initialization
0x3a540, disposal 0x3ba28, canvas creation 0x63050, frame creation 0x5f220 and
font selection/metrics 0x3f804..0x40b1c. Release graphics anchors are
0x2e41c/0x2e498 (construction), 0x2e770 (disposal), 0x2eea0 (clipping) and
0x2f4c4 (rectangle fill). Native records use generated Java field layouts, not
Solaris offsets. Graphics retain their native peer record after widget disposal
and reject further drawing when its widget has retired.

Linux Debug verification: all nine `motif118` CTests pass. The toolkit checks
display failure, screen/color queries and monitor release while idle. Font tests
exercise 20 family/style combinations plus byte-array bounds. Window115 checks
actual red X11 pixels, XTest button input, Java event-thread activation, twelve
show/dispose cycles, disposal from inside action callbacks, and graphics retained
and cloned across disposal. The runner rejects stderr warnings as well as errors.

The test probe requires libxtst-dev:i386; libawt itself does not link XTest.
Initialize the toolkit before loading the probe: loading its Xt dependency first
interposes Xt's vendor shell class ahead of Motif and produces an invalid
WM_NAME deletion during XmDisplay initialization. Both standalone visual probes
succeed, and the normal toolkit-first sequence removes this failure. XSendEvent
alone did not activate the button; XTest supplies server-generated pointer state
and timestamps. Neither issue was hidden by an X error handler.

Still open: original input filtering/put-back queue, window-manager behavior,
remaining widgets, complete graphics/font/image contracts, installed demos and
the full regression matrix. Tasks 3 through 6 remain open.

## Mouse dispatch checkpoint

Release 0x22fd8 filters input before Xt dispatch; 0x23174 drains the put-back
queue before normal events. Debug `awt_post_java_mouse_event` at 0x82f2c creates
MouseEvent and calls the peer's postEvent method. `nativeHandleEvent` returns
unconsumed events to the Motif loop; it does not invoke Xt on the Java event
thread. Modifier/click tracking is anchored by debug 0x80fc8 and release 0x3d0a4.

Input115 first failed with mouse counts 0/0/0 despite one button action. It now
checks press/release/click delivery, coordinates/time, consumed events suppressing
the action, and disposal from mousePressed preventing subsequent native replay.
All ten focused Linux Debug tests pass (the expanded input test was rerun after
correcting its use of a post-1.1 Java API).

Linux lifetime adaptation: Java event `data` contains an opaque token instead of
a raw XEvent pointer. Native records are retired when their widget is disposed;
a later Java callback safely finds no pending token. This avoids retaining a
native allocation when disposal removes Component.peer before Java dispatch.
Queued replay occurs only on the Motif loop and is canceled with its owner.
Keyboard, focus, modal/menu filtering and child-widget routing are still open.

## Initial keyboard and focus checkpoint

Release component screen-coordinate lookup (0x20264) and focus traversal
(0x207a4/0x20838) now use Xt translation/traversal through generated peer layouts.
Key mapping preserves the ordered 125-entry table at release 0x46830. Release
0x3caf0 supplies press/typed/release conversion; debug 0x80ed0/0x82dd8 supplies
focus-event filtering/construction. Focus handlers are registered with Xt so
Motif's synthetic focus changes reach Java as well as real X events.

Input115 failed first on the missing screen-coordinate native and then with no
keyboard or focus events. After conversion, it exposed missing KeyReleaseMask
selection (press and typed arrived, release did not). Explicit peer event masks
correct that host integration issue. Checks cover relative screen coordinates,
focus notification, lowercase, Shift, Control, arrow keys, consumed Space, and
the prior mouse/disposal cases. Remaining input work includes dynamic modifier
mapping, keypad/locale/XIM handling, edited-key replay, menus and modality.

## Label and checkbox checkpoint

Label resources follow release 0x2bff0/0x2c0ac/0x2c208. Toggle creation, state,
group indication and callbacks follow debug 0x482e0..0x48f78 and release
0x24d38..0x25044. Toggle callbacks invoke action(boolean); programmatic resource
updates do not synthesize ItemEvents. CheckboxGroup exclusivity remains in the
unchanged Java classes.

Widgets115 failed on missing MLabelPeer.create before implementation. It now
checks native label contents/alignment, null labels, actual checkbox state,
radio selection and group detachment over eight lifetime cycles. The initial
detachment expectation was corrected using the original 1.1.5 Checkbox bytecode:
setCheckboxGroup first clears the old group's selection, so the detached box
starts unchecked. All eleven focused Linux Debug tests pass. The native binding
subset is now 57 entries. Multifont label conversion remains a Task 5 gate.

## Scrolled-list checkpoint

Release 0x2b780..0x2bed0 and debug Slist_callback at 0x5b860 establish a compound
peer: the public widget is the scrolled window and selection operates on its
inner XmList. The native record now tracks that content widget; input, focus,
font and color propagation reach it, and mouse coordinates translate back to
the Java component. Disposal removes both event handlers before retiring widgets.

List115 first failed at missing MListPeer.create. Its six-cycle check now passes
multiple/single selection, insertion/deletion, native item counts, native scroll
position, mouse selection, Return activation, clearing and disposal. The partial
native inventory has 66 entries. Locale-aware item strings and menu/modal input
remain separate open gates.

## Choice checkpoint

Release 0x2625c..0x26b80 and debug Choice_callback at 0x4c1c0 establish an option
menu with a separately owned pulldown and indexed push-button items. The native
record retires its item array and pulldown before the public widget; a destroy
callback also handles a pulldown already destroyed with its parent. Selection,
removal and callback indices follow the unchanged Java peer boundary.

Choice115 failed at missing create, then passed popup keyboard selection,
removal, clear/regrowth to 50 items, and six disposal cycles. An added native
color check failed for newly added items after recoloring. Explicitly copying
the option menu's foreground/background, as release addItem does, fixes it.
All thirteen focused Linux Debug tests pass; 74 Motif bindings are implemented.
New-item font inheritance and multifont strings still belong to the open font
gate, along with the other previously recorded limitations.

Scrollbar recovery: release 0x26d80..0x27318 implements four bindings and the
line/page/absolute callbacks. Both orientations pass range, increment, repeat
release and disposal-while-pressed checks. The latter exposed an OpenMotif/Xt
keyboard grab targeting a freed scrollbar; GDB located the stale grab during
RemoveAllPMgr's synthetic key dispatch. Retiring the tracked scrollbar now
releases its Xt keyboard/pointer grab. Component retirement also restores the
original unmanage/drain/destroy ordering (debug 0x3ba28, helper 0x6d828).
Linux Debug: all 14 focused Motif tests pass; 78 native bindings implemented.
The remaining widget/rendering/demo gates still keep Phase 6 incomplete.

## Text peer checkpoint

Release 0x298dc..0x2a5a0 and 0x23da4..0x24bf0 recover text-field and text-area
callbacks, selection/caret, edits, editable state, masking, geometry and disposal.
The field owns its password buffer separately from displayed text, preserving
Solaris's append/trailing-deletion restriction. Buffer growth beyond 1024 bytes
and replacement/change of echo characters are exercised without private Motif
offsets. Compound text areas retain the scrolled outer and inner text widgets.
The original Java initialization also requires Frame.addTextComponent (0x2da90);
its status/preedit area query uses public XIC attributes rather than the Solaris
Motif layout used by helper 0x3123c. Full XIM and decorated-frame geometry remain
open integration gates.

Both fixtures first failed at missing create. TextField115 passes four lifetime
cycles, initial selection, caret placement, keyboard edits, text/action callbacks,
read-only state, echo storage/display and growth. TextArea115 passes all four
scrollbar policies, multiline edits, selection, keyboard/caret, font geometry and
show/hide/disposal. Fixtures wait for asynchronous Java/native event replay.
All 16 focused Linux Debug Motif tests pass; 111 native bindings are implemented.
Multifont/locale conversion, clipboard paste, full input/window semantics and the
remaining rendering/demo/platform gates are still unfinished.

## Scroll pane checkpoint

Release 0x27464..0x280f4 supplies seven ScrollPane bindings, adjustment reason
mapping, native insets, scrollbar policy and child positioning. The no-scrollbar
manager requires XtMoveWidget as in the original; a normal geometry request was
rejected, leaving the child at the origin. ScrollPane115 failed at missing create,
then verified all three policies, native positions/limits, arrow callbacks, child
replacement and disposal while holding an embedded scrollbar. That final case
reproduced the stale Xt keyboard grab with VertScrollBar in GDB. Grab retirement
now recognizes ancestor disposal and tracks the scrollbar's destruction callback.
All 17 focused Linux Debug tests pass, with 118 Motif bindings implemented.

## Graphics primitive and text checkpoint

Release 0x2ec44..0x2fbe0 recovers lines, rectangles, arcs/ovals, rounded rectangles,
polygons, copying and XOR/paint mode. Helpers 0x2e334 and 0x2f7a8 establish angle
normalization, polygon closure and translated X11 points. Text paths at
0x2e7cc..0x2eaa8 and 0x3240c preserve byte order, the single-font 1024-character
limit and multifont segment advances. Native arrays are bounds-checked without
signed overflow, and temporary polygon storage is freed on retired graphics.

Raster115 first failed at drawLine, then at drawMFCharsSegment. It now compares
exact server pixels for primitive geometry, XOR reversibility, copying and
translated clips over five allocation/disposal cycles. String, char-array,
byte-array and direct single-font renders have identical nonempty pixel rasters.
A further RED empty-clip test found a retained height; release 0x2f4c4 confirms
both dimensions become zero. Modular coordinates and X11's 16-bit clip storage
are retained explicitly, including the tested truncation boundary.
All 18 focused Linux Debug tests pass; 134 Motif bindings are implemented.
Offscreen images, image delivery/scaling, complete multifont/locale behavior,
printing and remaining window/input peers are still open Phase 6 gates.

## Offscreen image checkpoint

Release 0x32ecc/0x33620/0x36280/0x36314 supplies image storage, offscreen
initialization, disposal and consumer delivery; 0x2e594/0x2fc24 supplies graphics
creation and image copying. The native binding audit now includes the eight
ImageRepresentation/OffScreenImageSource entries against the Solaris inventory.
Five additional adapters are implemented, for 139 entries in total.

Image115 first failed at offscreenInit. It now verifies exact offscreen pixels,
cloned graphics, copying to a window and background clearing over eight cycles.
A consumer removes itself, flushes the image and requests GC during its first
row callback; delivery stops safely. Retained graphics also tolerate image flush.
All 19 focused Linux Debug Motif tests pass (67.16 seconds). Incoming pixel
conversion, transparency, scaling, other visuals and the full platform/demo
gates remain open; this checkpoint does not complete Task 5 or Phase 6.

## Producer image checkpoint

Release 0x336a4/0x338a0 supplies source/array bounds and conversion; 0x33a98
supplies finish/resend behavior. The generic Java ColorModel.getRGB path feeds
the X11 visual. Alpha uses the ordered matrix from 0x36630 and its transposed
lookup at 0x3b6d0. Coverage distinguishes undelivered, transparent and opaque
pixels. image_Done supplies coarse scanline replication for COMPLETESCANLINES.
Drawing follows 0x33b58 and 0x35a34, including XOR, background, clipping and
center-sampled reflection. It skips images without the original availability bits.

Image115 failed first at setIntPixels, then transparency, then imageStretch.
Expanded checks cover exact RGB/indexed pixels, transparent backgrounds, XOR
reversibility, scaled/flipped rectangles, incremental row replication and bounds.
A getRGB callback flushes the image and forces GC: native conversion stops after
one callback. The unchanged Java setPixels then throws NullPointerException
because bytecode 88..122 unconditionally reads newbits cleared by flush; the
fixture records that behavior rather than silently changing the baseline class.
All 19 focused Linux Debug tests pass (71.37 seconds), with 143 bindings.
Image codecs, other visuals, complete fonts and printing still require validation.

## Menu bar and item checkpoint

Release 0x28158..0x29704 recovers menu bars, pulldown/submenus, ordinary and
checkbox items, label/sensitivity updates and disposal. Frame pSetMenuBar and
helper 0x2c434 place the bar above the client and include its height in insets.
The port uses a public XmForm container and preferred widget geometry; the first
height reported by Motif was 16 while its preferred/final height was 30.
Frame screen locations use the shell origin and child placement accounts for
the menu offset. Native menu records are distinct from component peer layouts.

Menus115 failed at MMenuBarPeer.create and later at client-origin geometry.
It now drives real XTest menu selection, checkbox callbacks, label/membership
changes, attach/detach and frame disposal from an action over four cycles.
All 20 focused Linux Debug tests pass (88.00 seconds), with 156 bindings.
Popup menus, full shortcut/input semantics, multifont labels and decorated
window-manager behavior remain open gates.

## Owned window and popup checkpoint

Release 0x2af14..0x2b594 supplies six owned-window bindings and
0x288e0/0x28cd4/0x28e74 supplies popup creation, positioning and disposal.
OwnedWindow115 failed at create, then verified position, resize, painting,
hide/show, stacking and callback disposal over six cycles. Menus115 failed at
popup createMenu, then reproduced a disposal crash. GDB located a freed active
item in Motif ClearTraversalInternal while its parent popup was closing.
Retiring an item now first unmanages its active popup/pulldown parent, while
the traversal target remains alive. Programmatic popup selection and removal
while open pass over four cycles. The current popup path synthesizes its
position event; native mouse-trigger metadata remains part of the input gate.
All 21 focused Linux Debug tests pass (95.71 seconds), with 165 bindings.
Decorated-window geometry, security warning banners and full modal/input
semantics remain open; the owned-window check uses Xvfb without a window manager.

## Dialog checkpoint

Release 0x25628..0x26130 and awt_MToolkit_modalWait establish nine dialog
bindings, modal waiting on the AWT monitor and delayed native-state retirement.
The modal chain filters owner input before Java dispatch and keeps a native
reference while hide/disposal releases the waiting ModalThread. XmDialogShell
and XmForm provide the host dialog. Shared frame operations use only the audited
common component/insets prefix; no Java classes change.
Dialog115 failed at create, then passed four modal action/disposal cycles,
owner-input suppression, modeless title/resizability/show/hide and modal hide
followed by restored owner input. All 22 focused Linux Debug tests pass
(97.78 seconds), with 174 bindings. Nested modality, decorated insets, input
methods and event-dispatch-thread modal behavior still need the full input gate.

## File dialog checkpoint

Release 0x20e38..0x2144c and selection callbacks 0x20b3c/0x20bc8 supply seven
file-dialog bindings. XmFileSelectionDialog handles directory search and file
lists; Java's existing filename filter supplies the explicit list. Selection
and cancellation run the original peer callbacks and release the shared modal
wait. Directory masks use dynamic storage instead of the binary's fixed buffer.
FileDialog115 failed at create, then passed real XTest Open/Save selections,
cancellation, selected directory, font updates and exact filtered native list
counts over four cycles. All 23 focused Linux Debug tests pass (100.79 seconds),
with 181 bindings. Locale/font conversion and decorated-window gates remain open.

## Clipboard checkpoint

Release 0x2ffe4..0x30870 supplies eight selection bindings, Xt ownership,
flavor-to-atom mapping, conversion and asynchronous receive callbacks. Java
callbacks release the AWT monitor; each receive owns its completion state so
simultaneous selections do not share the binary's global completion flags.
TARGETS replies accept XA_ATOM as emitted by standard X11 clients as well as
the original TARGETS type check. Native lists and text are released after transfer.

Clipboard115 failed at init, then an external read caught the need to select
the first matching flavor (a later stream flavor cannot supply String data).
An independent X11 process verifies Java-to-X11 text, ownership-loss callbacks,
XA_ATOM targets and X11-to-Java text over four cycles with GC. The helper's
shutdown pipe needed an explicit flush; GDB located that wait in Process.waitFor.
All 24 focused Linux Debug tests pass (112.57 seconds), with 189 bindings.
Non-ASCII/compound-text locale conversion and clipboard paste into widgets
remain part of the full locale/input gate.

## Shared image codec checkpoint

The reconstructed Windows GIF LZW decoder is now shared C source, preserving
its Sun notice and the recovered ten-argument 1.1.5 ABI. Solaris release 0x3267c
has the corresponding parseImage entry. GDB caught Linux resolving the obsolete
seven-argument runtime stub before libawt; removing that registry entry selects
the AWT decoder without changing either original class archive.

Codec115 verifies completion and transparent drawing. The existing interlaced
and non-interlaced GIF and baseline/gray/progressive JPEG fixtures also run on
Linux, with every rendered RGB pixel checked against direct producer delivery.
The Windows oracle returns blue-channel grayscale for the color fixtures;
Linux TrueColor preserves RGB, so Linux uses its independently checked RGB hashes.
Windows Debug and Release retain the original oracle comparisons (2/2 each).
All 27 focused Linux Debug tests pass (137.48 seconds). JPEG is now a dependency
of the Motif test build. The manual GIF entry is outside the 207 generated-peer
binding count; that count remains 189 implemented. Other visual, font, printing,
input/window and installed-demo completion gates remain open.
