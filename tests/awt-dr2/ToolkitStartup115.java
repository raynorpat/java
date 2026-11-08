import java.awt.Toolkit;
import java.awt.Component;
import java.awt.AWTEvent;
import java.awt.event.MouseEvent;
import com.apple.rhapsody.awt.RToolkit;

public class ToolkitStartup115 {
    static { System.loadLibrary("toolkitprobe115"); }
    static native boolean dispatch(AWTEvent event);
    static int callbacks;
    static class Target extends Component {
        Target() { enableEvents(AWTEvent.MOUSE_EVENT_MASK); }
        protected void processEvent(AWTEvent event) {
            System.gc();
            if (Toolkit.getDefaultToolkit().getScreenSize().width <= 0)
                throw new Error("callback could not re-enter AppKit");
            callbacks++;
            ((MouseEvent)event).consume();
        }
    }
    public static void main(String[] args) {
        try { run(); }
        catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
        System.exit(0);
    }
    static void run() {
        Toolkit toolkit = Toolkit.getDefaultToolkit();
        if (!(toolkit instanceof RToolkit)) throw new Error("wrong toolkit: " + toolkit);
        if (!((RToolkit)toolkit).appkitIsRunning()) throw new Error("AppKit did not start");
        if (Toolkit.getDefaultToolkit() != toolkit) throw new Error("toolkit identity changed");
        System.gc();
        Target target = new Target();
        for (int i = 0; i < 20; i++) {
            if (!dispatch(new MouseEvent(target, MouseEvent.MOUSE_PRESSED, 0L, 0, 1, 1, 1, false)))
                throw new Error("original peer lost consumed event");
        }
        if (callbacks != 20) throw new Error("original peer did not dispatch callbacks: " + callbacks);
        System.out.println("DR2 original Toolkit peer starts the AppKit loop");
        System.exit(0);
    }
}
