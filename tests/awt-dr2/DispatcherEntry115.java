import java.awt.Toolkit;

public class DispatcherEntry115 {
    static { System.loadLibrary("dispatcherentry115"); }
    static native void callRun();
    public static void main(String[] args) {
        try {
            Toolkit.getDefaultToolkit();
            try { callRun(); throw new Error("Obsolete dispatcher unexpectedly ran"); }
            catch (RuntimeException expected) {
                if (expected.getMessage() == null || expected.getMessage().indexOf("runWithEnv:") < 0) throw expected;
            }
            Toolkit.getDefaultToolkit().sync(); System.gc();
            System.out.println("DR2 obsolete dispatcher preserves donor exception and recovery");
            System.exit(0);
        } catch (Throwable problem) { problem.printStackTrace(); System.exit(1); }
    }
}
