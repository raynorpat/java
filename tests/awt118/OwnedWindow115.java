import java.awt.*;
import java.awt.event.*;

public class OwnedWindow115 {
    static volatile int clicks;
    static void check(boolean value,String message) { if (!value) throw new RuntimeException(message); }
    public static void main(String[] args) throws Exception {
        try { run(); } catch (Throwable error) { error.printStackTrace(); System.exit(1); }
    }
    static void run() throws Exception {
        System.getProperties().put("awt.toolkit","sun.awt.motif.MToolkit");
        Toolkit.getDefaultToolkit(); System.loadLibrary("motif118xprobe");
        for (int cycle=0;cycle<6;cycle++) {
            Frame owner=new Frame("Owner"); owner.setBounds(10,10,100,100); owner.show();
            final Window window=new Window(owner); window.setLayout(null);
            Button button=new Button("Close"); button.setBounds(5,5,80,30); window.add(button);
            button.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent event) { window.dispose(); clicks++; }
            });
            Canvas canvas=new Canvas(); canvas.setBounds(5,40,80,40); window.add(canvas);
            window.setBounds(140,100,100,100); window.show(); Thread.sleep(300);
            check(window.getLocationOnScreen().equals(new Point(140,100)),"owned window position");
            Graphics graphics=canvas.getGraphics(); graphics.setColor(Color.red); graphics.fillRect(0,0,80,40);
            check((Window115.pixel(canvas,5,5)&0xffffff)==0xff0000,"owned window painting");
            window.hide(); window.setBounds(160,120,110,110); window.show(); Thread.sleep(200);
            check(window.getLocationOnScreen().equals(new Point(160,120)),"owned window reshape");
            window.toBack(); window.toFront();
            int before=clicks; Window115.click(button);
            long deadline=System.currentTimeMillis()+2000;
            while (clicks==before && System.currentTimeMillis()<deadline) Thread.sleep(10);
            check(clicks==before+1,"owned window callback disposal");
            graphics.fillRect(0,0,10,10); graphics.dispose(); window.dispose(); owner.dispose(); System.gc();
        }
        System.out.println("owned window geometry, painting and disposal passed"); System.exit(0);
    }
}
