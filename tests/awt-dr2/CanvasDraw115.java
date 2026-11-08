import java.awt.Canvas;
import java.awt.Color;
import java.awt.Rectangle;
import java.awt.event.PaintEvent;

public class CanvasDraw115 {
    static { System.loadLibrary("canvasdraw115"); }
    static native PaintEvent draw(Canvas source, boolean inherited);
    public static void main(String[] args) {
        try {
            java.awt.Toolkit.getDefaultToolkit();
            Canvas source = new Canvas();
            for (int cycle = 0; cycle < 8; cycle++) {
                boolean inherited = cycle % 2 != 0;
                source.setBackground(inherited ? null : Color.red);
                PaintEvent event = draw(source, inherited);
                System.gc();
                if (event == null || event.getSource() != source || event.getID() != PaintEvent.PAINT ||
                    !event.getUpdateRect().equals(new Rectangle(4, 5, 20, 21)))
                    throw new Error("Canvas paint event: " + event);
            }
            System.out.println("DR2 Canvas draws its background and queues the Java paint rectangle");
            System.exit(0);
        } catch (Throwable failure) {
            failure.printStackTrace();
            System.exit(1);
        }
    }
}
