import java.awt.Toolkit;
public class AudioBackend115 {
    static native void exercise();
    static native int buffers();
    static native int lastSize();
    static native void failWrite(boolean fail);
    public static void main(String[] args) {
        try {
            Toolkit.getDefaultToolkit(); System.loadLibrary("mmedia"); System.loadLibrary("audiobackend115");
            exercise(); System.gc();
            if (Audio115.open() != 1) throw new Error("JNI audio open failed");
            byte[] samples = new byte[32];
            int before = buffers();
            Audio115.write(samples, 100);
            if (buffers() != before + 1 || lastSize() != samples.length) throw new Error("JNI audio clamp failed");
            Audio115.write(samples, 0); Audio115.write(samples, -1);
            Audio115.close(); Audio115.write(samples, 32);
            if (buffers() != before + 1) throw new Error("Closed or empty audio write");
            try { Audio115.write(null, 0); throw new Error("Missing null sample check"); }
            catch (NullPointerException expected) { }
            Audio115.open();
            for (int cycle = 0; cycle < 12; cycle++) {
                failWrite(true);
                try { Audio115.write(samples, 32); throw new Error("Missing write failure"); }
                catch (RuntimeException expected) {
                    if (expected.getMessage().indexOf("injected audio write") < 0) throw expected;
                }
                failWrite(false); System.gc(); Audio115.write(samples, 32);
            }
            Audio115.close();
            System.out.println("DR2 SoundKit buffer conversion, retry and release passed");
            System.exit(0);
        } catch (Throwable problem) { problem.printStackTrace(); System.exit(1); }
    }
}
