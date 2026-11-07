import java.awt.*;
import java.awt.image.*;

public class Toolkit115 {
    public static void main(String[] args) throws Exception {
        try { run(); } catch (Throwable failure) {
            failure.printStackTrace(); System.exit(1);
        }
    }
    private static void run() throws Exception {
        Toolkit t = Toolkit.getDefaultToolkit();
        if (t != Toolkit.getDefaultToolkit()) throw new RuntimeException("toolkit identity");
        Dimension d = t.getScreenSize();
        if (d.width <= 0 || d.height <= 0 || t.getScreenResolution() <= 0)
            throw new RuntimeException("screen metrics");
        ColorModel model = t.getColorModel();
        if (model.getRed(0x123456) != 0x56 || model.getGreen(0x123456) != 0x34 ||
            model.getBlue(0x123456) != 0x12) throw new RuntimeException("color model");
        t.sync();
        for (int i = 0; i < 10; i++) {
            System.gc();
            if (t.getColorModel() != model) throw new RuntimeException("color model identity");
        }
        System.out.println("toolkit startup, queries, colors and GC passed");
        System.exit(0);
    }
}
