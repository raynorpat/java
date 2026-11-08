import java.awt.AWTEvent;
import java.awt.EventQueue;
import java.awt.Frame;
import java.awt.Toolkit;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;

public class QueueRead115 extends Frame implements KeyListener {
    static { System.loadLibrary("queueread115"); }
    static native AWTEvent[] exercise(EventQueue queue, Frame frame, int mode);
    volatile int pressed, typed;
    volatile boolean callbackProbe;
    volatile AWTEvent callbackPeek;
    volatile Throwable failure;
    QueueRead115() { addKeyListener(this); }
    public void keyPressed(KeyEvent e) {
        pressed++; Toolkit.getDefaultToolkit().getScreenSize(); System.gc();
        if (callbackProbe) {
            try {
                EventQueue queue = Toolkit.getDefaultToolkit().getSystemEventQueue();
                queue.postEvent(new KeyEvent(this, 402, 123, 0, 65, 'a'));
                AWTEvent peeked = queue.peekEvent(402);
                System.gc();
                callbackPeek = peeked;
            } catch (Throwable error) { failure = error; }
        }
    }
    public void keyTyped(KeyEvent e) { typed++; }
    public void keyReleased(KeyEvent e) { }
    static void callbackProbeStart(QueueRead115 frame, EventQueue queue) throws Exception {
        frame.callbackProbe = true;
        exercise(queue, frame, 2);
        for (int wait = 0; wait < 200 && frame.callbackPeek == null && frame.failure == null; wait++) Thread.sleep(10);
        if (frame.failure != null) throw new Error(frame.failure.toString());
        if (!(frame.callbackPeek instanceof KeyEvent) || frame.callbackPeek.getSource() != frame || frame.callbackPeek.getID() != 402)
            throw new Error("cross-thread peek lost the returned event");
    }
    public static void main(String[] args) {
        try {
            QueueRead115 frame = new QueueRead115(); frame.setBounds(350, 100, 300, 180); frame.show();
            EventQueue queue = Toolkit.getDefaultToolkit().getSystemEventQueue();
            exercise(queue, frame, 1); Thread.sleep(200);
            if (queue.peekEvent(400) != null) throw new Error("unsupported event mask returned an event");
            queue.peekEvent();
            AWTEvent[] events = exercise(queue, frame, 0);
            if (events == null || events.length != 5) throw new Error("queue probe failed");
            for (int i = 0; i < 5; i++) {
                if (i == 2 || i == 3) { if (events[i] != null) throw new Error("peek filter failed"); continue; }
                if (!(events[i] instanceof KeyEvent)) throw new Error("missing key at " + i);
                KeyEvent e = (KeyEvent)events[i];
                if (e.getSource() != frame || e.getID() != 401 || e.getKeyCode() != 65 || e.getKeyChar() != 'a')
                    throw new Error("queue values: " + e);
            }
            if (events[0] == events[1] || events[1] == events[4]) throw new Error("native conversion reused a Java event");
            if (frame.pressed != 1 || frame.typed != 1) throw new Error("dequeue dispatch counts: " + frame.pressed + "/" + frame.typed);
            callbackProbeStart(frame, queue);
            System.gc(); frame.dispose();
            System.out.println("DR2 queue peek filters without dispatch and get converts delivered native input");
            System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
