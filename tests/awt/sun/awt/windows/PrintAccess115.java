package sun.awt.windows;
import java.awt.*;
public class PrintAccess115 {
    public static void print(Graphics graphics,Component component) {
        ((WGraphics)graphics).print((WComponentPeer)component.getPeer());
    }
}
