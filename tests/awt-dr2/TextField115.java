import java.awt.*;
import java.awt.event.*;

public class TextField115 {
    static { System.loadLibrary("textfield115"); }
    static native void inject(TextField field, int character);
    static native boolean state(TextField field, boolean editable);
    static native void track();
    static native int liveFields();
    static native int liveEditors();
    static native void failConstruction(boolean fail);
    static volatile int actions, changes;
    static volatile Throwable callbackFailure;
    static final RuntimeException marker = new RuntimeException("text field font failure");
    static class BadFont extends Font {
        BadFont() { super("Dialog", Font.PLAIN, 12); }
        public int getSize() { throw marker; }
    }
    static void drained() throws Exception {
        for (int wait = 0; wait < 40 && (liveFields() != 0 || liveEditors() != 0); wait++) {
            Thread.sleep(50); Toolkit.getDefaultToolkit().sync(); System.gc();
        }
        require(liveFields() == 0 && liveEditors() == 0, "native text resources retained: " + liveFields() + "/" + liveEditors());
    }
    static void failedConstruction(boolean nativeFailure) throws Exception {
        TextField field = new TextField("failure");
        if (!nativeFailure) field.setFont(new BadFont());
        failConstruction(nativeFailure);
        try { field.addNotify(); throw new Error("text field construction succeeded"); }
        catch (RuntimeException failure) {
            if (!nativeFailure) require(failure == marker, "text field throwable identity");
            else require(failure.getMessage().indexOf("text field construction failure") >= 0, "native constructor exception");
        } finally { failConstruction(false); }
        require(field.getPeer() == null, "failed text field peer"); drained();
    }
    static void require(boolean value, String message) { if (!value) throw new Error(message); }
    public static void main(String[] args) {
        try {
            final boolean tracked = args.length != 0;
            if (tracked) {
                Toolkit.getDefaultToolkit(); track();
                for (int pass = 0; pass < 3; pass++) { failedConstruction(false); failedConstruction(true); }
            }
            for (int pass = 0; pass < 3; pass++) {
                final Frame frame = new Frame(); frame.setLayout(null);
                frame.setBounds(350, 100, 300, 180);
                final TextField field = new TextField("initial", 8);
                field.setBounds(20, 40, 240, 40);
                field.setFont(new Font("Dialog", Font.PLAIN, 12));
                frame.add(field); frame.show();
                require(field.getText().equals("initial"), "initial text");
                require(state(field, true), "native initial state");
                field.setText("Caf\u00e9");
                require(field.getText().equals("Caf\u00e9"), "Unicode text");
                Dimension small = field.getPreferredSize(2);
                Dimension large = field.getPreferredSize(12);
                require(large.width > small.width && large.height == small.height, "column sizing");
                field.setEditable(false);
                require(!field.isEditable(), "noneditable state");
                require(state(field, false), "native noneditable state");
                field.setEditable(true);
                field.setEchoChar('*');
                require(field.getText().equals("Caf\u00e9"), "echo preserves text");
                field.setEchoChar((char)0);
                field.requestFocus(); Toolkit.getDefaultToolkit().sync();
                Thread.sleep(100);
                field.select(1, 3);
                require(field.getSelectionStart() == 1 && field.getSelectionEnd() == 3, "selection");
                require(field.getSelectedText().equals("af"), "selected text");
                field.setCaretPosition(2);
                require(field.getCaretPosition() == 2 && field.getSelectionEnd() == 2, "caret");
                field.setText(null); require(field.getText().equals(""), "null text");
                field.addTextListener(new TextListener() {
                    public void textValueChanged(TextEvent event) {
                        try {
                            require(event.getSource() == field && event.getID() == TextEvent.TEXT_VALUE_CHANGED, "text event");
                            Toolkit.getDefaultToolkit().sync(); System.gc(); changes++;
                        } catch (Throwable failure) { callbackFailure = failure; }
                    }
                });
                field.addActionListener(new ActionListener() {
                    public void actionPerformed(ActionEvent event) {
                        try {
                            require(event.getSource() == field && event.getActionCommand().equals("x"), "action event");
                            Toolkit.getDefaultToolkit().sync(); System.gc();
                            if (tracked) frame.dispose();
                            actions++;
                        } catch (Throwable failure) { callbackFailure = failure; }
                    }
                });
                field.requestFocus(); field.selectAll();
                int before = changes; inject(field, 'x');
                for (int wait = 0; wait < 200 && changes == before && callbackFailure == null; wait++) Thread.sleep(10);
                require(changes > before && field.getText().equals("x"), "native editing/text callback");
                final TextField second = new TextField("second");
                second.setBounds(20, 90, 240, 40); frame.add(second);
                second.addTextListener(new TextListener() {
                    public void textValueChanged(TextEvent event) {
                        try {
                            require(event.getSource() == second, "shared editor text source");
                            Toolkit.getDefaultToolkit().sync(); System.gc(); changes++;
                        } catch (Throwable failure) { callbackFailure = failure; }
                    }
                });
                second.requestFocus(); second.selectAll();
                before = changes; inject(second, 'y');
                for (int wait = 0; wait < 200 && changes == before && callbackFailure == null; wait++) Thread.sleep(10);
                require(changes > before && second.getText().equals("y") && field.getText().equals("x"), "shared editor switch");
                before = changes; second.setText("programmatic");
                for (int wait = 0; wait < 200 && changes == before && callbackFailure == null; wait++) Thread.sleep(10);
                require(changes > before && second.getText().equals("programmatic"), "programmatic text callback");
                if (tracked) require(liveEditors() == 1, "one shared editor per window");
                second.setEditable(false);
                require(state(second, false), "active editor made noneditable");
                field.requestFocus(); field.setCaretPosition(1);
                before = actions; inject(field, 13);
                for (int wait = 0; wait < 200 && actions == before && callbackFailure == null; wait++) Thread.sleep(10);
                if (callbackFailure != null) throw new Error(callbackFailure.toString());
                require(actions == before + 1, "return action callback");
                frame.dispose(); Toolkit.getDefaultToolkit().sync(); System.gc();
                require(field.getPeer() == null, "disposed text peer");
                if (tracked) drained();
            }
            System.out.println("DR2 text fields preserve text, column sizing, selection and caret");
            System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
