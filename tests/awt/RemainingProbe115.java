import java.awt.*;
class RemainingProbe115 {
    static { System.loadLibrary("awtremaining115probe"); }
    static native int exports(int group);
    static native int host();
    static native Frame embed(int host);
    static native boolean child(int host, boolean present);
    static native boolean destroy(int host);
    static native int dc();
    static native void print(Frame frame, int dc);
    static native int pixel(int x,int y);
    static native boolean closeDC();
    static native int icon(String title);
    static native int resources();
    static native boolean printing(boolean cancel);
    static native int calls(int operation);
    static native void duringPrint(Runnable callback);
}
