import java.awt.*;
import java.awt.event.*;

public class Scrollbar115 {
    static { System.loadLibrary("scrollbar115"); }
    static native boolean state(Scrollbar bar, int value, int visible, int minimum, int maximum);
    static native void click(Scrollbar bar, int part);
    static native void drag(Scrollbar bar, boolean vertical);
    static native void track();
    static native int liveScrollers();
    static native void failConstruction(boolean fail);
    static volatile int events, lastType, lastValue;
    static volatile boolean disposeOnEvent;
    static volatile Throwable failure;
    static final RuntimeException marker = new RuntimeException("scrollbar color failure");
    static class BadColor extends Color {
        BadColor() { super(0); }
        public int getRGB() { throw marker; }
    }
    static void drained() throws Exception {
        for (int wait = 0; wait < 40 && liveScrollers() != 0; wait++) {
            Thread.sleep(50); Toolkit.getDefaultToolkit().sync(); System.gc();
        }
        require(liveScrollers() == 0, "native scrollbars retained");
    }
    static void failedConstruction(boolean nativeFailure, int orientation) throws Exception {
        Scrollbar bar = new Scrollbar(orientation);
        if (!nativeFailure) bar.setBackground(new BadColor());
        failConstruction(nativeFailure);
        try { bar.addNotify(); throw new Error("scrollbar construction succeeded"); }
        catch (RuntimeException problem) {
            if (!nativeFailure) require(problem == marker, "scrollbar throwable identity");
            else require(problem.getMessage().indexOf("scrollbar construction failure") >= 0, "native scrollbar exception");
        } finally { failConstruction(false); }
        require(bar.getPeer() == null, "failed scrollbar peer"); drained();
    }
    static void awaitEvent(int before) throws Exception {
        for (int wait = 0; wait < 200 && events == before && failure == null; wait++) Thread.sleep(10);
        if (failure != null) throw new Error(failure.toString());
        require(events > before, "missing scrollbar event");
    }
    static void require(boolean value, String message) { if (!value) throw new Error(message); }
    public static void main(String[] args) {
        try {
            boolean tracked = args.length != 0;
            if (tracked) {
                Toolkit.getDefaultToolkit(); track();
                for (int orientation = 0; orientation < 2; orientation++) {
                    failedConstruction(false, orientation); failedConstruction(true, orientation);
                }
            }
            for (int orientation = 0; orientation < 2; orientation++) {
                final Frame frame = new Frame(); frame.setLayout(null); frame.setBounds(350, 100, 300, 280);
                disposeOnEvent = false;
                final Scrollbar bar = new Scrollbar(orientation, 40, 20, -20, 180);
                bar.setBounds(20, 40, orientation == 0 ? 220 : 22, orientation == 0 ? 22 : 180);
                bar.setUnitIncrement(7); bar.setBlockIncrement(23);
                frame.add(bar); frame.show();
                require(state(bar, 40, 20, -20, 180), "initial scrollbar values");
                Dimension size = bar.getPeer().getPreferredSize();
                System.out.println(orientation + ":" + size.width + "x" + size.height);
                require(size.width == (orientation == 0 ? 50 : 18) && size.height == (orientation == 0 ? 18 : 50), "scrollbar preferred size");
                bar.addAdjustmentListener(new AdjustmentListener() {
                    public void adjustmentValueChanged(AdjustmentEvent event) {
                        try {
                            require(event.getSource() == bar && event.getID() == AdjustmentEvent.ADJUSTMENT_VALUE_CHANGED, "adjustment source");
                            require(bar.getValue() == event.getValue(), "Java adjustment value");
                            Toolkit.getDefaultToolkit().sync(); System.gc();
                            if (disposeOnEvent) frame.dispose();
                            lastType = event.getAdjustmentType(); lastValue = event.getValue(); events++;
                        } catch (Throwable problem) { failure = problem; }
                    }
                });
                int before = events; bar.setValues(40, 20, -20, 180);
                require(events == before, "programmatic adjustment callback");
                click(bar, 5);
                for (int wait = 0; wait < 200 && events == before && failure == null; wait++) Thread.sleep(10);
                if (failure != null) throw new Error(failure.toString());
                require(events == before + 1 && lastType == AdjustmentEvent.UNIT_INCREMENT && lastValue == 47, "native increment arrow");
                require(state(bar, 47, 20, -20, 180), "updated native value");
                int[] parts = {4, 3, 1}, types = {AdjustmentEvent.UNIT_DECREMENT, AdjustmentEvent.BLOCK_INCREMENT, AdjustmentEvent.BLOCK_DECREMENT};
                int[] values = {33, 63, 17};
                for (int index = 0; index < parts.length; index++) {
                    bar.setValue(40); before = events; click(bar, parts[index]); awaitEvent(before);
                    require(lastType == types[index] && lastValue == values[index], "native step type/value");
                }
                bar.setValue(40); before = events; drag(bar, orientation == 1); awaitEvent(before);
                Thread.sleep(100);
                require(lastType == AdjustmentEvent.TRACK && bar.getValue() > 40 && bar.getValue() <= 160, "native knob drag");
                bar.setUnitIncrement(9); bar.setBlockIncrement(31); bar.setValue(40);
                before = events; click(bar, 5); awaitEvent(before);
                require(lastType == AdjustmentEvent.UNIT_INCREMENT && lastValue == 49, "changed unit increment");
                bar.setValue(40); before = events; click(bar, 3); awaitEvent(before);
                require(lastType == AdjustmentEvent.BLOCK_INCREMENT && lastValue == 71, "changed block increment");
                bar.setValue(40); bar.setEnabled(false); before = events; click(bar, 5);
                Thread.sleep(150); require(events == before && bar.getValue() == 40, "disabled scrollbar input");
                bar.setEnabled(true);
                bar.setValues(500, 20, -20, 180); require(state(bar, 160, 20, -20, 180), "upper clamping");
                bar.setValues(-100, 20, -20, 180); require(state(bar, -20, 20, -20, 180), "lower clamping");
                bar.setValues(0, 200, -20, 180); require(state(bar, -20, 200, -20, 180), "full extent");
                if (tracked) {
                    bar.setValues(40, 20, -20, 180); disposeOnEvent = true;
                    before = events; click(bar, 5); awaitEvent(before);
                    require(bar.getPeer() == null, "scrollbar callback disposal");
                }
                frame.dispose(); Toolkit.getDefaultToolkit().sync(); System.gc();
                require(bar.getPeer() == null, "disposed scrollbar");
                if (tracked) drained();
            }
            System.out.println("DR2 scrollbars preserve ranges, extents and native adjustment callbacks");
            System.exit(0);
        } catch (Throwable problem) { problem.printStackTrace(); System.exit(1); }
    }
}
