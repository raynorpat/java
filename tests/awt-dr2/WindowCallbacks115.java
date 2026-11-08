import java.awt.Frame;
import java.awt.Rectangle;
import java.awt.Toolkit;
import java.awt.event.WindowEvent;

public class WindowCallbacks115 {
    static { System.loadLibrary("windowcallbacks115"); }
    static native WindowEvent exercise(Frame source, int kind);
    public static void main(String[] args) {
        try {
            int height = Toolkit.getDefaultToolkit().getScreenSize().height;
            int[] ids = {WindowEvent.WINDOW_CLOSING, WindowEvent.WINDOW_ICONIFIED,
                WindowEvent.WINDOW_DEICONIFIED, WindowEvent.WINDOW_ACTIVATED, WindowEvent.WINDOW_DEACTIVATED};
            for (int cycle = 0; cycle < 10; cycle++) {
                Frame source = new Frame("native callbacks");
                WindowEvent event = exercise(source, cycle % 5);
                Rectangle expected = cycle % 5 % 2 == 0 ? new Rectangle(100, height - 300, 300, 180)
                    : new Rectangle(130, height - 340, 320, 200);
                System.gc();
                if (!source.getBounds().equals(expected)) throw new Error("window bounds: " + source.getBounds());
                if (event == null || event.getSource() != source || event.getID() != ids[cycle % 5])
                    throw new Error("window callback: " + event);
            }
            System.out.println("DR2 window delegates update Java geometry and queue lifecycle events");
            System.exit(0);
        } catch (Throwable failure) {
            failure.printStackTrace();
            System.exit(1);
        }
    }
}
