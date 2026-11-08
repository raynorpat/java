import java.awt.Cursor;
import java.awt.Toolkit;
public class Cursors115 {
    static { System.loadLibrary("cursors115"); }
    static native boolean matches(Cursor cursor, int expected);
    static class Unknown extends Cursor {
        Unknown() { super(Cursor.DEFAULT_CURSOR); }
        public int getType() { return 99; }
    }
    public static void main(String[] args) {
        try {
            Toolkit.getDefaultToolkit();
            for (int cycle = 0; cycle < 20; cycle++) {
                for (int type = 0; type <= 13; type++)
                    if (!matches(Cursor.getPredefinedCursor(type), type)) throw new Error("cursor " + type);
                System.gc();
            }
            if (!matches(null, -1) || !matches(new Unknown(), -1)) throw new Error("null/unknown cursor");
            System.out.println("DR2 cursor conversion matches all native cursor identities");
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
        System.exit(0);
    }
}
