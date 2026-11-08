import java.awt.*;
import java.awt.event.*;

public class Button115 {
    static { System.loadLibrary("button115"); }
    static native void click(Button button);
    static native void inject(Button button);
    static native boolean state(Button button, String title, boolean enabled, int size);
    static native void track();
    static native int liveButtons();
    static native void failConstruction(boolean fail);
    static native boolean foreground(Button button);
    static final RuntimeException marker = new RuntimeException("button font failure");
    static class BadFont extends Font {
        BadFont() { super("Dialog", Font.PLAIN, 12); }
        public int getSize() { throw marker; }
    }
    static void drained() throws Exception {
        for (int wait = 0; wait < 40 && liveButtons() != 0; wait++) {
            Thread.sleep(50); System.gc(); Toolkit.getDefaultToolkit().sync();
        }
        require(liveButtons() == 0, "native buttons retained: " + liveButtons());
    }
    static void failedConstruction(boolean nativeFailure) throws Exception {
        Button button = new Button("failure");
        if (!nativeFailure) button.setFont(new BadFont());
        failConstruction(nativeFailure);
        try {
            button.addNotify();
            throw new Error("button construction unexpectedly succeeded");
        } catch (RuntimeException failure) {
            if (!nativeFailure) require(failure == marker, "button throwable identity changed");
            else require(failure.getMessage().indexOf("button construction failure") >= 0, "native button exception changed");
        } finally { failConstruction(false); }
        require(button.getPeer() == null, "failed button installed peer"); drained();
    }
    static volatile int actions;
    static volatile Throwable callbackFailure;
    static void require(boolean value, String message) { if (!value) throw new Error(message); }
    public static void main(String[] args) {
        try {
            if (args.length != 0) {
                Toolkit.getDefaultToolkit(); track();
                for (int pass = 0; pass < 3; pass++) { failedConstruction(false); failedConstruction(true); }
            }
            for (int pass = 0; pass < 3; pass++) {
                final Frame frame = new Frame(); frame.setLayout(null); frame.setBounds(350, 100, 300, 180);
                final Button button = new Button("Start"); button.setBounds(30, 40, 160, 35);
                button.setFont(new Font("Dialog", Font.PLAIN, 12));
                frame.add(button); frame.show();
                require(state(button, "Start", true, 12), "initial button state");
                Dimension initial = button.getPeer().getPreferredSize();
                button.setLabel("A much longer button title");
                require(button.getPeer().getPreferredSize().width > initial.width, "button preferred size follows title");
                button.setLabel("Caf\u00e9"); button.setFont(new Font("Dialog", Font.BOLD, 18));
                button.setForeground(Color.red); button.setBackground(Color.yellow);
                button.setCursor(new Cursor(Cursor.HAND_CURSOR));
                require(state(button, "Caf\u00e9", true, 18), "updated button state");
                require(foreground(button), "button attributed foreground");
                button.setActionCommand("run");
                button.addActionListener(new ActionListener() {
                    public void actionPerformed(ActionEvent event) {
                        try {
                            require(event.getSource() == button && event.getID() == ActionEvent.ACTION_PERFORMED, "action source/id");
                            require(event.getActionCommand().equals("run"), "action command");
                            button.setLabel("clicked"); Toolkit.getDefaultToolkit().sync(); System.gc();
                            actions++;
                        } catch (Throwable failure) { callbackFailure = failure; }
                    }
                });
                int before = actions;
                click(button);
                for (int wait = 0; wait < 100 && actions == before && callbackFailure == null; wait++) Thread.sleep(10);
                if (callbackFailure != null) throw callbackFailure;
                require(actions == before + 1 && state(button, "clicked", true, 18), "action delivery/reentry");
                button.setEnabled(false); require(state(button, "clicked", false, 18), "disabled button state");
                click(button); Thread.sleep(100); require(actions == before + 1, "disabled button dispatched action");
                button.setEnabled(true); require(state(button, "clicked", true, 18), "enabled button state");
                if (args.length != 0) {
                    button.addActionListener(new ActionListener() {
                        public void actionPerformed(ActionEvent event) {
                            try { frame.dispose(); Toolkit.getDefaultToolkit().sync(); System.gc(); }
                            catch (Throwable failure) { callbackFailure = failure; }
                        }
                    });
                }
                inject(button);
                for (int wait = 0; wait < 200 && callbackFailure == null &&
                    (actions < before + 2 || (args.length != 0 && button.getPeer() != null)); wait++) Thread.sleep(10);
                if (callbackFailure != null) throw callbackFailure;
                require(actions == before + 2, "queued native click action");
                if (args.length != 0) require(button.getPeer() == null, "action callback disposal");
                frame.dispose(); Toolkit.getDefaultToolkit().sync(); System.gc();
                require(button.getPeer() == null, "button peer retained");
                if (args.length != 0) drained();
            }
            System.out.println("DR2 buttons preserve labels, sizing, enabled state and action callbacks");
            System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}


