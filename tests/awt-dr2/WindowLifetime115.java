import java.awt.Frame;
import java.awt.Font;
import java.awt.Toolkit;

public class WindowLifetime115 {
    static { System.loadLibrary("windowlifetime115"); }
    static native void installTracker();
    static native int liveWindows();
    static native int liveViews();
    static native int visibleWindows();
    static final RuntimeException marker = new RuntimeException("construction marker");
    static class BadFont extends Font {
        BadFont() { super("Dialog", Font.PLAIN, 12); }
        public int getSize() { throw marker; }
    }
    static void checkFailedConstruction(Toolkit toolkit) throws Exception {
        Frame frame = new Frame("failed construction");
        frame.setBounds(30, 40, 200, 100);
        frame.setFont(new BadFont());
        try {
            frame.addNotify();
            throw new Error("construction unexpectedly succeeded");
        } catch (RuntimeException failure) {
            if (failure != marker) throw new Error("construction exception identity changed");
        }
        for (int retry = 0; retry < 40 && (liveWindows() != 0 || liveViews() != 0); retry++) {
            System.gc();
            Thread.sleep(50);
            toolkit.sync();
        }
        if (liveWindows() != 0 || liveViews() != 0)
            throw new Error("failed construction retained window/view: " + liveWindows() + "/" + liveViews());
        if (frame.getPeer() != null) throw new Error("failed construction installed a peer");
    }
    public static void main(String[] args) {
        try {
            installTracker();
            Toolkit toolkit = Toolkit.getDefaultToolkit();
            checkFailedConstruction(toolkit);
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
