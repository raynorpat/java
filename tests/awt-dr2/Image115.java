import java.awt.*;
import java.awt.image.*;

public class Image115 {
    static { System.loadLibrary("image115"); }
    static native void track();
    static native int live();
    static native void failNext();
    static native boolean hasFocus();
    static native int focusExceptions();
    static native void release(Image image);
    static native boolean upload(Image image, boolean bytes, int x, int y, int width, int height,
        ColorModel model, Object pixels, int offset, int scan);
    static class Consumer implements ImageConsumer {
        Image image;
        ImageProducer source;
        int at, action, rows, hints;
        RuntimeException marker = new RuntimeException("image callback failure");
        Consumer(Image image, int at, int action) {
            this.image = image; source = image.getSource(); this.at = at; this.action = action;
        }
        void callback(int stage) {
            Toolkit.getDefaultToolkit().sync(); System.gc();
            if (stage != at) return;
            if (action == 0) source.removeConsumer(this);
            if (action == 1) release(image);
            if (action == 2) throw marker;
        }
        public void setDimensions(int w, int h) { }
        public void setProperties(java.util.Hashtable properties) { }
        public void setColorModel(ColorModel model) { callback(1); }
        public void setHints(int value) { hints = value; callback(2); }
        public void setPixels(int x, int y, int w, int h, ColorModel model, byte[] pixels, int offset, int scan) { throw new Error("Unexpected byte delivery"); }
        public void setPixels(int x, int y, int w, int h, ColorModel model, int[] pixels, int offset, int scan) {
            rows++; callback(3);
        }
        public void imageComplete(int status) { }
    }
    static void callbacks(Frame frame, boolean tracking) {
        for (int action = 0; action < (tracking ? 3 : 1); action++) for (int at = 1; at <= 3; at++) {
            Image image = frame.createImage(8, 4);
            Graphics g = image.getGraphics(); g.setColor(Color.red); g.fillRect(0, 0, 8, 4); g.dispose();
            Consumer consumer = new Consumer(image, at, action);
            try {
                consumer.source.startProduction(consumer);
                if (action == 2) throw new Error("Missing callback exception");
            } catch (RuntimeException expected) {
                if (action != 2 || expected != consumer.marker) throw expected;
            }
            if (action == 0 && consumer.rows != (at == 3 ? 1 : 0)) throw new Error("Consumer removal ignored");
            if (action == 1 && consumer.rows != 4) throw new Error("Disposal interrupted retained image delivery");
            if (at == 3 && consumer.hints != 11) throw new Error("Wrong delivery hints");
            if (tracking) release(image);
        }
        if (tracking && live() != 0) throw new Error("Callback images retained: " + live());
    }
    public static void main(String[] args) {
        try {
            Frame frame = new Frame();
            frame.setBounds(350, 100, 220, 150);
            frame.setBackground(Color.green);
            frame.show(); Thread.sleep(150);
            if (args.length != 0) {
                track(); failNext();
                try { frame.createImage(8, 4); throw new Error("Missing constructor failure"); }
                catch (RuntimeException expected) {
                    if (expected.getMessage().indexOf("image constructor failure") < 0) throw expected;
                }
                if (live() != 0) throw new Error("Failed constructor retained image");
            }
            for (int cycle = 0; cycle < 3; cycle++) {
                Image image = frame.createImage(30, 20);
                Graphics g = image.getGraphics();
                g.setColor(Color.red); g.fillRect(0, 0, 15, 20);
                g.setColor(Color.blue); g.fillRect(15, 0, 15, 20);
                g.dispose();
                int[] pixels = new int[600];
                PixelGrabber grab = new PixelGrabber(image, 0, 0, 30, 20, pixels, 0, 30);
                if (!grab.grabPixels(5000)) throw new Error("Offscreen pixel delivery failed");
                System.out.println("pixels:" + Integer.toHexString(pixels[0]) + ":" + Integer.toHexString(pixels[29]) + ":" + Integer.toHexString(pixels[599]));
                g = frame.getGraphics();
                g.drawImage(image, 10, 40, null);
                g.dispose(); Toolkit.getDefaultToolkit().sync();
                System.out.println("screen:" + Integer.toHexString(Raster115.pixel(frame, 12, 42)) + ":" + Integer.toHexString(Raster115.pixel(frame, 38, 42)));
                image.flush(); System.gc();
                if (args.length != 0) {
                    Graphics retained = image.getGraphics();
                    release(image);
                    if (live() != 1) throw new Error("Graphics lost its image");
                    retained.setColor(Color.green); retained.fillRect(0, 0, 5, 5);
                    retained.dispose();
                    if (live() != 0) throw new Error("Offscreen image retained: " + live());
                }
            }
            callbacks(frame, args.length != 0);
            frame.dispose();
            System.out.println("DR2 offscreen images draw and deliver pixels");
            System.exit(0);
        } catch (Throwable problem) { problem.printStackTrace(); System.exit(1); }
    }
}
