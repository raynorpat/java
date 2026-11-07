import java.awt.*;
import java.awt.event.*;

public class Window115 {
    static volatile int paints, clicks;
    static String callbackThread;
    static class Surface extends Canvas {
        public void paint(Graphics graphics) {
            graphics.setColor(Color.red);
            graphics.fillRect(0,0,getSize().width,getSize().height);
            paints++;
        }
    }
    static native void click(Component component);
    static native int pixel(Canvas canvas, int x, int y);
    public static void main(String[] args) throws Exception {
        System.getProperties().put("awt.toolkit", "sun.awt.motif.MToolkit");
        Toolkit.getDefaultToolkit();
        System.loadLibrary("motif118xprobe");
        for (int cycle=0; cycle<12; cycle++) {
            final Frame frame = new Frame("Motif reconstruction test");
            final boolean disposeInCallback = (cycle & 1) != 0;
            frame.setLayout(null);
            final Button button = new Button("Click");
            Surface canvas = new Surface();
            button.setBounds(15,15,100,30);
            canvas.setBounds(15,60,100,60);
            frame.add(button); frame.add(canvas);
            button.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent event) {
                    callbackThread=Thread.currentThread().getName();
                    if (disposeInCallback) frame.dispose();
                    clicks++;
                }
            });
            frame.setSize(180,160);
            int oldPaints=paints, oldClicks=clicks;
            frame.show();
            canvas.repaint();
            long deadline=System.currentTimeMillis()+3000;
            while (paints==oldPaints && System.currentTimeMillis()<deadline) Thread.sleep(10);
            if (paints==oldPaints) throw new RuntimeException("missing paint");
            Toolkit.getDefaultToolkit().sync();
            if ((pixel(canvas,5,5)&0xffffff)!=0xff0000) throw new RuntimeException("paint pixel");
            Graphics retained = canvas.getGraphics();
            Graphics copy = retained.create();
            click(button);
            deadline=System.currentTimeMillis()+3000;
            while (clicks==oldClicks && System.currentTimeMillis()<deadline) Thread.sleep(10);
            if (clicks!=oldClicks+1) throw new RuntimeException("button activation");
            if (callbackThread.equals("main") || callbackThread.equals("AWT-Motif"))
                throw new RuntimeException("callback bypassed Java event dispatch");
            if (!disposeInCallback) { frame.hide(); frame.show(); }
            frame.dispose();
            retained.fillRect(0,0,10,10);
            copy.clearRect(0,0,10,10);
            retained.dispose(); copy.dispose();
            System.gc();
        }
        System.out.println("window paint, click, event dispatch and disposal passed");
        System.exit(0);
    }
}
