import java.awt.*;

public class Primitives115 {
    static { System.loadLibrary("primitives115"); }
    static native int checksum(Frame frame);
    static native void track();
    static native int live();
    static void invalidPolygons(Graphics g) {
        int[] points = {10, 20, 30};
        try { g.drawPolygon(points, points, 4); throw new Error("Missing polygon bounds check"); }
        catch (IllegalArgumentException expected) { }
        try { g.fillPolygon(points, points, -1); throw new Error("Missing polygon count check"); }
        catch (ArrayIndexOutOfBoundsException expected) { }
        try { g.drawPolyline(null, points, 1); throw new Error("Missing polygon null check"); }
        catch (NullPointerException expected) { }
        int[] large = new int[1024];
        for (int i = 0; i < large.length; i++) large[i] = 10 + i % 50;
        g.drawPolygon(large, large, large.length);
    }
    static void draw(Graphics g, int kind) {
        g.setColor(Color.white);
        g.fillRect(0, 0, 220, 150);
        g.setColor(Color.black);
        switch (kind) {
        case 0:
            g.drawLine(10, 30, 80, 30); g.drawLine(10, 35, 60, 80);
            g.drawLine(90, 80, 90, 35); g.drawLine(100, 40, 100, 40);
            break;
        case 1:
            g.drawRoundRect(10, 30, 60, 40, 16, 12);
            g.fillRoundRect(80, 30, 60, 40, 16, 12);
            g.drawRoundRect(10, 80, 40, 30, 0, 0);
            g.fillRoundRect(60, 80, 40, 30, 80, 60);
            break;
        case 2:
            g.drawOval(10, 30, 60, 40); g.fillOval(80, 30, 60, 40);
            g.drawOval(10, 80, 0, 10); g.fillOval(30, 80, 1, 1);
            break;
        case 3:
            g.drawArc(10, 30, 60, 40, 30, 220);
            g.fillArc(80, 30, 60, 40, -30, -220);
            g.drawArc(10, 80, 30, 20, 0, 0);
            g.fillArc(60, 80, 40, 30, 400, 720);
            break;
        case 4:
            int[] xs = {10, 30, 60, 40}, ys = {30, 70, 30, 50};
            g.drawPolyline(xs, ys, 4);
            g.translate(70, 0); g.drawPolygon(xs, ys, 4);
            g.translate(-70, 50); g.fillPolygon(xs, ys, 4);
            break;
        case 5:
            g.setFont(new Font("Serif", Font.BOLD | Font.ITALIC, 17));
            g.drawString("ABC caf\u00e9 \u03a9", 10, 50);
            g.setFont(new Font("Monospaced", Font.PLAIN, 12));
            g.setColor(Color.blue); g.drawString("second line", 10, 80);
            break;
        case 6:
            g.setColor(Color.red); g.fillRect(10, 30, 30, 20);
            g.copyArea(10, 30, 30, 20, 40, 10);
            g.translate(20, 20); g.copyArea(-10, 10, 30, 20, 80, 30);
            break;
        case 7:
            g.setColor(Color.red); g.fillRect(10, 30, 120, 70);
            g.setXORMode(Color.blue); g.fillRect(20, 40, 40, 30);
            g.fillRect(20, 40, 40, 30);
            g.fillRect(80, 40, 40, 30);
            g.setPaintMode(); g.setColor(Color.green); g.fillRect(20, 80, 40, 10);
            break;
        case 8:
            Graphics child = g.create();
            child.translate(15, 20); child.clipRect(10, 10, 100, 60);
            child.setFont(new Font("Dialog", Font.PLAIN, 12));
            child.drawString("clipped ABC", 0, 30);
            child.drawLine(0, 0, 130, 100);
            child.fillOval(0, 0, 130, 100);
            child.dispose();
            g.setColor(Color.red); g.drawString("parent", 10, 110);
            break;
        }
    }
    public static void main(String[] args) {
        try {
            Toolkit.getDefaultToolkit();
            if (args.length != 0) track();
            for (int cycle = 0; cycle < 3; cycle++) {
                Frame frame = new Frame();
                frame.setBounds(350, 100, 220, 150);
                frame.show(); Thread.sleep(150);
                for (int kind = 0; kind < 9; kind++) {
                    Graphics g = frame.getGraphics();
                    if (g == null) throw new Error("No graphics");
                    if (args.length != 0) invalidPolygons(g);
                    draw(g, kind); g.dispose();
                    Toolkit.getDefaultToolkit().sync();
                    System.out.println(cycle + ":" + kind + ":" + checksum(frame));
                    System.gc();
                }
                frame.dispose();
                if (args.length != 0) {
                    for (int wait = 0; wait < 100 && live() != 0; wait++) { System.gc(); Thread.sleep(10); }
                    if (live() != 0) throw new Error("Native graphics retained: " + live());
                }
            }
            System.out.println("DR2 native primitives preserve raster output");
            System.exit(0);
        } catch (Throwable problem) { problem.printStackTrace(); System.exit(1); }
    }
}
