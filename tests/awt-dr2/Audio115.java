import java.awt.Toolkit;

public class Audio115 {
    static native int open();
    static native void close();
    static native void write(byte[] samples, int length);
    public static void main(String[] args) {
        try {
            Toolkit.getDefaultToolkit();
            System.loadLibrary("mmedia"); System.loadLibrary("audio115");
            byte[] silence = new byte[400];
            for (int i = 0; i < silence.length; i++) silence[i] = (byte)255;
            for (int cycle = 0; cycle < 2; cycle++) {
                int status = open();
                if (status < -1 || status > 1) throw new Error("Invalid audio status: " + status);
                System.out.println("audio open=" + status);
                if (status == 1) {
                    if (open() != 1) throw new Error("Audio reopen failed");
                    write(silence, silence.length + 10);
                    write(silence, 0); write(silence, -1);
                }
                close(); close(); write(silence, silence.length);
            }
            System.out.println("DR2 audio open, silence writes and close passed");
            System.exit(0);
        } catch (Throwable problem) { problem.printStackTrace(); System.exit(1); }
    }
}
