import java.awt.*;
import java.awt.event.*;

public class Checkbox115 implements ItemListener {
    static { System.loadLibrary("checkbox115"); }
    static native void click(Checkbox box);
    static native boolean state(Checkbox box, String label, boolean selected);
    static native void inject(Checkbox box);
    static native void track();
    static native void failConstruction(boolean fail);
    static native int liveButtons();
    static final RuntimeException marker = new RuntimeException("checkbox font failure");
    static class BadFont extends Font {
        BadFont() { super("Dialog", Font.PLAIN, 12); }
        public int getSize() { throw marker; }
    }
    static void drained() throws Exception {
        for (int wait = 0; wait < 40 && liveButtons() != 0; wait++) {
            Thread.sleep(50); Toolkit.getDefaultToolkit().sync(); System.gc();
        }
        require(liveButtons() == 0, "native checkboxes retained");
    }
    static void failedConstruction(boolean nativeFailure, boolean grouped) throws Exception {
        Checkbox box = new Checkbox("failure", grouped ? new CheckboxGroup() : null, true);
        if (!nativeFailure) box.setFont(new BadFont());
        failConstruction(nativeFailure);
        try { box.addNotify(); throw new Error("checkbox construction unexpectedly succeeded"); }
        catch (RuntimeException failure) {
            if (!nativeFailure) require(failure == marker, "checkbox throwable identity");
            else require(failure.getMessage().indexOf("checkbox construction failure") >= 0, "native checkbox exception");
        } finally { failConstruction(false); }
        require(box.getPeer() == null, "failed checkbox peer"); drained();
    }
    volatile int events;
    volatile ItemEvent last;
    volatile Throwable failure;
    Frame closeOnEvent;
    public void itemStateChanged(ItemEvent event) {
        try {
            Checkbox box = (Checkbox)event.getSource();
            require(event.getID() == ItemEvent.ITEM_STATE_CHANGED, "item ID");
            require(event.getItem().equals(box.getLabel()), "item label");
            Toolkit.getDefaultToolkit().sync(); System.gc();
            if (closeOnEvent != null) { closeOnEvent.dispose(); Toolkit.getDefaultToolkit().sync(); System.gc(); }
            last = event; events++;
        } catch (Throwable problem) { failure = problem; }
    }
    static void require(boolean value, String message) { if (!value) throw new Error(message); }
    void clicked(Checkbox box, boolean selected) throws Exception {
        int before = events; click(box);
        for (int wait = 0; wait < 100 && events == before && failure == null; wait++) Thread.sleep(10);
        if (failure != null) throw new Error("item callback failed: " + failure);
        require(events == before + 1 && last.getSource() == box, "item delivery");
        require(box.getState() == selected && state(box, box.getLabel(), selected), "native/Java selection");
    }
    public static void main(String[] args) {
        try {
            if (args.length != 0) {
                Toolkit.getDefaultToolkit(); track();
                for (int pass = 0; pass < 3; pass++) {
                    failedConstruction(false, pass != 0); failedConstruction(true, pass != 0);
                }
            }
            for (int pass = 0; pass < 3; pass++) {
                Checkbox115 listener = new Checkbox115();
                Frame frame = new Frame(); frame.setLayout(null); frame.setBounds(350, 100, 300, 200);
                Checkbox box = new Checkbox("check", false);
                CheckboxGroup group = new CheckboxGroup();
                Checkbox first = new Checkbox("first", group, true), second = new Checkbox("second", group, false);
                Checkbox[] boxes = {box, first, second};
                for (int i = 0; i < boxes.length; i++) {
                    boxes[i].setBounds(20, 40 + i * 40, 180, 30); boxes[i].addItemListener(listener); frame.add(boxes[i]);
                }
                frame.show();
                require(state(box, "check", false) && state(first, "first", true) && state(second, "second", false), "initial state");
                box.setLabel("Caf\u00e9"); box.setState(true);
                require(state(box, "Caf\u00e9", true) && listener.events == 0, "programmatic state/label");
                listener.clicked(box, false); require(listener.last.getStateChange() == ItemEvent.DESELECTED, "deselected event");
                listener.clicked(box, true); require(listener.last.getStateChange() == ItemEvent.SELECTED, "selected event");
                listener.clicked(second, true);
                require(group.getSelectedCheckbox() == second && !first.getState() && state(first, "first", false), "radio exclusivity");
                listener.clicked(second, true);
                second.setCheckboxGroup(null);
                require(!second.getState() && state(second, "second", false), "group removal clears selection");
                listener.clicked(second, true); listener.clicked(second, false);
                box.setCheckboxGroup(group); box.setState(true);
                require(group.getSelectedCheckbox() == box, "dynamic group membership");
                listener.clicked(first, true);
                require(!box.getState() && state(box, "Caf\u00e9", false), "dynamic radio exclusivity");
                int before = listener.events; first.setEnabled(false); click(first); Thread.sleep(100);
                require(listener.events == before, "disabled checkbox event");
                if (args.length != 0) listener.closeOnEvent = frame;
                inject(box);
                for (int wait = 0; wait < 200 && listener.events == before && listener.failure == null; wait++) Thread.sleep(10);
                if (listener.failure != null) throw new Error("queued callback: " + listener.failure);
                require(listener.events == before + 1 && listener.last.getSource() == box && box.getState(), "queued checkbox input");
                if (args.length != 0) require(box.getPeer() == null, "item callback disposal");
                frame.dispose(); Toolkit.getDefaultToolkit().sync(); System.gc();
                for (int i = 0; i < boxes.length; i++) require(boxes[i].getPeer() == null, "checkbox peer retained");
                if (args.length != 0) drained();
            }
            System.out.println("DR2 checkboxes preserve selection, radio groups and item callbacks");
            System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
