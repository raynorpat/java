import java.awt.*;

public class Decorated115 {
    static class Preferred extends Canvas {
        public Dimension getPreferredSize() { return new Dimension(200,100); }
    }
    public static void main(String[] args) {
        try {
            Toolkit.getDefaultToolkit();
            System.loadLibrary("motif118xprobe");
            for (int cycle=0; cycle<4; cycle++) {
                Frame frame = new Frame("Decorated frame");
                Button button = new Button("Client");
                frame.add(button);
                frame.setBounds(70, 80, 320, 220);
                frame.show();
                Thread.sleep(700);
                Insets insets = frame.getInsets();
                if (insets.top <= 0 || insets.left <= 0 || insets.bottom <= 0)
                    throw new RuntimeException("missing decorations: " + insets);
                frame.setSize(350, 240);
                Thread.sleep(400);
                frame.validate();
                Point outer = frame.getLocationOnScreen();
                Point inner = button.getLocationOnScreen();
                if (inner.x-outer.x != insets.left || inner.y-outer.y != insets.top)
                    throw new RuntimeException("client origin: " + outer + " " + inner + " " + insets);
                if (button.getSize().width != 350-insets.left-insets.right ||
                    button.getSize().height != 240-insets.top-insets.bottom)
                    throw new RuntimeException("client size: " + button.getSize());
                frame.remove(button);
                Graphics graphics=frame.getGraphics();
                graphics.setColor(Color.red);
                graphics.fillRect(insets.left, insets.top, 5, 5);
                graphics.dispose();
                Toolkit.getDefaultToolkit().sync();
                if ((Window115.pixel(frame, 1, 1)&0xffffff)!=0xff0000)
                    throw new RuntimeException("frame graphics origin");
                frame.dispose();
            }
            Frame packed=new Frame("Packed frame");
            Canvas preferred=new Preferred(); packed.add(preferred); packed.pack(); packed.show();
            Thread.sleep(500);
            if (preferred.getSize().width!=200 || preferred.getSize().height!=100)
                throw new RuntimeException("packed content clipped: "+preferred.getSize());
            packed.dispose();
            System.out.println("decorated frame bounds and client layout passed");
            System.exit(0);
        } catch (Throwable failure) {
            failure.printStackTrace();
            System.exit(1);
        }
    }
}
