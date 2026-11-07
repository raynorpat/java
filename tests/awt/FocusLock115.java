import java.awt.*;
public class FocusLock115 {
    static class EmptyFrame extends Frame {
        EmptyFrame() { super("FocusLock115"); }
        public Component getFocusOwner() { return null; }
    }
    public static void main(String[] args) {
        try {
            Frame frame=new EmptyFrame(); frame.setBounds(60,70,240,160); frame.show();
            WinAwtProbe.windowState("FocusLock115",9); frame.toFront();
            synchronized(frame.getPeer()) {
                WinAwtProbe.activation("FocusLock115",true);
                Thread.sleep(200);
                frame.hide();
            }
            frame.dispose();
            System.out.println("default focus restoration while peer monitor is held passed"); System.exit(0);
        } catch(Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
