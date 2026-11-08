# Phase 6 implementation decisions

The phase targets the 32-bit Linux X11/OpenMotif host described in
compatibility.md. These are the decisions made while executing the plan,
in chronological order; bootstrap-evidence.md retains recovery and test evidence.

1. Treat the initial class audit as structural preflight, not runtime or native
   layout proof. Cost: linking and layout still required tests in Task 2; those
   gates were subsequently exercised.
2. Reconstruct InputContext's toolkit factory owner/cast as MToolkit, preserving
   baseline SunToolkit. Cost: XIM creation could fail despite clean linkage;
   native input-method and composition tests subsequently passed.
3. Initially define all 76 selected classes but resolve only the component/button
   boundary, because resolution runs static initializers. Cost: toolkit startup
   remained unproved until its native implementation and later live/demo tests.
4. Recover bootstrap font/graphics operations before the first live-window gate.
   Cost: the initial slice grew; full raster/font tests followed in Task 5.
5. Use opaque native-event tokens instead of raw XEvent pointers so disposal can
   retire pending events safely. Cost: internal event-data representation differs
   from Solaris; Java treats it as opaque, and replay/disposal tests exercise it.
6. Query public XIC area attributes instead of Solaris private Motif structure
   offsets. Cost: host XIM geometry may differ; the public host lifecycle and
   composition path is tested, not every Solaris input-method implementation.
7. Advance independent graphics primitives while the remaining widget groups
   were open. Cost: sequencing differed; neither group was declared complete
   before its own checks passed.
8. Use a deterministic indexed color cube and gray ramp instead of Solaris's
   adaptive palette. Cost: exact 8-bit palette/dither parity is not claimed;
   colormap ownership, nearest matching and 8/16/24-bit behavior are exercised.
9. Use EWMH pre-map frame extents because the baseline Window lacks Solaris's
   later isPacked correction. Initially leave classic-WM handling open, then
   explicitly require _NET_REQUEST_FRAME_EXTENTS for decorated Linux windows.
   Cost: classic non-EWMH managers can yield incorrect packed-window insets.
   Bare Xvfb and Openbox are verified; full Solaris WM support remains separate.
10. Preserve the donor X11Selection COMPOUND_TEXT-only flavor-discovery bug.
    Cost: clients relying on advertised flavors cannot paste from such owners;
    direct stringFlavor transfer works and is tested. A Java behavior repair
    belongs in the explicit Solaris class-delta phase, not a silent archive edit.
11. Retain host font metrics and glyph availability. Cost: unavailable glyphs use
    the donor default, and some fixed-size demo layouts require resizing. Latin-1,
    Symbol/Dingbats, XIM and resized MessageFormat layout are verified; exhaustive
    Solaris locale/font pixel parity is not claimed.

The initial audit review's baseline-hash and inherited-field/interface fixture
follow-ups were both completed in Task 2; they are not outstanding deferrals.

The fresh whole-branch review (9beb8ab..e7c353a) found two Important issues:
missing WM-driven movement and iconification callbacks. Both are accepted as
bugs within the supported Openbox boundary, not Solaris compatibility exclusions.
No Critical or separate Minor findings were reported.

Final rulings on behavior the reviewer declined to judge:

- Full Solaris runtime/classic-WM parity, adaptive palette equality and all locale
  fonts remain outside the documented Linux host boundary. Cost: applications
  needing those environments or exact raster output require later work.
- Visual acceptance rests on the implementer's inspection of all 96 client views,
  not a second independent screenshot review or a live Solaris comparison.
  Cost: subtle visual differences from Solaris can remain undetected.
- Linux audio-device operation remains unavailable in this environment; both
  platform suites explicitly skip that test. Cost: real-device audio is unverified.
- Static review and lifetime/concurrency regressions do not prove freedom from
  every scheduler-dependent race. Cost: rare interleavings may still expose defects;
  this is a limit of the evidence, not a claim of exhaustive race freedom.

Outstanding deferred Minor review findings: none.
