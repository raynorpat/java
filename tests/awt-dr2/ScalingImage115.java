import java.awt.*;
import java.awt.image.*;

public class ScalingImage115 {
    public static void main(String[] args) {
        try {
            Frame frame = new Frame(); frame.setBounds(350, 100, 220, 150); frame.show(); Thread.sleep(150);
            if (args.length != 0) {
                int failures = Image115.focusExceptions();
                if (failures != 0) throw new Error("Drawing failure focus stages: " + failures);
                Image115.track();
            }
            int[] colors = {0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffffff};
            for (int cycle = 0; cycle < 2; cycle++) for (int source = 0; source < 2; source++) {
                Image image;
                if (source == 0) {
                    image = frame.createImage(16, 8);
                    Graphics g = image.getGraphics();
                    for (int y = 0; y < 2; y++) for (int x = 0; x < 2; x++) {
                        g.setColor(new Color(colors[y * 2 + x])); g.fillRect(x * 8, y * 4, 8, 4);
                    }
                    g.dispose();
                } else {
                    int[] pixels = new int[128];
                    for (int y = 0; y < 8; y++) for (int x = 0; x < 16; x++) pixels[y * 16 + x] = colors[(y / 4) * 2 + x / 8];
                    image = Toolkit.getDefaultToolkit().createImage(new MemoryImageSource(16, 8, pixels, 0, 16));
                    MediaTracker tracker = new MediaTracker(frame); tracker.addImage(image, 0); tracker.waitForAll(5000);
                    if (!tracker.checkAll() || tracker.isErrorAny()) throw new Error("Image load failed");
                }
                if (args.length != 0) {
                    Graphics g = frame.getGraphics(); Image115.failNext();
                    try { g.drawImage(image, 10, 40, 64, 32, null); throw new Error("Missing drawing failure"); }
                    catch (RuntimeException expected) {
                        if (expected.getMessage().indexOf("image drawing failure") < 0) throw expected;
                    }
                    g.dispose();
                    if (Image115.hasFocus()) throw new Error("Image exception retained drawing focus");
                }
                for (int step = 0; step < 8; step++) {
                    int kind = cycle == 1 && source == 1 && step < 2 ? 1 - step : step;
                    Graphics g = frame.getGraphics(); g.setColor(Color.yellow); g.fillRect(0, 0, 220, 150);
                    switch (kind) {
                    case 0: g.drawImage(image, 10, 40, null); break;
                    case 1: g.drawImage(image, 10, 40, 64, 32, null); break;
                    case 2: g.drawImage(image, 10, 40, 70, 80, 2, 1, 14, 7, null); break;
                    case 3: g.drawImage(image, 10, 40, 70, 80, 14, 1, 2, 7, null); break;
                    case 4: g.drawImage(image, 70, 40, 10, 80, 2, 1, 14, 7, null); break;
                    case 5: g.drawImage(image, 10, 80, 70, 40, 2, 1, 14, 7, null); break;
                    case 6: g.drawImage(image, 10, 40, 22, 46, 2, 1, 14, 7, null); break;
                    case 7: g.drawImage(image, 10, 40, 10, 80, 2, 1, 14, 7, null); break;
                    }
                    g.dispose(); Toolkit.getDefaultToolkit().sync();
                    System.out.println(cycle + ":" + source + ":" + kind + ":" + Primitives115.checksum(frame));
                    System.gc();
                }
                image.flush();
                if (args.length != 0) {
                    if (source == 0) Image115.release(image);
                    if (Image115.live() != 0) throw new Error("Scaled image retained: " + Image115.live());
                }
            }
            frame.dispose();
            System.out.println("DR2 images preserve scaling, cropping and mirrored destinations");
            System.exit(0);
        } catch (Throwable problem) { problem.printStackTrace(); System.exit(1); }
    }
}
