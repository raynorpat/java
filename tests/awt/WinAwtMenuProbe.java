class WinAwtMenuProbe {
    static { System.loadLibrary("awtmenu115probe"); }
    static native int exports();
    static native int query(String title, int menu, int item, int nested, int operation);
    static native void activate(String title, int menu, int item, int nested);
    static native boolean popup(String title);
    static native void queueAndRetire(String title, int menu, int item, java.awt.MenuItem itemPeer);
}
