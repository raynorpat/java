import java.awt.EventQueue;
import java.awt.Frame;
import java.awt.Toolkit;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;

public class SyntheticInput115 extends Frame implements KeyListener, MouseListener, MouseMotionListener {
    static { System.loadLibrary("syntheticvalues115"); }
    static native void activate(Frame frame);
    volatile String keys = "", mouse = "";
    volatile Throwable failure;
    SyntheticInput115() { addKeyListener(this); addMouseListener(this); addMouseMotionListener(this); }
    void key(KeyEvent event, String mark) {
        if (event.getSource() != this || event.getKeyChar() != 'a' || event.getModifiers() != 2 || event.getWhen() == 123)
            failure = new Error("synthetic key values: " + event);
        Toolkit.getDefaultToolkit().getScreenSize(); System.gc(); keys += mark;
    }
    public void keyPressed(KeyEvent e) { key(e, "P"); }
    public void keyTyped(KeyEvent e) { key(e, "T"); }
    public void keyReleased(KeyEvent e) { key(e, "R"); }
    void mouse(MouseEvent event, String mark) {
        if (event.getSource() != this || event.getX() != 30 || event.getY() != 50 || event.getModifiers() != 1 || event.getWhen() == 123)
            failure = new Error("synthetic mouse values: " + event);
        Toolkit.getDefaultToolkit().getScreenSize(); System.gc(); mouse += mark;
    }
    public void mousePressed(MouseEvent e) { mouse(e, "P"); }
    public void mouseDragged(MouseEvent e) { mouse(e, "D"); }
    public void mouseReleased(MouseEvent e) { mouse(e, "R"); }
    public void mouseClicked(MouseEvent e) { mouse(e, "C"); }
    public void mouseMoved(MouseEvent e) { mouse(e, "M"); }
    public void mouseEntered(MouseEvent e) { }
    public void mouseExited(MouseEvent e) { }
    public static void main(String[] args) {
        try {
            SyntheticInput115 frame = new SyntheticInput115();
            frame.setBounds(350, 100, 300, 180); frame.show(); activate(frame);
            EventQueue queue = Toolkit.getDefaultToolkit().getSystemEventQueue();
            queue.postEvent(new KeyEvent(frame, 401, 123, 2, 65, 'a'));
            queue.postEvent(new KeyEvent(frame, 402, 123, 2, 65, 'a'));
            for (int wait = 0; wait < 200 && frame.keys.length() < 3; wait++) Thread.sleep(10);
            if (!frame.keys.equals("PTR")) throw new Error("synthetic key order: " + frame.keys);
            int[] ids = {501, 506, 502};
            for (int i = 0; i < ids.length; i++) queue.postEvent(new MouseEvent(frame, ids[i], 123, 1, 30, 50, 1, false));
            for (int wait = 0; wait < 200 && frame.mouse.length() < 4; wait++) Thread.sleep(10);
            if (!frame.mouse.equals("PDRC")) throw new Error("synthetic mouse order: " + frame.mouse);
            queue.postEvent(new KeyEvent(frame, 400, 123, 2, 0, 'a'));
            queue.postEvent(new MouseEvent(frame, 500, 123, 1, 30, 50, 1, false));
            queue.postEvent(new MouseEvent(frame, 503, 123, 1, 30, 50, 0, false));
            Thread.sleep(200);
            if (!frame.keys.equals("PTR") || !frame.mouse.equals("PDRC")) throw new Error("omitted synthetic events delivered");
            if (frame.failure != null) throw new Error(frame.failure.toString());
            frame.dispose();
            System.out.println("DR2 original EventQueue posts native synthetic key and mouse input");
            System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
