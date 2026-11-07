import java.awt.*;

/* Run with the installed launchers and their default environment. */
public class Installed115 {
    public static void main(String[] args) {
        try {
            Toolkit toolkit = Toolkit.getDefaultToolkit();
            if (!toolkit.getClass().getName().equals("sun.awt.motif.MToolkit"))
                throw new RuntimeException("wrong installed toolkit");
            Frame frame = new Frame("Installed Motif");
            frame.add(new Button("Installed peers"));
            frame.setSize(240, 120);
            frame.show();
            if (frame.getFontMetrics(new Font("Dialog", Font.PLAIN, 12)).stringWidth("Hello") <= 0)
                throw new RuntimeException("installed font resources");
            toolkit.sync();
            frame.dispose();
            System.out.println("installed Motif passed");
            System.exit(0);
        } catch (Throwable failure) {
            failure.printStackTrace();
            System.exit(1);
        }
    }
}
