import java.awt.Frame;
import java.awt.Font;
import java.awt.Toolkit;

public class WindowLifetime115 {
    static { System.loadLibrary("windowlifetime115"); }
    static native void installTracker();
    static native int liveWindows();
    static native int liveViews();
    static native int liveThrowables();
    static native void failAt(int stage);
    static native int visibleWindows();
    static final RuntimeException marker = new RuntimeException("construction marker");
    static class BadFont extends Font {
        BadFont() { super("Dialog", Font.PLAIN, 12); }
        public int getSize() { throw marker; }
    }
    static class BadBoundsFrame extends Frame {
        boolean fail;
        public void setBounds(int x, int y, int width, int height) {
            if (fail) throw marker;
            super.setBounds(x, y, width, height);
        }
    }
    static void checkFailedConstruction(Toolkit toolkit, int stage) throws Exception {
        Frame frame;
        if (stage == 3) {
            BadBoundsFrame bad = new BadBoundsFrame();
            bad.setBounds(30, 40, 10, 10);
            bad.fail = true;
            frame = bad;
        } else {
            frame = new Frame("failed construction");
            frame.setBounds(30, 40, 200, 100);
            if (stage == 0) frame.setFont(new BadFont());
        }
        failAt(stage);
        try {
            frame.addNotify();
            throw new Error("construction unexpectedly succeeded");
        } catch (RuntimeException failure) {
            if (stage == 0 || stage == 3) {
                if (failure != marker) throw new Error("construction exception identity changed");
            } else if (failure.getMessage().indexOf("construction failure") < 0) {
                throw new Error("native constructor exception changed: " + failure);
            }
        } finally {
            failAt(0);
        }
        for (int retry = 0; retry < 40 && (liveWindows() != 0 || liveViews() != 0 || liveThrowables() != 0); retry++) {
            System.gc();
            Thread.sleep(50);
            toolkit.sync();
        }
        if (liveWindows() != 0 || liveViews() != 0)
            throw new Error("failed construction retained window/view: " + liveWindows() + "/" + liveViews());
        if (liveThrowables() != 0) throw new Error("failed construction retained throwable owner: " + liveThrowables());
        if (frame.getPeer() != null) throw new Error("failed construction installed a peer");
    }
    public static void main(String[] args) {
        try {
            installTracker();
            Toolkit toolkit = Toolkit.getDefaultToolkit();
            for (int failure = 0; failure < 5; failure++) {
                for (int stage = 0; stage < 4; stage++) checkFailedConstruction(toolkit, stage);
            }
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
                for (int retry = 0; retry < 40 && (liveWindows() != 0 || liveViews() != 0 || liveThrowables() != 0); retry++) {
                    System.gc();
                    Thread.sleep(50);
                    toolkit.sync();
                }
                if (liveWindows() != 0) throw new Error("native window retained after disposal: " + liveWindows());
                if (liveViews() != 0) throw new Error("native view retained after disposal: " + liveViews());
                if (visibleWindows() != 0) throw new Error("disposed native window still visible");
                if (frame.getPeer() != null) throw new Error("Java peer not disposed");
            }
            System.out.println("DR2 Frame failures preserve exceptions and release resources across five lifetimes");
            System.exit(0);
        } catch (Throwable failure) {
            failure.printStackTrace();
            System.exit(1);
        }
    }
}
