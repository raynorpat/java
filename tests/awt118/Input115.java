import java.awt.*;
import java.awt.event.*;

public class Input115 {
    static volatile int presses, releases, clicks, actions;
    static volatile boolean consume, disposeOnPress;
    public static void main(String[] args) throws Exception {
        System.getProperties().put("awt.toolkit", "sun.awt.motif.MToolkit");
        Toolkit.getDefaultToolkit();
        System.loadLibrary("motif118xprobe");
        final Frame frame = new Frame("Input");
        Button button = new Button("Input");
        frame.setLayout(null); button.setBounds(10,10,100,30);
        frame.add(button); frame.setSize(160,100);
        button.addMouseListener(new MouseAdapter() {
            public void mousePressed(MouseEvent event) {
                if (event.getX()!=10 || event.getY()!=10 || event.getWhen()<=0)
                    throw new RuntimeException("mouse coordinates/time");
                if (consume) event.consume();
                if (disposeOnPress) frame.dispose();
                presses++;
            }
            public void mouseReleased(MouseEvent event) {
                if (consume) event.consume();
                releases++;
            }
            public void mouseClicked(MouseEvent event) { clicks++; }
        });
        button.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent event) { actions++; }
        });
        frame.show(); Toolkit.getDefaultToolkit().sync();
        Window115.click(button);
        long deadline=System.currentTimeMillis()+3000;
        while ((actions<1 || clicks<1) && System.currentTimeMillis()<deadline) Thread.sleep(10);
        if (presses!=1 || releases!=1 || clicks!=1 || actions!=1)
            throw new RuntimeException("mouse delivery " + presses+"/"+releases+"/"+clicks+"/"+actions);
        consume=true;
        Window115.click(button);
        deadline=System.currentTimeMillis()+3000;
        while (clicks<2 && System.currentTimeMillis()<deadline) Thread.sleep(10);
        Thread.sleep(300);
        if (presses!=2 || releases!=2 || clicks!=2 || actions!=1)
            throw new RuntimeException("consumed mouse activated button");
        consume=false; disposeOnPress=true;
        Window115.click(button);
        deadline=System.currentTimeMillis()+3000;
        while (presses<3 && System.currentTimeMillis()<deadline) Thread.sleep(10);
        Thread.sleep(300);
        if (presses!=3 || frame.isShowing() || actions!=1)
            throw new RuntimeException("mouse replay after disposal");
        System.gc(); Toolkit.getDefaultToolkit().sync();
        frame.dispose();
        System.out.println("mouse delivery and consumption passed");
        System.exit(0);
    }
}
