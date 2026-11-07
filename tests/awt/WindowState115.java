import java.awt.*;
import java.awt.event.*;
public class WindowState115 {
    static volatile int iconified, restored, activated, deactivated;
    public static void main(String[] args) {
        try {
            Frame first=new Frame("WindowState115"), second=new Frame("WindowState115 other");
            first.setBounds(60,70,250,180); second.setBounds(340,70,250,180);
            first.add(new Button("Focus"));
            first.addWindowListener(new WindowAdapter() {
                public void windowIconified(WindowEvent e) { System.gc(); iconified++; }
                public void windowDeiconified(WindowEvent e) { restored++; }
                public void windowActivated(WindowEvent e) { activated++; }
                public void windowDeactivated(WindowEvent e) { deactivated++; }
            });
            first.show(); second.show(); WinAwtProbe.windowState("WindowState115",9); first.toFront();
            for (int i=0;activated==0 && i<100;i++) Thread.sleep(10);
            if (activated==0) throw new RuntimeException("activation");
            WinAwtProbe.windowState("WindowState115",6);
            for (int i=0;iconified==0 && i<100;i++) Thread.sleep(10);
            if (iconified==0) throw new RuntimeException("iconification");
            WinAwtProbe.windowState("WindowState115",9);
            for (int i=0;restored==0 && i<100;i++) Thread.sleep(10);
            if (restored==0) throw new RuntimeException("restoration");
            second.toFront();
            for (int i=0;deactivated==0 && i<100;i++) Thread.sleep(10);
            if (deactivated==0) throw new RuntimeException("deactivation");
            second.dispose(); first.dispose();
            System.out.println("window activation, iconification, restoration and retirement passed"); System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
