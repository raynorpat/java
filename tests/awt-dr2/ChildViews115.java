import java.awt.Canvas;
import java.awt.Color;
import java.awt.Component;
import java.awt.Frame;
import java.awt.Font;
import java.awt.Graphics;
import java.awt.Panel;
import java.awt.Toolkit;

public class ChildViews115 {
    static { System.loadLibrary("childviews115"); }
    static native boolean check(Component parent, Component child, int x, int y, int w, int h, boolean visible, boolean enabled);
    static native void track();
    static native int liveViews();
    static native void failConstruction(boolean fail);
    static final RuntimeException marker = new RuntimeException("child font failure");
    static class BadFont extends Font {
        BadFont() { super("Dialog", Font.PLAIN, 12); }
        public int getSize() { throw marker; }
    }
    static void failedConstruction(Component child, boolean nativeFailure) throws Exception {
        if (!nativeFailure) child.setFont(new BadFont());
        failConstruction(nativeFailure);
        try {
            child.addNotify();
            throw new Error("child construction unexpectedly succeeded");
        } catch (RuntimeException failure) {
            if (!nativeFailure && failure != marker) throw new Error("child exception identity changed");
            if (nativeFailure && failure.getMessage().indexOf("child construction failure") < 0)
                throw new Error("native child exception changed: " + failure);
        } finally { failConstruction(false); }
        for (int retry = 0; retry < 40 && liveViews() != 0; retry++) {
            Thread.sleep(50); Toolkit.getDefaultToolkit().sync(); System.gc();
        }
        if (liveViews() != 0 || child.getPeer() != null) throw new Error("failed child retained native view/peer");
    }
    static class Painted extends Canvas {
        volatile int paints;
        public void paint(Graphics g) { g.setColor(Color.red); g.fillRect(0, 0, 20, 20); paints++; }
    }
    static void state(Component parent, Component child, int x, int y, int w, int h, boolean visible, boolean enabled) {
        if (!check(parent, child, x, y, w, h, visible, enabled)) throw new Error("native child state differs");
    }
    public static void main(String[] args) {
        try {
            if (args.length != 0) track();
            if (args.length != 0) {
                Toolkit.getDefaultToolkit();
                for (int pass = 0; pass < 3; pass++) {
                    failedConstruction(new Canvas(), false);
                    failedConstruction(new Panel(), false);
                    failedConstruction(new Canvas(), true);
                    failedConstruction(new Panel(), true);
                }
            }
            for (int pass = 0; pass < 3; pass++) {
                Frame frame = new Frame(); frame.setLayout(null); frame.setBounds(350, 100, 300, 200);
                Panel panel = new Panel(null); panel.setBounds(20, 40, 200, 120);
                Painted canvas = new Painted(); canvas.setBounds(10, 15, 80, 60);
                panel.add(canvas); frame.add(panel); frame.show();
                for (int wait = 0; wait < 200 && canvas.paints == 0; wait++) Thread.sleep(10);
                if (canvas.paints == 0) throw new Error("child never painted");
                state(frame, panel, 20, 40, 200, 120, true, true);
                state(panel, canvas, 10, 15, 80, 60, true, true);
                canvas.setBounds(-5, 8, 40, 30); canvas.setEnabled(false);
                state(panel, canvas, -5, 8, 40, 30, true, false);
                canvas.setVisible(false); state(panel, canvas, -5, 8, 40, 30, false, false);
                canvas.setVisible(true); canvas.setEnabled(true);
                state(panel, canvas, -5, 8, 40, 30, true, true);
                panel.remove(canvas);
                if (canvas.getPeer() != null) throw new Error("removed child peer retained");
                panel.add(canvas); panel.validate();
                state(panel, canvas, -5, 8, 40, 30, true, true);
                frame.dispose(); Toolkit.getDefaultToolkit().sync(); System.gc();
                if (canvas.getPeer() != null || panel.getPeer() != null) throw new Error("disposed children retain peers");
                for (int retry = 0; args.length != 0 && retry < 40 && liveViews() != 0; retry++) {
                    Thread.sleep(50); Toolkit.getDefaultToolkit().sync(); System.gc();
                }
                if (args.length != 0 && liveViews() != 0) throw new Error("native child views leaked: " + liveViews());
            }
            System.out.println("DR2 child views preserve attachment, bounds, enabled state and painting");
            System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
