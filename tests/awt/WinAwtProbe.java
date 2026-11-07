public class WinAwtProbe {
    static { System.loadLibrary("awt115probe"); }
    public static native void move(String title, int x, int y, int w, int h);
    public static native void close(String title);
    public static native void click(String title, String label);
    public static native int inkLeft(String title, String label);
}
