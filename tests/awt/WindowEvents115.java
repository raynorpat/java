import java.awt.*;
import java.awt.event.*;

public class WindowEvents115 {
    static volatile int closed, resized, moved;
    public static void main(String[] args) {
        try {
            Frame f = new Frame("WindowEvents115");
            f.addWindowListener(new WindowAdapter() {
                public void windowClosing(WindowEvent e) { System.gc(); closed++; }
            });
            f.addComponentListener(new ComponentAdapter() {
                public void componentResized(ComponentEvent e) { System.gc(); resized++; }
                public void componentMoved(ComponentEvent e) { moved++; }
            });
            f.setBounds(50,60,240,180); f.show();
            Thread.sleep(100); resized = 0; moved = 0;
            WinAwtProbe.move("WindowEvents115",90,100,280,200);
            for (int i = 0; (resized == 0 || moved == 0) && i < 100; i++) Thread.sleep(10);
            if (resized == 0 || moved == 0) throw new RuntimeException("missing native geometry events");
            Rectangle r = f.getBounds();
            if (r.x != 90 || r.y != 100 || r.width != 280 || r.height != 200)
                throw new RuntimeException("native geometry " + r);
            WinAwtProbe.close("WindowEvents115");
            for (int i = 0; closed == 0 && i < 100; i++) Thread.sleep(10);
            if (closed != 1) throw new RuntimeException("missing window closing event");
            if (f.getPeer() == null || !f.isShowing()) throw new RuntimeException("close disposed window");
            f.dispose();
            System.out.println("native move, resize and close events passed"); System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
