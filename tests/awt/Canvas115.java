import java.awt.*;
import java.awt.event.*;

public class Canvas115 {
    static class FocusObserver extends FocusAdapter {
        volatile int gained, lost;
        public void focusGained(FocusEvent e) { System.gc(); gained++; }
        public void focusLost(FocusEvent e) { lost++; }
    }
    static class PaintedCanvas extends Canvas {
        volatile int paints;
        public void paint(Graphics g) { g.setColor(Color.green); g.fillRect(0,0,20,20); paints++; }
    }
    public static void main(String[] args) {
        try {
            for (int cycle = 0; cycle < 5; cycle++) {
                Frame f = new Frame("Canvas115");
                f.setLayout(null); f.setBounds(80,90,280,220);
                Panel p = new Panel(null); p.setBounds(20,50,180,120); f.add(p);
                PaintedCanvas c = new PaintedCanvas(); c.setBounds(10,15,100,70); p.add(c);
                Canvas other = new Canvas(); other.setBounds(115,15,50,70); p.add(other);
                FocusObserver focus = new FocusObserver(); c.addFocusListener(focus);
                f.show(); c.repaint();
                for (int i = 0; c.paints == 0 && i < 100; i++) Thread.sleep(10);
                if (c.paints == 0) throw new RuntimeException("missing canvas paint");
                Point point = c.getLocationOnScreen();
                if (point.x != 110 || point.y != 155) throw new RuntimeException("child origin " + point);
                c.setBounds(15,20,90,60);
                point = c.getLocationOnScreen();
                for (int i = 0; (point.x != 115 || point.y != 160) && i < 100; i++) {
                    Thread.sleep(10); point = c.getLocationOnScreen();
                }
                if (point.x != 115 || point.y != 160) throw new RuntimeException("child reshape " + point);
                c.requestFocus();
                for (int i = 0; focus.gained == 0 && i < 100; i++) Thread.sleep(10);
                if (focus.gained == 0) throw new RuntimeException("missing focus gained");
                other.requestFocus();
                for (int i = 0; focus.lost == 0 && i < 100; i++) Thread.sleep(10);
                if (focus.lost == 0) throw new RuntimeException("missing focus lost");
                Graphics g = c.getGraphics();
                c.setEnabled(false); c.hide(); c.show(); c.setEnabled(true);
                f.dispose(); System.gc(); g.fillRect(0,0,20,20); g.dispose();
                if (c.getPeer() != null || p.getPeer() != null) throw new RuntimeException("child retirement");
            }
            System.out.println("nested panels, canvas geometry, painting and retirement passed"); System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
