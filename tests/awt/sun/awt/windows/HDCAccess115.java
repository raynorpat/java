package sun.awt.windows;
import java.awt.Graphics;
public class HDCAccess115 {
    public static Graphics create(java.awt.peer.ComponentPeer peer) { return new WGraphics((WComponentPeer)peer); }
    public static Graphics create(int dc) { return new WGraphics(dc); }
}
