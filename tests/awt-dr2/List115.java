import java.awt.*;
import java.awt.event.*;

public class List115 {
    static { System.loadLibrary("list115"); }
    static native void click(List list, int row, int count);
    static native void track();
    static native int retainedCallbackRefs();
    static native int failedDispatch(List list);
    static native int liveLists();
    static native void failConstruction(boolean fail);
    static volatile int items, actions;
    static volatile Throwable failure;
    static volatile boolean disposeOnEvent;
    static void awaitItem(int before) throws Exception {
        for (int wait = 0; wait < 200 && items == before && failure == null; wait++) Thread.sleep(10);
        if (failure != null) throw new Error(failure.toString());
        require(items > before, "missing native item event");
    }
    static void drained() throws Exception {
        for (int wait = 0; wait < 40 && liveLists() != 0; wait++) {
            Toolkit.getDefaultToolkit().sync(); System.gc(); Thread.sleep(50);
        }
        require(liveLists() == 0, "native lists or tables retained");
    }
    static final RuntimeException marker = new RuntimeException("list font failure");
    static class BadFont extends Font {
        BadFont() { super("Dialog", Font.PLAIN, 12); }
        public int getSize() { throw marker; }
    }
    static void failedConstruction(boolean nativeFailure, boolean multiple) throws Exception {
        List list = new List(4, multiple);
        if (!nativeFailure) list.setFont(new BadFont());
        failConstruction(nativeFailure);
        try { list.addNotify(); throw new Error("list construction succeeded"); }
        catch (RuntimeException problem) {
            if (!nativeFailure) require(problem == marker, "list throwable identity");
            else require(problem.getMessage().indexOf("list construction failure") >= 0, "native constructor exception");
        } finally { failConstruction(false); }
        require(list.getPeer() == null, "failed list peer"); drained();
    }
    static void require(boolean value, String message) { if (!value) throw new Error(message); }
    static void selection(List list, int first, int second) {
        int[] selected = list.getSelectedIndexes();
        require(selected.length == (first < 0 ? 0 : second < 0 ? 1 : 2), "selection count");
        if (first >= 0) require(selected[0] == first, "first selection");
        if (second >= 0) require(selected[1] == second, "second selection");
    }
    public static void main(String[] args) {
        try {
            boolean tracked = args.length != 0;
            if (tracked) {
                Toolkit.getDefaultToolkit(); track();
                failedConstruction(false, false); failedConstruction(true, false);
                failedConstruction(false, true); failedConstruction(true, true);
            }
            for (int mode = 0; mode < 2; mode++) {
                final Frame frame = new Frame(); frame.setLayout(null); frame.setBounds(350, 100, 300, 280);
                final List list = new List(4, mode != 0); list.setBounds(20, 40, 220, 180);
                list.add("duplicate"); list.add("duplicate"); list.add("caf\u00e9"); list.add("");
                frame.add(list); frame.show();
                require(list.getItemCount() == 4 && list.getItem(2).equals("caf\u00e9"), "initial items");
                Dimension small = list.getPreferredSize(2), large = list.getPreferredSize(6);
                System.out.println(mode + ":" + small.width + "x" + small.height + ":" + large.width + "x" + large.height);
                list.select(1); selection(list, 1, -1);
                /* DR2 preserves prior ItemPair flags even in single-selection mode. */
                list.select(3); selection(list, 1, 3);
                list.deselect(3); selection(list, 1, -1);
                list.add("insert", 0);
                selection(list, 2, -1);
                list.remove(0); list.makeVisible(2);
                list.setEnabled(false); list.setEnabled(true);
                list.setForeground(Color.blue); list.setBackground(Color.white);
                list.setFont(new Font("Dialog", Font.PLAIN, 18));
                require(list.getPreferredSize(6).height > large.height, "font sizing");
                int[] oldSelection = list.getSelectedIndexes();
                for (int index = 0; index < oldSelection.length; index++) list.deselect(oldSelection[index]);
                list.setMultipleMode(true); list.select(0); list.select(2); selection(list, 0, 2);
                list.removeAll(); selection(list, -1, -1);
                list.add("alpha"); list.add("caf\u00e9"); list.add("gamma");
                disposeOnEvent = false;
                list.addItemListener(new ItemListener() {
                    public void itemStateChanged(ItemEvent event) {
                        try {
                            require(event.getSource() == list && event.getID() == ItemEvent.ITEM_STATE_CHANGED, "item event source");
                            require(event.getItem() instanceof Integer, "item index type");
                            Toolkit.getDefaultToolkit().sync(); System.gc();
                            if (disposeOnEvent) frame.dispose();
                            items++;
                        } catch (Throwable problem) { failure = problem; }
                    }
                });
                list.addActionListener(new ActionListener() {
                    public void actionPerformed(ActionEvent event) {
                        try {
                            require(event.getSource() == list && event.getActionCommand().equals("caf\u00e9"), "list action command");
                            Toolkit.getDefaultToolkit().sync(); System.gc(); actions++;
                        } catch (Throwable problem) { failure = problem; }
                    }
                });
                list.setEnabled(false); int disabledBefore = items; click(list, 1, 1);
                Thread.sleep(150); require(items == disabledBefore, "disabled native input");
                list.setEnabled(true);
                int before = items; click(list, 1, 1); awaitItem(before); Thread.sleep(100);
                selection(list, 1, -1);
                int beforeAction = actions; click(list, 1, 2);
                for (int wait = 0; wait < 200 && actions == beforeAction && failure == null; wait++) Thread.sleep(10);
                if (failure != null) throw new Error(failure.toString());
                require(actions > beforeAction, "missing double-click action");
                if (tracked) {
                    int retained = failedDispatch(list);
                    require(retained == 0, "exceptional list callback JNI references: " + retained);
                    disposeOnEvent = true; before = items; click(list, 2, 1); awaitItem(before);
                    require(list.getPeer() == null, "list callback disposal");
                } else list.removeAll();
                if (!tracked) { list.add("refilled"); list.select(0); selection(list, 0, -1); }
                frame.dispose(); Toolkit.getDefaultToolkit().sync(); System.gc();
                require(list.getPeer() == null, "disposed list");
                if (tracked) drained();
            }
            if (tracked) require(retainedCallbackRefs() == 0, "callback JNI references retained: " + retainedCallbackRefs());
            System.out.println("DR2 lists preserve items, selection and row sizing");
            System.exit(0);
        } catch (Throwable problem) { problem.printStackTrace(); System.exit(1); }
    }
}
