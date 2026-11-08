import java.awt.*;
import java.awt.image.*;
import java.util.Hashtable;

public class ImageDelivery115 implements ImageObserver {
    int flags, frames;
    public boolean imageUpdate(Image image, int info, int x, int y, int width, int height) {
        flags |= info;
        if ((info & FRAMEBITS) != 0) frames++;
        Toolkit.getDefaultToolkit().sync(); System.gc();
        return true;
    }
    static class Producer implements ImageProducer {
        ImageConsumer consumer;
        int removals;
        public void addConsumer(ImageConsumer value) { consumer = value; }
        public boolean isConsumer(ImageConsumer value) { return consumer == value; }
        public void removeConsumer(ImageConsumer value) {
            if (consumer == value) { consumer = null; removals++; }
        }
        public void requestTopDownLeftRightResend(ImageConsumer value) { }
        public void startProduction(ImageConsumer value) {
            addConsumer(value);
            value.setDimensions(8, 4);
            value.setProperties(new Hashtable());
            value.setColorModel(ColorModel.getRGBdefault());
            value.setHints(ImageConsumer.TOPDOWNLEFTRIGHT | ImageConsumer.COMPLETESCANLINES);
        }
        void rows(int y, int color) {
            int[] pixels = new int[16];
            for (int i = 0; i < pixels.length; i++) pixels[i] = color;
            consumer.setPixels(0, y, 8, 2, ColorModel.getRGBdefault(), pixels, 0, 8);
        }
    }
    static void check(boolean condition, String message) { if (!condition) throw new Error(message); }
    public static void main(String[] args) {
        try {
            Frame frame = new Frame("Image delivery"); frame.setBounds(100, 100, 200, 120); frame.show();
            Toolkit toolkit = Toolkit.getDefaultToolkit();
            for (int cycle = 0; cycle < 3; cycle++) {
                Producer source = new Producer();
                Image image = toolkit.createImage(source);
                ImageDelivery115 observer = new ImageDelivery115();
                check(!toolkit.prepareImage(image, -1, -1, observer), "Premature completion");
                check(source.consumer != null, "Production did not start");
                source.rows(0, 0xffff0000);
                check((observer.flags & SOMEBITS) != 0, "Missing partial delivery");
                source.rows(2, 0xff0000ff);
                source.consumer.imageComplete(ImageConsumer.SINGLEFRAMEDONE);
                check(observer.frames == 1, "Missing first frame");
                Graphics graphics = frame.getGraphics();
                graphics.drawImage(image, 10, 40, observer); graphics.dispose(); toolkit.sync();
                System.out.println("frame " + cycle + ":" + Integer.toHexString(Raster115.pixel(frame, 11, 40)) + ":" + Integer.toHexString(Raster115.pixel(frame, 11, 42)));
                source.rows(0, 0xff00ff00);
                source.consumer.imageComplete(ImageConsumer.SINGLEFRAMEDONE);
                check(observer.frames == 2, "Missing second frame");
                graphics = frame.getGraphics(); graphics.drawImage(image, 10, 40, observer); graphics.dispose(); toolkit.sync();
                System.out.println("updated " + cycle + ":" + Integer.toHexString(Raster115.pixel(frame, 11, 40)));
                source.consumer.imageComplete(ImageConsumer.IMAGEABORTED);
                check((observer.flags & ABORT) != 0, "Missing abort delivery");
                image.flush(); System.gc();
                check(source.consumer == null && source.removals > 0, "Aborted consumer retained");
            }
            frame.dispose();
            System.out.println("DR2 partial, animated and aborted image delivery passed");
            System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
