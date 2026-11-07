import java.awt.*;
import java.applet.Applet;

public class LatePanel115 {
    static native boolean sizeMatches(Component component);
    public static void main(String[] args) {
        try {
            Toolkit.getDefaultToolkit(); System.loadLibrary("motif118xprobe");
            for (int cycle=0; cycle<4; cycle++) {
                Frame frame=new Frame("Late applet layout");
                Panel host=new Panel(new BorderLayout());
                frame.add(host); frame.setSize(400,400); frame.show();
                Applet applet=new Applet(); applet.setLayout(new BorderLayout());
                applet.resize(400,400); host.add(applet);
                Panel child=new Panel(); child.add(new Button("Late child"));
                applet.add("Center",child); frame.validate();
                applet.setSize(500,500);
                Thread.sleep(200);
                if (!sizeMatches(applet) || !sizeMatches(child))
                    throw new RuntimeException("native late panel size differs from Java layout");
                frame.dispose();
            }
            System.out.println("late applet and nested panel geometry passed");
            System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
