import java.awt.*;
public class FrameworkRuntime {
    static native int verifyImages(String root);
    static native int callback(int value);
    static int twice(int value) { return value * 2; }
    public static void main(String[] args) throws Exception {
        System.load(args[0]);
        if (callback(21) != 42) throw new RuntimeException("JNI callback");
        if (!System.getProperty("java.home").equals(args[1] + "/Home"))
            throw new RuntimeException("java.home=" + System.getProperty("java.home"));
        if (!new java.math.BigInteger("123").multiply(new java.math.BigInteger("456")).toString().equals("56088"))
            throw new RuntimeException("math library");
        java.util.zip.Adler32 checksum = new java.util.zip.Adler32();
        checksum.update(new byte[] {1, 2, 3});
        if (checksum.getValue() != 851975L) throw new RuntimeException("zip library");
        if (java.net.InetAddress.getByName("127.0.0.1").getAddress().length != 4)
            throw new RuntimeException("net library");
        Toolkit toolkit = Toolkit.getDefaultToolkit();
        if (!toolkit.getClass().getName().equals("com.apple.rhapsody.awt.RToolkit"))
            throw new RuntimeException("unexpected AWT toolkit");
        Frame frame = new Frame("Private framework acceptance");
        frame.add(new Button("JavaVM.framework"));
        frame.setBounds(400, 100, 300, 180);
        frame.show();
        toolkit.sync();
        if (!frame.isVisible() || frame.getPeer() == null)
            throw new RuntimeException("window not realized");
        frame.dispose();
        if (frame.getPeer() != null) throw new RuntimeException("window not disposed");
        AudioLibrary115.main(new String[0]);
        if (verifyImages(args[1]) != 3) throw new RuntimeException("private native images");
        System.out.println("PASS framework console JNI AWT audio and provenance");
        System.exit(0);
    }
}
