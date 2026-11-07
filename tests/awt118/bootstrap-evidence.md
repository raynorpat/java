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
