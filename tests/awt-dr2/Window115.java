import java.awt.Frame;
import java.awt.Insets;
import java.awt.Point;
import java.awt.Toolkit;

public class Window115 {
    public static void main(String[] args) {
        try { run(); }
        catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
        System.exit(0);
    }
    static void run() throws Exception {
        for (int cycle = 0; cycle < 3; cycle++) {
            Frame frame = new Frame("DR2 reconstructed window " + cycle);
            frame.setBounds(360, 100, 300, 180);
            frame.show();
            frame.toFront();
            Toolkit.getDefaultToolkit().sync();
            if (!frame.isVisible() || frame.getPeer() == null) throw new Error("window not realized");
            Insets insets = frame.getInsets();
            Point location = frame.getLocationOnScreen();
            if (insets.top <= 0 || location.x != 360 || location.y != 100)
                throw new Error("window geometry: " + location + " " + insets);
            frame.setTitle("DR2 changed title " + cycle);
            frame.setResizable(false);
            frame.setBounds(380, 120, 320, 200);
            Thread.sleep(500);
            frame.hide();
            frame.show();
            System.gc();
            frame.dispose();
            if (frame.getPeer() != null) throw new Error("window peer not disposed");
        }
        System.out.println("DR2 original Frame peer passes three window lifetimes");
    }
}
