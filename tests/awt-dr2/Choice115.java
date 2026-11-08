import java.awt.*;
import java.awt.event.*;

public class Choice115 {
    static { System.loadLibrary("choice115"); }
    static native boolean state(Choice choice, String[] items, int selection);
    static native void choose(Choice choice, int index);
    static native void chooseQueued(Choice choice);
    static native void track();
    static native int liveChoices();
    static native void failConstruction(boolean fail);
    static volatile int events;
    static volatile boolean disposeOnEvent;
    static volatile Throwable failure;
    static final RuntimeException marker = new RuntimeException("choice font failure");
    static class BadFont extends Font {
        BadFont() { super("Dialog", Font.PLAIN, 12); }
        public int getSize() { throw marker; }
    }
    static void drained() throws Exception {
        for (int wait = 0; wait < 40 && liveChoices() != 0; wait++) {
            Thread.sleep(50); Toolkit.getDefaultToolkit().sync(); System.gc();
        }
        require(liveChoices() == 0, "native choices retained");
    }
    static void failedConstruction(boolean nativeFailure) throws Exception {
        Choice choice = new Choice(); choice.add("failure");
        if (!nativeFailure) choice.setFont(new BadFont());
        failConstruction(nativeFailure);
        try { choice.addNotify(); throw new Error("choice construction succeeded"); }
        catch (RuntimeException problem) {
            if (!nativeFailure) require(problem == marker, "choice throwable identity");
            else require(problem.getMessage().indexOf("choice construction failure") >= 0, "native choice exception");
        } finally { failConstruction(false); }
        require(choice.getPeer() == null, "failed choice peer"); drained();
    }
    static void require(boolean value, String message) { if (!value) throw new Error(message); }
    public static void main(String[] args) {
        try {
            boolean tracked = args.length != 0;
            if (tracked) {
                Toolkit.getDefaultToolkit(); track();
                for (int pass = 0; pass < 3; pass++) { failedConstruction(false); failedConstruction(true); }
            }
            for (int pass = 0; pass < 3; pass++) {
                final Frame frame = new Frame(); frame.setLayout(null); frame.setBounds(350, 100, 300, 180);
                disposeOnEvent = false;
                final Choice choice = new Choice(); choice.setBounds(20, 40, 240, 30);
                choice.add("same"); choice.add("same"); choice.add("Caf\u00e9"); choice.select(1);
                frame.add(choice); frame.show();
                require(state(choice, new String[] {"same", "same", "Caf\u00e9"}, 1), "initial choice duplicates");
                choice.insert("inserted", 1); choice.add(""); choice.select(3);
                require(state(choice, new String[] {"same", "inserted", "same", "Caf\u00e9", ""}, 3), "insert/select");
                choice.remove(1); choice.select(2);
                require(state(choice, new String[] {"same", "same", "Caf\u00e9", ""}, 2), "remove/select");
                choice.addItemListener(new ItemListener() {
                    public void itemStateChanged(ItemEvent event) {
                        try {
                            require(event.getSource() == choice && event.getID() == ItemEvent.ITEM_STATE_CHANGED &&
                                event.getStateChange() == ItemEvent.SELECTED && event.getItem().equals("same"), "choice item event");
                            require(choice.getSelectedIndex() == 1, "choice Java selection");
                            Toolkit.getDefaultToolkit().sync(); System.gc();
                            if (disposeOnEvent) frame.dispose();
                            events++;
                        } catch (Throwable problem) { failure = problem; }
                    }
                });
                int before = events; choice.select(0);
                require(events == before, "programmatic choice callback");
                choose(choice, 1);
                for (int wait = 0; wait < 200 && events == before && failure == null; wait++) Thread.sleep(10);
                if (failure != null) throw new Error(failure.toString());
                require(events == before + 1, "choice callback delivery");
                choice.select(0); before = events; chooseQueued(choice);
                for (int wait = 0; wait < 200 && events == before && failure == null; wait++) Thread.sleep(10);
                if (failure != null) throw new Error(failure.toString());
                require(events == before + 1 && choice.getSelectedIndex() == 1, "queued choice keyboard selection");
                choice.removeAll(); require(state(choice, new String[0], -1), "empty choice");
                choice.add("new"); require(state(choice, new String[] {"new"}, 0), "refilled choice");
                if (tracked) {
                    choice.add("same"); disposeOnEvent = true; before = events;
                    choose(choice, 1);
                    for (int wait = 0; wait < 200 && events == before && failure == null; wait++) Thread.sleep(10);
                    if (failure != null) throw new Error(failure.toString());
                    require(events == before + 1 && choice.getPeer() == null, "choice callback disposal");
                }
                frame.dispose(); Toolkit.getDefaultToolkit().sync(); System.gc();
                require(choice.getPeer() == null, "disposed choice peer");
                if (tracked) drained();
            }
            System.out.println("DR2 choices preserve duplicate items, selection and item callbacks");
            System.exit(0);
        } catch (Throwable problem) { problem.printStackTrace(); System.exit(1); }
    }
}
