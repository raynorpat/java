import java.awt.*;

public class WindowTreeLock115 {
    public static void main(String[] args) {
        try {
            for (int i = 0; i < 8; i++) {
                Frame frame = new Frame("tree lock");
                synchronized (frame.getTreeLock()) {
                    frame.addNotify();
                    if (frame.getSize().width < 97 || frame.getSize().height < 29)
                        throw new Error("minimum bounds not reflected: " + frame.getBounds());
                    frame.add(new Button("packed"));
                    frame.pack();
                }
                frame.dispose();
            }
            System.out.println("DR2 window creation preserves bounds under the AWT tree lock");
            System.exit(0);
        } catch (Throwable failure) {
            failure.printStackTrace();
            System.exit(1);
        }
    }
}
