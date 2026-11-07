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
