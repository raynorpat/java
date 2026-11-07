import java.awt.*;
import java.awt.event.*;

public class Lifecycle115 {
    static class PaintedFrame extends Frame {
        volatile int paints;
        PaintedFrame() { super("AWT lifecycle"); }
        public void paint(Graphics g) { g.setColor(Color.red); g.fillRect(20,40,15,15); System.gc(); paints++; }
    }
    static class ResizeObserver extends ComponentAdapter {
        volatile int resized;
        public void componentResized(ComponentEvent event) { System.gc(); resized++; }
    }
    static void cycle() throws Exception {
        PaintedFrame frame = new PaintedFrame();
        ResizeObserver observer = new ResizeObserver();
        frame.addComponentListener(observer);
        frame.setBounds(60, 70, 240, 160);
        frame.addNotify();
        frame.show();
        frame.repaint();
        for (int i = 0; frame.paints == 0 && i < 100; i++) Thread.sleep(10);
        if (frame.paints == 0) throw new RuntimeException("missing paint callback");
        if (frame.getPeer() == null || !frame.isShowing()) throw new RuntimeException("visibility");
        Point p = frame.getLocationOnScreen();
        if (p.x != 60 || p.y != 70) throw new RuntimeException("location " + p);
        Insets insets = frame.getInsets();
        if (insets.top <= 0 || insets.left < 0) throw new RuntimeException("insets");
        frame.setTitle("Changed");
        frame.setResizable(false);
        frame.setBounds(80, 90, 260, 180);
        for (int i = 0; observer.resized == 0 && i < 100; i++) Thread.sleep(10);
        if (observer.resized == 0) throw new RuntimeException("missing resize callback");
        for (int i = 0; i < 20; i++) frame.setBounds(80, 90, 260 + i, 180 + i);
        frame.hide();
        System.gc();
        frame.dispose();
        frame.dispose();
        if (frame.getPeer() != null) throw new RuntimeException("dispose");
    }
    public static void main(String[] args) {
        try {
            for (int i = 0; i < 5; i++) cycle();
            Thread worker = new Thread(new Runnable() {
                public void run() {
                    try { cycle(); } catch (Throwable failure) {
                        failure.printStackTrace(); System.exit(1);
                    }
                }
            });
            worker.start(); worker.join();
            System.out.println("window lifecycle, worker creation and GC passed");
            System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
