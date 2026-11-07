import java.awt.*;
import java.awt.event.*;
public class InputExit115 {
    static volatile boolean exited;
    static volatile int x,y;
    public static void main(String[] args) {
        try {
            if(System.getProperty("awt.correctedBounds")!=null) {
                Frame frame=new Frame("InputExit115"); Canvas canvas=new Canvas(); frame.add(canvas); frame.setSize(220,170);
                canvas.addMouseListener(new MouseAdapter() { public void mouseExited(MouseEvent e) { x=e.getX();y=e.getY();exited=true; } });
                frame.show();
                int point=WinAwtProbe.mousePosition("InputExit115");
                WinAwtProbe.input("InputExit115",false,675,0,0,0);
                for(int i=0;!exited && i<200;i++) Thread.sleep(10);
                if(!exited || x!=(short)point || y!=(short)(point>>16)) throw new RuntimeException("exit coordinates "+x+","+y+" expected "+(short)point+","+(short)(point>>16));
                frame.dispose();
            }
            System.out.println("native mouse exit coordinates passed"); System.exit(0);
        } catch(Throwable t) { t.printStackTrace(); System.exit(1); }
    }
}
