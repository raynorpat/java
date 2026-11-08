import java.awt.*;
import java.awt.peer.ComponentPeer;

public class ComponentState115 {
    static { System.loadLibrary("componentstate115"); }
    static native boolean focused(ComponentPeer peer);
    static native boolean cursor(ComponentPeer peer, int type);
    static native boolean repaint(ComponentPeer peer);
    static native void fractionalSize(ComponentPeer peer);
    static void require(boolean value, String message) { if (!value) throw new Error(message); }
    public static void main(String[] args) {
        try {
            for (int pass = 0; pass < 3; pass++) {
                Frame frame = new Frame(); frame.setLayout(null); frame.setBounds(350, 100, 300, 200);
                Panel panel = new Panel(null); panel.setBounds(20, 40, 200, 120);
                Canvas first = new Canvas(), second = new Canvas();
                first.setBounds(10, 15, 80, 60); second.setBounds(100, 15, 60, 40);
                panel.add(first); panel.add(second); frame.add(panel); frame.show();
                ComponentPeer peer = first.getPeer();
                Point origin = panel.getLocationOnScreen(), point = first.getLocationOnScreen();
                require(point.x == origin.x + 10 && point.y == origin.y + 15, "nested screen coordinates");
                require(peer.getPreferredSize().equals(new Dimension(80, 60)), "preferred size");
                require(peer.getMinimumSize().equals(new Dimension(80, 60)), "minimum size");
                first.setBounds(-5, 8, 40, 30);
                point = first.getLocationOnScreen();
                require(point.x == origin.x - 5 && point.y == origin.y + 8, "moved screen coordinates");
                require(peer.getPreferredSize().equals(new Dimension(40, 30)), "resized preferred size");
                require(peer.isFocusTraversable(), "enabled focus traversal");
                first.requestFocus(); require(focused(peer), "focus request");
                second.requestFocus(); require(focused(second.getPeer()), "second focus request");
                first.setEnabled(false);
                require(!peer.isFocusTraversable(), "disabled focus traversal");
                first.requestFocus(); require(focused(second.getPeer()), "disabled focus request changed responder");
                first.setEnabled(true); first.requestFocus(); require(focused(peer), "restored focus request");
                for (int type = 0; type < 14; type++) {
                    first.setCursor(new Cursor(type)); require(cursor(peer, type), "cursor " + type);
                }
                require(repaint(peer), "native repaint invalidation");
                fractionalSize(peer);
                require(peer.getPreferredSize().equals(new Dimension(41, 31)), "fractional preferred size rounds up");
                Toolkit.getDefaultToolkit().sync(); System.gc(); frame.dispose();
            }
            System.out.println("DR2 component coordinates, sizing, focus, cursors and repaint agree");
            System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
