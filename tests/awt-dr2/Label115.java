import java.awt.*;
import java.awt.event.*;

public class Label115 {
    static { System.loadLibrary("label115"); }
    static native boolean state(Label label, String text, int alignment, boolean enabled, int size);
    static native boolean colors(Label label);
    static native void track();
    static native void failConstruction(boolean fail);
    static native int liveLabels();
    static native void inject(Label label);
    static volatile int clicks;
    static volatile Throwable callbackFailure;
    static final RuntimeException marker = new RuntimeException("label font failure");
    static class BadFont extends Font {
        BadFont() { super("Dialog", Font.PLAIN, 12); }
        public int getSize() { throw marker; }
    }
    static void drained() throws Exception {
        for (int wait = 0; wait < 40 && liveLabels() != 0; wait++) {
            Thread.sleep(50); Toolkit.getDefaultToolkit().sync(); System.gc();
        }
        require(liveLabels() == 0, "native labels retained");
    }
    static void failedConstruction(boolean nativeFailure) throws Exception {
        Label label = new Label("failure");
        if (!nativeFailure) label.setFont(new BadFont());
        failConstruction(nativeFailure);
        try { label.addNotify(); throw new Error("label construction unexpectedly succeeded"); }
        catch (RuntimeException failure) {
            if (!nativeFailure) require(failure == marker, "label throwable identity");
            else require(failure.getMessage().indexOf("label construction failure") >= 0, "native label exception");
        } finally { failConstruction(false); }
        require(label.getPeer() == null, "failed label peer"); drained();
    }
    static void require(boolean value, String message) { if (!value) throw new Error(message); }
    public static void main(String[] args) {
        try {
            if (args.length != 0) {
                Toolkit.getDefaultToolkit(); track();
                for (int pass = 0; pass < 3; pass++) { failedConstruction(false); failedConstruction(true); }
            }
            for (int pass = 0; pass < 3; pass++) {
                final Frame frame = new Frame(); frame.setLayout(null); frame.setBounds(350, 100, 300, 180);
                final Label label = new Label(null, pass); label.setBounds(20, 40, 240, 40);
                final boolean disposeInCallback = args.length != 0;
                label.setFont(new Font("Dialog", Font.PLAIN, 12));
                frame.add(label); frame.show();
                require(state(label, "", pass, true, 12), "initial label state");
                require(!label.getPeer().isFocusTraversable(), "label focus traversal");
                label.setText("short"); Dimension small = label.getPeer().getPreferredSize();
                label.setText("A much longer label");
                require(label.getPeer().getPreferredSize().width > small.width, "label preferred width");
                label.setText("Caf\u00e9"); label.setFont(new Font("Dialog", Font.BOLD, 18));
                label.setForeground(Color.red); label.setBackground(Color.yellow);
                for (int alignment = 0; alignment < 3; alignment++) {
                    label.setAlignment(alignment);
                    require(state(label, "Caf\u00e9", alignment, true, 18), "updated alignment/text/font");
                }
                require(colors(label), "label native colors");
                label.setEnabled(false); require(state(label, "Caf\u00e9", 2, false, 18), "disabled label");
                label.setEnabled(true); label.setText(null);
                require(state(label, "", 2, true, 18), "null label text");
                label.setCursor(new Cursor(Cursor.HAND_CURSOR));
                label.hide(); label.show(); Toolkit.getDefaultToolkit().sync(); System.gc();
                label.addMouseListener(new MouseAdapter() {
                    public void mousePressed(MouseEvent event) {
                        try {
                            require(event.getSource() == label, "label mouse source");
                            label.setText("clicked"); Toolkit.getDefaultToolkit().sync(); System.gc();
                            if (disposeInCallback) frame.dispose();
                            clicks++;
                        } catch (Throwable failure) { callbackFailure = failure; }
                    }
                });
                int before = clicks; inject(label);
                for (int wait = 0; wait < 200 && clicks == before && callbackFailure == null; wait++) Thread.sleep(10);
                if (callbackFailure != null) throw new Error("label callback: " + callbackFailure);
                require(clicks == before + 1, "label mouse delivery");
                if (disposeInCallback) require(label.getPeer() == null, "label callback disposal");
                frame.dispose(); Toolkit.getDefaultToolkit().sync(); System.gc();
                require(label.getPeer() == null, "label peer retained");
                if (args.length != 0) drained();
            }
            System.out.println("DR2 labels preserve text, alignment, colors and noneditable sizing");
            System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
