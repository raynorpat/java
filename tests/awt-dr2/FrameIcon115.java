import java.awt.*;
import java.awt.image.*;
import java.awt.event.*;

public class FrameIcon115 {
    static { System.loadLibrary("frameicon115"); }
    static native boolean nativeIcon(Frame frame, boolean seed);
    static native void track();
    static native void miniaturize(Frame frame, boolean mini);
    static volatile int iconified, restored;
    static class UnreadableImage extends Image {
        public int getWidth(ImageObserver observer) { throw new Error("Unexpected image conversion"); }
        public int getHeight(ImageObserver observer) { throw new Error("Unexpected image conversion"); }
        public ImageProducer getSource() { throw new Error("Unexpected image conversion"); }
        public Graphics getGraphics() { throw new Error("Unexpected image conversion"); }
        public Object getProperty(String name, ImageObserver observer) { throw new Error("Unexpected image conversion"); }
        public void flush() { throw new Error("Unexpected image conversion"); }
    }
    public static void main(String[] args) {
        try {
            Toolkit.getDefaultToolkit(); track();
            for (int cycle = 0; cycle < 3; cycle++) {
                Frame frame = new Frame("DR2 frame icon");
                Image image = new UnreadableImage();
                frame.setIconImage(image); frame.setBounds(350, 100, 220, 150);
                frame.addWindowListener(new WindowAdapter() {
                    public void windowIconified(WindowEvent event) { iconified++; Toolkit.getDefaultToolkit().sync(); System.gc(); }
                    public void windowDeiconified(WindowEvent event) { restored++; Toolkit.getDefaultToolkit().sync(); System.gc(); }
                });
                frame.show(); Thread.sleep(100);
                if (nativeIcon(frame, false)) throw new Error("Unexpected initial native icon");
                if (!nativeIcon(frame, true)) throw new Error("Native icon seeding failed");
                image = new UnreadableImage();
                frame.setIconImage(image);
                if (nativeIcon(frame, false) || frame.getIconImage() != image) throw new Error("Icon setter mismatch");
                miniaturize(frame, true);
                for (int wait = 0; wait < 200 && iconified != cycle + 1; wait++) Thread.sleep(10);
                if (iconified != cycle + 1) throw new Error("Missing iconification event");
                miniaturize(frame, false);
                for (int wait = 0; wait < 200 && restored != cycle + 1; wait++) Thread.sleep(10);
                if (restored != cycle + 1) throw new Error("Missing restoration event");
                if (!nativeIcon(frame, true)) throw new Error("Native icon reseeding failed");
                frame.setIconImage(null);
                if (nativeIcon(frame, false) || frame.getIconImage() != null) throw new Error("Null icon mismatch");
                frame.dispose();
            }
            System.out.println("DR2 frame icons preserve donor clearing and miniaturization events");
            System.exit(0);
        } catch (Throwable problem) { problem.printStackTrace(); System.exit(1); }
    }
}
