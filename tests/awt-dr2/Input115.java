import java.awt.Frame;
import java.awt.Toolkit;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.awt.event.FocusEvent;
import java.awt.event.FocusListener;

public class Input115 extends Frame implements MouseListener, MouseMotionListener, FocusListener {
    static { System.loadLibrary("input115"); }
    static native void inject(Frame frame);
    static native void focus(Frame frame, boolean temporary);
    volatile String focusEvents = "";
    volatile boolean checkingFocus, temporaryFocus;
    volatile int pressed, dragged, released, clicked;
    volatile boolean consumePress, consumeRelease, disposeOnPress;
    volatile Throwable failure;
    Input115() { addMouseListener(this); addMouseMotionListener(this); addFocusListener(this); }
    void inspectFocus(FocusEvent event, String value) {
        if (!checkingFocus) return;
        if (event.getSource() != this || event.isTemporary() != temporaryFocus) failure = new Error("native focus values: " + event);
        Toolkit.getDefaultToolkit().getScreenSize();
        System.gc();
        focusEvents += value;
    }
    public void focusGained(FocusEvent event) { inspectFocus(event, "G"); }
    public void focusLost(FocusEvent event) { inspectFocus(event, "L"); }
    void inspect(MouseEvent event) {
        if (event.getSource() != this || event.getX() != 30 || event.getY() != 50 || event.getModifiers() != 3)
            failure = new Error("native input values: " + event);
        if (Toolkit.getDefaultToolkit().getScreenSize().width <= 0) failure = new Error("input callback reentry failed");
        System.gc();
    }
    public void mousePressed(MouseEvent event) {
        inspect(event);
        if (consumePress) event.consume();
        if (disposeOnPress) { dispose(); Toolkit.getDefaultToolkit().sync(); System.gc(); }
        pressed++;
    }
    public void mouseDragged(MouseEvent event) { inspect(event); dragged++; }
    public void mouseReleased(MouseEvent event) { inspect(event); if (consumeRelease) event.consume(); released++; }
    public void mouseClicked(MouseEvent event) { inspect(event); clicked++; }
    public void mouseEntered(MouseEvent event) { }
    public void mouseExited(MouseEvent event) { }
    public void mouseMoved(MouseEvent event) { }
    public static void main(String[] args) {
        try {
            Input115 frame = new Input115();
            frame.setBounds(350, 100, 300, 180);
            frame.show();
            Thread.sleep(100);
            frame.focusEvents = "";
            frame.checkingFocus = true;
            focus(frame, false);
            for (int wait = 0; wait < 200 && frame.focusEvents.length() < 2; wait++) Thread.sleep(10);
            if (!frame.focusEvents.equals("LG")) throw new Error("native focus sequence: " + frame.focusEvents);
            frame.focusEvents = "";
            frame.temporaryFocus = true;
            focus(frame, true);
            for (int wait = 0; wait < 200 && frame.focusEvents.length() < 2; wait++) Thread.sleep(10);
            if (!frame.focusEvents.equals("LG")) throw new Error("native temporary focus sequence: " + frame.focusEvents);
            frame.checkingFocus = false;
            frame.consumePress = true;
            inject(frame);
            for (int wait = 0; wait < 200 && frame.clicked != 1; wait++) Thread.sleep(10);
            if (frame.failure != null) throw new Error(frame.failure.toString());
            if (frame.pressed != 1 || frame.dragged != 1 || frame.released != 1 || frame.clicked != 1)
                throw new Error("native sequence lost: " + frame.pressed + "/" + frame.dragged + "/" + frame.released + "/" + frame.clicked);
            frame.consumeRelease = true;
            inject(frame);
            for (int wait = 0; wait < 200 && frame.released != 2; wait++) Thread.sleep(10);
            Thread.sleep(100);
            if (frame.failure != null) throw new Error(frame.failure.toString());
            if (frame.pressed != 2 || frame.dragged != 2 || frame.released != 2 || frame.clicked != 1)
                throw new Error("consumed release generated a click or lost native tracking");
            if (args.length != 0) {
                frame.disposeOnPress = true;
                inject(frame);
                for (int wait = 0; wait < 200 && frame.pressed != 3; wait++) Thread.sleep(10);
                Thread.sleep(100);
                if (frame.pressed != 3 || frame.getPeer() != null || frame.released != 2 || frame.clicked != 1)
                    throw new Error("mouse callback disposal: pressed=" + frame.pressed + " released=" + frame.released + " clicked=" + frame.clicked + " peer=" + (frame.getPeer() != null));
            } else frame.dispose();
            System.out.println("DR2 native mouse input preserves drag tracking, consumption and clicks");
            System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
