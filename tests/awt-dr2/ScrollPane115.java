import java.awt.*;
import java.awt.event.*;

public class ScrollPane115 {
    static { System.loadLibrary("scrollpane115"); }
    static native void click(ScrollPane pane, int part, boolean vertical);
    static native void drag(ScrollPane pane, boolean vertical);
    static native boolean state(ScrollPane pane, int x, int y, boolean enabled);
    static native void track();
    static native int livePanes();
    static native void failConstruction(boolean fail);
    static volatile int events;
    static volatile Throwable failure;
    static volatile boolean disposeOnEvent;
    static void awaitEvent(int before) throws Exception {
        for (int wait = 0; wait < 200 && events == before && failure == null; wait++) Thread.sleep(10);
        if (failure != null) throw new Error(failure.toString());
        require(events > before, "missing native adjustment");
    }
    static void drained() throws Exception {
        for (int wait = 0; wait < 40 && livePanes() != 0; wait++) {
            Toolkit.getDefaultToolkit().sync(); System.gc(); Thread.sleep(50);
        }
        require(livePanes() == 0, "native scroll panes retained");
    }
    static final RuntimeException marker = new RuntimeException("scroll pane color failure");
    static class BadColor extends Color {
        BadColor() { super(0); }
        public int getRGB() { throw marker; }
    }
    static void failedConstruction(boolean nativeFailure, int policy) throws Exception {
        ScrollPane pane = new ScrollPane(policy);
        if (!nativeFailure) pane.setBackground(new BadColor());
        failConstruction(nativeFailure);
        try { pane.addNotify(); throw new Error("scroll pane construction succeeded"); }
        catch (RuntimeException problem) {
            if (!nativeFailure) require(problem == marker, "scroll pane throwable identity");
            else require(problem.getMessage().indexOf("scroll pane construction failure") >= 0, "native construction exception");
        } finally { failConstruction(false); }
        require(pane.getPeer() == null, "failed scroll pane peer"); drained();
    }
    static class Child extends Canvas {
        Dimension preferred = new Dimension(420, 360);
        public Dimension getPreferredSize() { return preferred; }
    }
    static void require(boolean value, String message) {
        if (!value) throw new Error(message);
    }
    static void geometry(ScrollPane pane, int policy, String size) {
        Insets insets = pane.getInsets();
        Dimension viewport = pane.getViewportSize();
        System.out.println(policy + ":" + size + ":" + insets.top + "," + insets.left + "," +
            insets.bottom + "," + insets.right + ":" + viewport.width + "x" + viewport.height);
    }
    public static void main(String[] args) {
        try {
            boolean tracked = args.length != 0;
            if (tracked) {
                Toolkit.getDefaultToolkit(); track();
                for (int policy = 0; policy < 3; policy++) {
                    failedConstruction(false, policy); failedConstruction(true, policy);
                }
            }
            for (int policy = 0; policy < 3; policy++) {
                final Frame frame = new Frame(); frame.setLayout(null); frame.setBounds(350, 100, 300, 280);
                final ScrollPane pane = new ScrollPane(policy); pane.setBounds(20, 40, 220, 180);
                Child child = new Child(); pane.add(child); frame.add(pane); frame.show(); frame.validate();
                Toolkit.getDefaultToolkit().sync();
                Dimension preferred = pane.getPeer().getPreferredSize();
                require(preferred.width == 100 && preferred.height == 100, "native preferred size");
                geometry(pane, policy, "large");
                System.out.println(policy + ":bars:" + pane.getHScrollbarHeight() + "," + pane.getVScrollbarWidth());
                pane.setScrollPosition(31, 47);
                require(pane.getScrollPosition().equals(new Point(31, 47)), "scroll position");
                pane.getHAdjustable().setUnitIncrement(7); pane.getVAdjustable().setUnitIncrement(9);
                pane.getHAdjustable().setValue(53); pane.getVAdjustable().setValue(61);
                require(pane.getScrollPosition().equals(new Point(53, 61)), "adjustable position");
                require(state(pane, 53, 61, true), "native position and increments");
                pane.setEnabled(false);
                require(state(pane, 53, 61, false), "disabled native scrollers");
                pane.setEnabled(true);
                disposeOnEvent = false;
                AdjustmentListener listener = new AdjustmentListener() {
                    public void adjustmentValueChanged(AdjustmentEvent event) {
                        try {
                            Toolkit.getDefaultToolkit().sync(); System.gc();
                            require(event.getValue() == ((Adjustable)event.getSource()).getValue(), "adjustment value");
                            if (disposeOnEvent) frame.dispose();
                            events++;
                        } catch (Throwable problem) { failure = problem; }
                    }
                };
                pane.getHAdjustable().addAdjustmentListener(listener);
                pane.getVAdjustable().addAdjustmentListener(listener);
                if (policy != ScrollPane.SCROLLBARS_NEVER) {
                    for (int axis = 0; axis < 2; axis++) {
                        boolean vertical = axis == 1;
                        pane.setScrollPosition(50, 50); int before = events;
                        click(pane, 5, vertical); awaitEvent(before); Thread.sleep(100);
                        Point position = pane.getScrollPosition();
                        require((vertical ? position.y : position.x) > 50, "native arrow direction");
                        before = events; drag(pane, vertical); awaitEvent(before); Thread.sleep(100);
                        position = pane.getScrollPosition();
                        require((vertical ? position.y : position.x) > 80, "native knob drag");
                    }
                }
                child.preferred = new Dimension(40, 30); child.invalidate(); pane.invalidate(); frame.validate();
                geometry(pane, policy, "small");
                pane.setScrollPosition(0, 0);
                child.hide(); child.show();
                if (tracked && policy != ScrollPane.SCROLLBARS_NEVER) {
                    child.preferred = new Dimension(420, 360); child.invalidate(); pane.invalidate(); frame.validate();
                    pane.setScrollPosition(50, 50); disposeOnEvent = true;
                    int before = events; click(pane, 5, true); awaitEvent(before);
                    require(pane.getPeer() == null, "callback disposal");
                }
                frame.dispose(); Toolkit.getDefaultToolkit().sync(); System.gc();
                require(pane.getPeer() == null && child.getPeer() == null, "disposed pane and child");
                if (tracked) drained();
            }
            System.out.println("DR2 scroll panes preserve policies, viewport sizing and adjustable positions");
            System.exit(0);
        } catch (Throwable problem) { problem.printStackTrace(); System.exit(1); }
    }
}
