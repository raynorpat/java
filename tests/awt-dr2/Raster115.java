import java.awt.Color;
import java.awt.Frame;
import java.awt.Graphics;
import java.awt.Toolkit;

public class Raster115 extends Frame {
    static { System.loadLibrary("raster115"); }
    static native int pixel(Frame frame, int x, int y);
    volatile int paints;
    public void paint(Graphics graphics) {
        graphics.setColor(Color.red);
        graphics.fillRect(20, 40, 60, 40);
        graphics.clearRect(25, 45, 5, 5);
        Graphics copy = graphics.create();
        copy.translate(80, 0);
        copy.setClip(20, 40, 20, 20);
        copy.setColor(Color.blue);
        copy.fillRect(0, 0, 80, 100);
        copy.dispose();
        graphics.fillRect(20, 90, 20, 10);
        paints++;
    }
    static void color(Frame frame, int x, int y, int expected) {
        int actual = pixel(frame, x, y);
        if (actual != expected) throw new Error("pixel " + x + "," + y + ": " + actual + " expected " + expected);
    }
    public static void main(String[] args) {
        try {
            for (int cycle = 0; cycle < 3; cycle++) {
                Raster115 frame = new Raster115();
                frame.setBackground(Color.green);
                frame.setBounds(350, 100, 260, 180);
                frame.show();
                for (int wait = 0; wait < 200 && frame.paints == 0; wait++) Thread.sleep(10);
                if (frame.paints == 0) throw new Error("Java paint did not complete");
                Toolkit.getDefaultToolkit().sync();
                color(frame, 35, 55, 0xff0000);
                color(frame, 27, 47, 0x00ff00);
                color(frame, 105, 45, 0x0000ff);
                color(frame, 125, 45, 0x00ff00);
                color(frame, 25, 95, 0xff0000);
                frame.dispose();
                System.gc();
            }
            System.out.println("DR2 Java painting preserves fills, clears, clipping and cloned origins");
            System.exit(0);
        } catch (Throwable failure) {
            failure.printStackTrace();
            System.exit(1);
        }
    }
}
