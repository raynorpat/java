import java.awt.Canvas;
import java.awt.Rectangle;
import java.awt.event.PaintEvent;

public class PaintCover115 {
    static { System.loadLibrary("paintcover115"); }
    static native PaintEvent make(Canvas source, int type, float x, float y, float w, float h);
    public static void main(String[] args) {
        try {
            java.awt.Toolkit.getDefaultToolkit();
            Canvas source = new Canvas();
            for (int cycle = 0; cycle < 40; cycle++) {
                int type = cycle % 2 == 0 ? PaintEvent.PAINT : PaintEvent.UPDATE;
                PaintEvent event = make(source, type, -2.75f, 3.875f, 15.75f, 19.125f);
                System.gc();
                if (event.getSource() != source || event.getID() != type ||
                    !event.getUpdateRect().equals(new Rectangle(-2, 3, 15, 19)))
                    throw new Error("paint event conversion");
            }
            System.out.println("DR2 paint covers preserve source, type and truncated rectangle");
            System.exit(0);
        } catch (Throwable failure) {
            failure.printStackTrace();
            System.exit(1);
        }
    }
}
