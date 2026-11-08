import java.awt.Frame;
import java.awt.Toolkit;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;

public class Keyboard115 extends Frame implements KeyListener {
    static { System.loadLibrary("keyboard115"); }
    static native void inject(Frame frame, int kind);
    volatile String events = "";
    volatile int mode;
    volatile Throwable failure;
    Keyboard115() { addKeyListener(this); }
    void inspect(KeyEvent event, char kind) {
        int modifiers = kind == 'T' && mode == 2 ? 2 : 1;
        char character = kind == 'T' && mode == 2 ? 'z' : 'A';
        int code = kind == 'T' ? 0 : 65;
        if (mode == 5) { code = 38; character = 63232; }
        if (mode == 6 && kind == 'T') character = '\n';
        if (mode == 4) {
            modifiers = kind == 'P' ? 8 : 0;
            character = 0;
            code = events.length() == 0 ? 16 : 18;
        }
        if (event.getSource() != this || event.getModifiers() != modifiers ||
            event.getKeyChar() != character || event.getKeyCode() != code)
            failure = new Error("native key values: " + event);
        Toolkit.getDefaultToolkit().getScreenSize();
        System.gc();
        events += kind;
    }
    public void keyPressed(KeyEvent event) {
        inspect(event, 'P');
        if (mode == 1) event.consume();
        if (mode == 2) { event.setKeyChar('z'); event.setKeyCode(90); event.setModifiers(2); }
        if (mode == 6) { event.setKeyChar('\n'); event.setKeyCode(10); }
        if (mode == 3) { dispose(); Toolkit.getDefaultToolkit().sync(); System.gc(); }
    }
    public void keyTyped(KeyEvent event) { inspect(event, 'T'); }
    public void keyReleased(KeyEvent event) { inspect(event, 'R'); if (mode != 4) event.consume(); }
    public static void main(String[] args) {
        try {
            Keyboard115 frame = new Keyboard115();
            frame.setBounds(350, 100, 300, 180);
            frame.show();
            int[] modes = args.length == 0 ? new int[] {0, 1, 2, 5, 6, 4} : new int[] {0, 1, 2, 5, 6, 4, 3};
            for (int index = 0; index < modes.length; index++) {
                int mode = modes[index];
                frame.events = "";
                frame.mode = mode;
                inject(frame, mode);
                String expected = mode == 1 || mode == 5 ? "PR" : mode == 3 ? "P" : mode == 4 ? "RPR" : "PTR";
                for (int wait = 0; wait < 200 && frame.events.length() < expected.length(); wait++) Thread.sleep(10);
                Thread.sleep(100);
                if (frame.failure != null) throw new Error(frame.failure.toString());
                if (!frame.events.equals(expected)) throw new Error("native key sequence mode=" + mode + ": " + frame.events);
                if (mode == 3 && frame.getPeer() != null) throw new Error("key callback disposal failed");
            }
            if (frame.getPeer() != null) frame.dispose();
            System.out.println("DR2 native keys preserve order, consumption and Java edits");
            System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
