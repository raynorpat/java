package sun.awt.motif;
import java.awt.*;

public class InputThread115 {
    static volatile int paints;
    public static void main(String[] args) throws Exception {
        try {
            Toolkit.getDefaultToolkit();
            InputThread input=new InputThread();
            Frame frame=new Frame();
            Canvas canvas=new Canvas(){public void paint(Graphics graphics){graphics.setColor(Color.blue);graphics.fillRect(0,0,40,40);paints++;}};
            frame.add(canvas); frame.setSize(120,100); frame.show();
            for(int cycle=0;cycle<4;cycle++) {
                int previous=paints; canvas.repaint(); long end=System.currentTimeMillis()+2000;
                while(paints==previous && System.currentTimeMillis()<end) Thread.sleep(10);
                if(paints==previous || !input.isAlive()) throw new RuntimeException("input readiness handshake");
                frame.hide(); frame.show();
            }
            frame.dispose();
            System.out.println("input readiness notification and dispatch passed"); System.exit(0);
        } catch(Throwable error) { error.printStackTrace(); System.exit(1); }
    }
}
