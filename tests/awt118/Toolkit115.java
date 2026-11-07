import java.awt.*;
import java.awt.image.*;

public class Toolkit115 {
    public static void main(String[] args) throws Exception {
        System.getProperties().put("awt.toolkit", "sun.awt.motif.MToolkit");
        if (args.length != 0) {
            try { Toolkit.getDefaultToolkit(); }
            catch (InternalError expected) {
                if (expected.getMessage().indexOf("X11") < 0) throw expected;
                System.out.println("display failure reported");
                System.exit(0);
            }
            throw new RuntimeException("invalid display accepted");
        }
        final Toolkit toolkit = Toolkit.getDefaultToolkit();
        Dimension size = toolkit.getScreenSize();
        if (size.width != 800 || size.height != 600) throw new RuntimeException("screen size " + size);
        if (toolkit.getScreenResolution() < 70 || toolkit.getScreenResolution() > 120)
            throw new RuntimeException("screen resolution");
        ColorModel model = toolkit.getColorModel();
        if (model.getRGB(0x123456) != 0xff123456) throw new RuntimeException("screen color model");
        int[] colors = new int[26];
        ((sun.awt.motif.MToolkit)toolkit).loadSystemColors(colors);
        if ((colors[17] >>> 24) != 255 || (colors[12] >>> 24) != 255)
            throw new RuntimeException("Motif system colors");
        Thread worker = new Thread(new Runnable() {
            public void run() { for (int i=0; i<50; i++) toolkit.sync(); }
        });
        worker.start();
        worker.join(5000);
        if (worker.isAlive()) throw new RuntimeException("event loop held toolkit monitor while idle");
        System.gc();
        toolkit.sync();
        System.out.println("toolkit display, colors and idle monitor passed");
        System.exit(0);
    }
}
