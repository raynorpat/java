public class WinAwtProbe {
    static { System.loadLibrary("awt115probe"); }
    public static native void move(String title, int x, int y, int w, int h);
    public static native void close(String title);
    public static native void click(String title, String label);
    public static native int inkLeft(String title, String label);
    public static native int choice(String title, int index);
    public static native void listEvent(String title, int index, boolean action);
    public static native void scroll(String title, int code, int value);
}
