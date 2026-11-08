import java.awt.*;
import java.awt.peer.ComponentPeer;
import java.awt.event.*;

public class BareWindow115 {
    static { System.loadLibrary("barewindow115"); }
    static native void track();
    static native int liveObjects();
    static native void failAt(int stage);
    static native boolean focused(ComponentPeer peer);
    static native void fractional(ComponentPeer peer);
    static native void close(ComponentPeer peer);
    static final RuntimeException marker = new RuntimeException("window font failure");
    static volatile int closed;
    static class BadFont extends Font {
        BadFont() { super("Dialog", Font.PLAIN, 12); }
        public int getSize() { throw marker; }
    }
    static void awaitObjects(int expected) throws Exception {
        for (int wait = 0; wait < 100 && liveObjects() != expected; wait++) {
            Thread.sleep(50); Toolkit.getDefaultToolkit().sync(); System.gc();
        }
        require(liveObjects() == expected, "native window objects retained: " + liveObjects());
    }
    static void failedConstruction(Frame owner, int stage) throws Exception {
        Window window = new Window(owner); window.setBounds(370, 130, 200, 120);
        if (stage == 0) window.setFont(new BadFont());
        failAt(stage);
        try { window.addNotify(); throw new Error("window construction succeeded"); }
        catch (RuntimeException failure) {
            if (stage == 0) require(failure == marker, "window throwable identity");
            else require(failure.getMessage().indexOf("construction failure") >= 0, "native window exception");
        } finally { failAt(0); }
        require(window.getPeer() == null, "failed window peer"); awaitObjects(2);
    }
    static void require(boolean value, String message) { if (!value) throw new Error(message); }
    public static void main(String[] args) {
        try {
            boolean tracked = args.length != 0;
            if (tracked) { Toolkit.getDefaultToolkit(); track(); }
            Frame owner = new Frame("Window owner"); owner.setBounds(350, 100, 300, 240); owner.show();
            if (tracked) for (int stage = 0; stage < 3; stage++) failedConstruction(owner, stage);
            for (int cycle = 0; cycle < 3; cycle++) {
                final Window window = new Window(owner); window.setBounds(370, 130, 200, 120);
                window.show(); ComponentPeer peer = window.getPeer();
                require(peer.getPreferredSize().equals(new Dimension(200, 120)), "native window preferred size");
                require(window.getInsets().equals(new Insets(0, 0, 0, 0)), "borderless insets");
                require(!peer.isFocusTraversable(), "window focus traversal");
                peer.requestFocus(); Toolkit.getDefaultToolkit().sync();
                require(focused(peer), "native window first responder");
                require(peer.getMinimumSize().equals(new Dimension(29, 10)), "native window minimum size");
                window.setBounds(390, 140, 220, 130);
                require(window.getLocationOnScreen().equals(new Point(390, 140)), "window location");
                require(peer.getPreferredSize().equals(new Dimension(220, 130)), "resized preferred size");
                fractional(peer); require(peer.getPreferredSize().equals(new Dimension(221, 131)), "fractional window sizing");
                window.toBack(); window.toFront(); window.hide(); window.show();
                window.addWindowListener(new WindowAdapter() {
                    public void windowClosing(WindowEvent event) {
                        Toolkit.getDefaultToolkit().sync(); System.gc(); window.dispose(); closed++;
                    }
                });
                close(peer);
                for (int wait = 0; wait < 200 && closed <= cycle; wait++) Thread.sleep(10);
                require(closed == cycle + 1, "native closing callback");
                require(window.getPeer() == null, "window disposal");
            }
            owner.dispose(); if (tracked) awaitObjects(0);
            System.out.println("DR2 borderless windows preserve geometry, sizing and focus contracts");
            System.exit(0);
        } catch (Throwable problem) { problem.printStackTrace(); System.exit(1); }
    }
}
