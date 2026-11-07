import java.awt.*;

public class Graphics115 {
    static void rectangle(Rectangle r, int x, int y, int w, int h) {
        if (r == null || r.x != x || r.y != y || r.width != w || r.height != h)
            throw new RuntimeException("clip " + r);
    }
    public static void main(String[] args) {
        try {
            Frame frame = new Frame("Graphics contracts");
            frame.setBounds(60,70,240,160); frame.show();
            Graphics g = frame.getGraphics();
            if (g.getClipBounds() != null) throw new RuntimeException("initial clip");
            g.setClip(20,40,100,80);
            g.translate(5,7);
            rectangle(g.getClipBounds(),15,33,100,80);
            Graphics copy = g.create();
            rectangle(copy.getClipBounds(),15,33,100,80);
            copy.clipRect(20,40,20,30);
            rectangle(copy.getClipBounds(),20,40,20,30);
            rectangle(g.getClipBounds(),15,33,100,80);
            copy.setClip(null);
            if (copy.getClipBounds() != null) throw new RuntimeException("remove clip");
            g.setColor(Color.blue); g.fillRect(20,40,30,30);
            g.clearRect(25,45,10,10);
            g.setClip(0,0,-1,5);
            if (!g.getClipBounds().isEmpty()) throw new RuntimeException("negative clip");
            frame.dispose(); System.gc();
            g.fillRect(0,0,10,10); copy.clearRect(0,0,10,10);
            g.dispose(); g.dispose(); copy.dispose(); copy.dispose();
            System.out.println("component graphics, clipping, copies and retirement passed");
            System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
