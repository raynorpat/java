import java.awt.*;

public class Smoke115 {
    public static void main(String[] args) throws Exception {
        Toolkit toolkit = Toolkit.getDefaultToolkit();
        if (!toolkit.getClass().getName().equals("com.apple.rhapsody.awt.RToolkit"))
            throw new RuntimeException("unexpected toolkit: " + toolkit.getClass().getName());
        for (int cycle = 0; cycle < 3; cycle++) {
            Frame frame = new Frame("DR2 AppKit smoke " + cycle);
            frame.add(new Button("DR2 AppKit"));
            frame.setBounds(400, 100, 300, 180);
            frame.show(); frame.toFront();
            toolkit.sync();
            if (!frame.isVisible() || frame.getPeer() == null)
                throw new RuntimeException("frame not realized");
            Thread.sleep(1500);
            frame.dispose();
            if (frame.getPeer() != null)
                throw new RuntimeException("frame peer not disposed");
        }
        System.out.println("DR2 AppKit startup and three window lifetimes passed");
        System.exit(0);
    }
}
