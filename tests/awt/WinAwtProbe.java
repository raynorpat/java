public class WinAwtProbe {
    static { System.loadLibrary("awt115probe"); }
    public static native void move(String title, int x, int y, int w, int h);
    public static native void close(String title);
    public static native void click(String title, String label);
    public static native int inkLeft(String title, String label);
    public static native int choice(String title, int index);
    public static native void listEvent(String title, int index, boolean action);
    public static native void scroll(String title, int code, int value);
    public static native void edit(String title, String text, boolean enter);
    public static native void paneScroll(String title, boolean vertical, int code, int value);
    public static native void windowState(String title, int state);
    public static native void paint(String title, String label);
    public static native void paintAll(String title);
    public static native void paneSpans(Object peer);
    public static native boolean windowEnabled(String title);
    public static native void chooseFile(String title, String path);
    public static native boolean fileDialogReady(String title);
}
