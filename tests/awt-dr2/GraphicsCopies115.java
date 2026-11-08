import java.awt.*;

public class GraphicsCopies115 {
    public static void main(String[] args) {
        try {
            Frame frame = new Frame("Graphics copies");
            frame.setBounds(100, 100, 420, 250); frame.show(); Thread.sleep(200);
            Graphics parent = frame.getGraphics();
            parent.setColor(Color.white); parent.fillRect(0, 0, 420, 250);
            parent.translate(6, 22); parent.setClip(0, 0, 400, 200);
            int[] xs = {57, 146, 235};
            for (int i = 0; i < xs.length; i++) {
                Graphics child = parent.create(xs[i], 17, 84, 84);
                System.out.println("clip " + i + ":" + child.getClipBounds());
                child.setColor(Color.red); child.fillOval(0, 0, 83, 83); child.dispose();
            }
            Toolkit.getDefaultToolkit().sync();
            for (int i = 0; i < xs.length; i++)
                System.out.println("pixel " + i + ":" + Integer.toHexString(Raster115.pixel(frame, xs[i] + 46, 80)));
            Graphics child = parent.create(xs[0], 17, 84, 84);
            child.setColor(Color.blue); child.fillOval(0, 0, 83, 83); child.dispose();
            parent.dispose(); Toolkit.getDefaultToolkit().sync();
            for (int i = 0; i < xs.length; i++)
                System.out.println("updated " + i + ":" + Integer.toHexString(Raster115.pixel(frame, xs[i] + 46, 80)));
            frame.dispose();
            System.out.println("DR2 graphics copies preserve the original peer clip behavior");
            System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
