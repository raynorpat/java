import java.awt.*;
import java.awt.image.*;

public class ProducerImage115 {
    static class CustomModel extends ColorModel {
        Image dispose;
        RuntimeException failure;
        CustomModel() { super(32); }
        public int getRed(int pixel) { return (pixel >> 16) & 255; }
        public int getGreen(int pixel) { return (pixel >> 8) & 255; }
        public int getBlue(int pixel) { return pixel & 255; }
        public int getAlpha(int pixel) { return 255; }
        public int getRGB(int pixel) {
            Toolkit.getDefaultToolkit().sync(); System.gc();
            if (dispose != null) { Image image = dispose; dispose = null; Image115.release(image); }
            if (failure != null) throw failure;
            return pixel | 0xff000000;
        }
    }
    static void safety(Image image) {
        ColorModel model = ColorModel.getRGBdefault();
        int[][] cases = {{0, 0, 8, 4, -1, 12}, {0, 0, 8, 4, 47, 12}, {0, 0, 8, 4, 0, 2147483647},
            {-1, 0, 8, 4, 0, 12}, {0, -1, 8, 4, 0, 12}, {1, 0, 8, 4, 0, 12}, {0, 1, 8, 4, 0, 12}};
        for (int bytes = 0; bytes < 2; bytes++) {
            Object pixels = bytes == 0 ? (Object)new int[48] : (Object)new byte[48];
            for (int i = 0; i < cases.length; i++) {
                int[] c = cases[i];
                try { Image115.upload(image, bytes != 0, c[0], c[1], c[2], c[3], model, pixels, c[4], c[5]); throw new Error("Missing upload bounds check"); }
                catch (ArrayIndexOutOfBoundsException expected) { }
            }
            try { Image115.upload(image, bytes != 0, 0, 0, -1, 1, model, pixels, 0, 12); throw new Error("Missing size check"); }
            catch (IllegalArgumentException expected) { }
            try { Image115.upload(image, bytes != 0, 0, 0, 8, 4, model, null, 0, 12); throw new Error("Missing null check"); }
            catch (NullPointerException expected) { }
            if (Image115.upload(image, bytes != 0, 0, 0, 0, 4, model, pixels, 0, 12)) throw new Error("Nonempty zero-width upload");
        }
        CustomModel custom = new CustomModel();
        custom.failure = new RuntimeException("custom model failure");
        try { Image115.upload(image, false, 0, 0, 8, 4, custom, new int[48], 0, 12); throw new Error("Missing model exception"); }
        catch (RuntimeException expected) { if (expected != custom.failure) throw expected; }
        custom.failure = null;
        if (!Image115.upload(image, false, 0, 0, 8, 4, custom, new int[48], 0, 12)) throw new Error("Failed upload recovery");
        custom.dispose = image;
        Image115.upload(image, false, 0, 0, 8, 4, custom, new int[48], 0, 12);
    }
    public static void main(String[] args) {
        try {
            Frame frame = new Frame();
            frame.setBounds(350, 100, 220, 150); frame.show(); Thread.sleep(150);
            if (args.length != 0) Image115.track();
            byte[] red = {(byte)255, 0, 0, (byte)255};
            byte[] green = {0, (byte)255, 0, (byte)255};
            byte[] blue = {0, 0, (byte)255, (byte)255};
            ColorModel[] models = {
                ColorModel.getRGBdefault(), new DirectColorModel(24, 0xff0000, 0xff00, 0xff),
                new IndexColorModel(8, 4, red, green, blue), new IndexColorModel(8, 4, red, green, blue, 3),
                new DirectColorModel(8, 0xe0, 0x1c, 3), new CustomModel()
            };
            if (args.length != 0) {
                Image failed = Toolkit.getDefaultToolkit().createImage(new MemoryImageSource(1, 1, new int[] {0xffff0000}, 0, 1));
                MediaTracker tracker = new MediaTracker(frame); tracker.addImage(failed, 0);
                Image115.failNext();
                tracker.waitForAll(5000);
                if (!tracker.isErrorAny()) throw new Error("Missing producer constructor failure");
                failed.flush();
                if (Image115.live() != 0) throw new Error("Failed producer constructor retained image");
            }
            for (int kind = 0; kind < models.length; kind++) {
                int[] samples = kind == 2 || kind == 3 ? new int[] {0, 1, 2, 3} :
                    kind == 4 ? new int[] {0xe0, 0x1c, 3, 0xff} : new int[] {0xffff0000, 0xff00ff00, 0xff0000ff, 0x00ffffff};
                for (int bytes = 0; bytes < (kind >= 2 && kind <= 4 ? 2 : 1); bytes++) {
                    int[] pixels = new int[48];
                    byte[] bytePixels = new byte[48];
                    for (int y = 0; y < 4; y++) for (int x = 0; x < 8; x++) {
                        int value = samples[x / 2]; pixels[2 + y * 12 + x] = value; bytePixels[2 + y * 12 + x] = (byte)value;
                    }
                    MemoryImageSource source = bytes == 0 ? new MemoryImageSource(8, 4, models[kind], pixels, 2, 12) :
                        new MemoryImageSource(8, 4, models[kind], bytePixels, 2, 12);
                    Image image = Toolkit.getDefaultToolkit().createImage(source);
                    MediaTracker tracker = new MediaTracker(frame); tracker.addImage(image, 0); tracker.waitForAll(5000);
                    if (!tracker.checkAll() || tracker.isErrorAny()) throw new Error("Producer image failed: " + kind);
                    Graphics g = frame.getGraphics(); g.setColor(Color.yellow); g.fillRect(0, 0, 220, 150);
                    g.drawImage(image, 10, 40, null); g.dispose(); Toolkit.getDefaultToolkit().sync();
                    String line = kind + ":" + bytes;
                    for (int x = 0; x < 8; x += 2) line += ":" + Integer.toHexString(Raster115.pixel(frame, 10 + x, 41));
                    System.out.println(line);
                    if (args.length != 0) safety(image);
                    image.flush(); System.gc();
                    if (args.length != 0 && Image115.live() != 0) throw new Error("Producer image retained: " + Image115.live());
                }
            }
            frame.dispose();
            System.out.println("DR2 producers preserve color-model delivery");
            System.exit(0);
        } catch (Throwable problem) { problem.printStackTrace(); System.exit(1); }
    }
}
