import java.awt.Canvas;
import java.awt.Color;
import java.awt.Rectangle;
import java.awt.event.PaintEvent;

public class ContainerDraw115 {
    public static void main(String[] args) {
        try {
            java.awt.Toolkit.getDefaultToolkit();
            Canvas source = new Canvas();
            source.setBackground(Color.red);
            for (int cycle = 0; cycle < 8; cycle++) {
                PaintEvent event = CanvasDraw115.draw(source, false, true);
                System.gc();
                if (event == null || event.getSource() != source || event.getID() != PaintEvent.PAINT ||
                    !event.getUpdateRect().equals(new Rectangle(4, 5, 20, 21)))
                    throw new Error("Container paint event: " + event);
            }
            System.out.println("DR2 containers clip child rectangles and preserve tracking and hit testing");
            System.exit(0);
        } catch (Throwable failure) {
            failure.printStackTrace();
            System.exit(1);
        }
    }
}
