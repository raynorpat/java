import java.awt.Frame;
import java.awt.Toolkit;

public class WindowLifetime115 {
    static { System.loadLibrary("windowlifetime115"); }
    static native void installTracker();
    static native int liveWindows();
    static native int liveViews();
    static native int visibleWindows();
    public static void main(String[] args) {
        try {
            installTracker();
            Toolkit toolkit = Toolkit.getDefaultToolkit();
            for (int cycle = 0; cycle < 5; cycle++) {
                Frame frame = new Frame("native lifetime " + cycle);
                frame.setBounds(360, 100, 300, 180);
                frame.show();
                toolkit.sync();
                if (liveWindows() != 1 || liveViews() != 1) throw new Error("native window/view not created");
                if (visibleWindows() != 1) throw new Error("native window not shown");
                frame.hide();
                if (visibleWindows() != 0) throw new Error("native window not hidden");
                frame.show();
                if (visibleWindows() != 1) throw new Error("native window not reshown");
                frame.dispose();
                for (int retry = 0; retry < 40 && (liveWindows() != 0 || liveViews() != 0); retry++) {
                    System.gc();
                    Thread.sleep(50);
                    toolkit.sync();
                }
                if (liveWindows() != 0) throw new Error("native window retained after disposal: " + liveWindows());
                if (liveViews() != 0) throw new Error("native view retained after disposal: " + liveViews());
                if (visibleWindows() != 0) throw new Error("disposed native window still visible");
                if (frame.getPeer() != null) throw new Error("Java peer not disposed");
            }
            System.out.println("DR2 Frame disposal releases all five native windows and views");
            System.exit(0);
        } catch (Throwable failure) {
            failure.printStackTrace();
            System.exit(1);
        }
    }
}
