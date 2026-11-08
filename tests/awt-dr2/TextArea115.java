import java.awt.*;
import java.awt.event.*;

public class TextArea115 {
    static { System.loadLibrary("textarea115"); }
    static native void inject(TextArea area, int character);
    static native boolean state(TextArea area, boolean editable, int mode);
    static native void track();
    static native int liveFields();
    static native int liveEditors();
    static native void failConstruction(boolean fail);
    static volatile int changes;
    static volatile boolean disposeOnChange;
    static volatile Throwable callbackFailure;
    static final RuntimeException marker = new RuntimeException("text area font failure");
    static class BadFont extends Font {
        BadFont() { super("Dialog", Font.PLAIN, 12); }
        public int getSize() { throw marker; }
    }
    static void drained() throws Exception {
        for (int wait = 0; wait < 40 && (liveFields() != 0 || liveEditors() != 0); wait++) {
            Thread.sleep(50); Toolkit.getDefaultToolkit().sync(); System.gc();
        }
        require(liveFields() == 0 && liveEditors() == 0, "native text areas retained: " + liveFields() + "/" + liveEditors());
    }
    static void failedConstruction(boolean nativeFailure, int mode) throws Exception {
        TextArea area = new TextArea("failure", 2, 8, mode);
        if (!nativeFailure) area.setFont(new BadFont());
        failConstruction(nativeFailure);
        try { area.addNotify(); throw new Error("text area construction succeeded"); }
        catch (RuntimeException failure) {
            if (!nativeFailure) require(failure == marker, "text area throwable identity");
            else require(failure.getMessage().indexOf("text area construction failure") >= 0, "native constructor exception");
        } finally { failConstruction(false); }
        require(area.getPeer() == null, "failed text area peer"); drained();
    }
    static void require(boolean value, String message) { if (!value) throw new Error(message); }
    public static void main(String[] args) {
        try {
            boolean tracked = args.length != 0;
            if (tracked) {
                Toolkit.getDefaultToolkit(); track();
                for (int mode = 0; mode < 4; mode++) { failedConstruction(false, mode); failedConstruction(true, mode); }
            }
            for (int mode = 0; mode < 4; mode++) {
                final Frame frame = new Frame(); frame.setLayout(null); frame.setBounds(350, 100, 300, 230);
                final TextArea area = new TextArea("first\nsecond", 4, 12, mode);
                disposeOnChange = false;
                area.setBounds(20, 40, 240, 150); area.setFont(new Font("Dialog", Font.PLAIN, 12));
                area.select(1, 4); frame.add(area); frame.show();
                require(area.getText().equals("first\nsecond"), "initial multiline text");
                require(state(area, true, mode), "native scrollbar configuration");
                require(area.getSelectionStart() == 1 && area.getSelectionEnd() == 4, "initial selection");
                area.setText("Caf\u00e9\nnext"); require(area.getText().equals("Caf\u00e9\nnext"), "Unicode text");
                area.insert("!", 4); require(area.getText().equals("Caf\u00e9!\nnext"), "insert text");
                area.replaceRange("tea", 0, 4); require(area.getText().equals("tea!\nnext"), "replace text");
                area.append("\nlast"); require(area.getText().equals("tea!\nnext\nlast"), "append text");
                area.select(1, 3); require(area.getSelectedText().equals("ea"), "selection");
                area.setCaretPosition(2); require(area.getCaretPosition() == 2, "caret");
                Dimension small = area.getPreferredSize(2, 4), large = area.getPreferredSize(5, 14);
                require(large.width > small.width && large.height > small.height, "row/column sizing");
                System.out.println(mode + ":" + small.width + "x" + small.height + ":" + large.width + "x" + large.height);
                area.setEditable(false); require(state(area, false, mode), "native noneditable state");
                area.setEnabled(false); area.setEditable(true); require(state(area, false, mode), "disabled editing");
                area.setEnabled(true); require(state(area, true, mode), "enabled editing");
                try { area.setText(null); throw new Error("null text accepted"); }
                catch (RuntimeException expected) { require(expected.getMessage().indexOf("nil NSString") >= 0, "null text exception"); }
                area.setText(""); require(area.getText().equals(""), "empty text");
                area.getPreferredSize(2, 4); require(area.getText().equals(""), "empty preferred-size text preservation");
                area.addTextListener(new TextListener() {
                    public void textValueChanged(TextEvent event) {
                        try {
                            require(event.getSource() == area && event.getID() == TextEvent.TEXT_VALUE_CHANGED, "text area callback source");
                            Toolkit.getDefaultToolkit().sync(); System.gc();
                            if (disposeOnChange) frame.dispose();
                            changes++;
                        } catch (Throwable failure) { callbackFailure = failure; }
                    }
                });
                area.requestFocus(); int before = changes; inject(area, 'x');
                for (int wait = 0; wait < 200 && changes == before && callbackFailure == null; wait++) Thread.sleep(10);
                if (callbackFailure != null) throw new Error(callbackFailure.toString());
                require(changes > before && area.getText().equals("x"), "native text area typing");
                before = changes; inject(area, 13);
                for (int wait = 0; wait < 200 && changes == before && callbackFailure == null; wait++) Thread.sleep(10);
                require(changes > before && area.getText().equals("x\n"), "native multiline typing");
                if (tracked) {
                    before = changes; disposeOnChange = true; inject(area, 'z');
                    for (int wait = 0; wait < 200 && changes == before && callbackFailure == null; wait++) Thread.sleep(10);
                    if (callbackFailure != null) throw new Error(callbackFailure.toString());
                    require(changes > before && area.getPeer() == null, "text area callback disposal");
                }
                frame.dispose(); Toolkit.getDefaultToolkit().sync(); System.gc();
                require(area.getPeer() == null, "disposed text area");
                if (tracked) drained();
            }
            System.out.println("DR2 text areas preserve multiline editing, selection and scrollbar sizing");
            System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
