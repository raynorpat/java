import java.awt.*;
import java.awt.event.*;
public class DefaultFocus115 {
    static volatile Component focused;
    static class RestrictedQueue extends SecurityManager {
        public void checkAwtEventQueueAccess() { throw new SecurityException("public event queue denied"); }
        public boolean checkTopLevelWindow(Object window) { return true; }
        public void checkExit(int status) {}
        public void checkPropertyAccess(String property) {}
        public void checkPropertiesAccess() {}
        public void checkAccess(Thread thread) {}
        public void checkAccess(ThreadGroup group) {}
    }
    static class FreshFrame extends Frame {
        FreshFrame() { super("DefaultFocus115"); }
        public Component getFocusOwner() { return null; }
    }
    static class Light extends Component {
        public boolean isFocusTraversable() { return true; }
        public Dimension getPreferredSize() { return new Dimension(80,30); }
        public void paint(Graphics g) { g.drawString("Lightweight",2,20); }
    }
    static void check(boolean lightweight) throws Exception {
        Frame frame=new FreshFrame(); frame.setLayout(new FlowLayout());
        Button disabled=new Button("Disabled"); disabled.setEnabled(false);
        Button hidden=new Button("Hidden"); hidden.setVisible(false);
        Panel nested=new Panel(); nested.add(new Label("Not traversable"));
        Component wanted=lightweight ? (Component)new Light() : (Component)new Button("Wanted");
        FocusListener observer=new FocusAdapter() {
            public void focusGained(FocusEvent event) { focused=event.getComponent(); }
        };
        disabled.addFocusListener(observer); hidden.addFocusListener(observer); wanted.addFocusListener(observer);
        nested.add(wanted); frame.add(disabled); frame.add(hidden); frame.add(nested);
        focused=null; frame.setBounds(60,70,420,180); frame.show();
        WinAwtProbe.windowState("DefaultFocus115",9); frame.toFront(); WinAwtProbe.activation("DefaultFocus115",true);
        for(int i=0;focused==null && i<200;i++) Thread.sleep(10);
        if(focused!=wanted) throw new RuntimeException("default focus selection lightweight="+lightweight+" focus="+focused);
        frame.dispose();
    }
    public static void main(String[] args) {
        try {
            check(false); check(true);
            System.setSecurityManager(new RestrictedQueue());
            check(false); check(true);
            System.out.println("default focus order, eligibility and lightweight dispatch passed"); System.exit(0);
        } catch(Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
