import java.awt.*;

/* No peer-specific imports or test native library: exercise installed defaults. */
public class InstalledAwt115 {
    public static void main(String[] args) {
        try {
            Toolkit toolkit = Toolkit.getDefaultToolkit();
            if (!toolkit.getClass().getName().equals("com.apple.rhapsody.awt.RToolkit"))
                throw new Error("Wrong installed toolkit: " + toolkit);
            Frame frame = new Frame("Installed DR2 AWT");
            frame.add(new Button("Installed peer"));
            frame.setSize(240, 100); frame.show();
            toolkit.sync(); System.gc();
            if (!frame.isShowing()) throw new Error("Installed frame is not showing");
            frame.dispose();
            System.loadLibrary("mmedia");
            System.out.println("Installed DR2 AWT and audio libraries passed");
            System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
