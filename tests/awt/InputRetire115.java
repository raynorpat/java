import java.awt.*;
import java.awt.event.*;
public class InputRetire115 {
    static volatile boolean retired;
    public static void main(String[] args) {
        try {
            for(int cycle=0;cycle<20;cycle++) {
                final Frame frame=new Frame("InputRetire115"); frame.setLayout(null); frame.setBounds(40,50,250,180);
                Canvas canvas=new Canvas(); canvas.setBounds(20,40,100,80); frame.add(canvas); retired=false;
                canvas.addMouseListener(new MouseAdapter() {
                    public void mousePressed(MouseEvent e) { frame.dispose(); System.gc(); retired=true; }
                });
                frame.show(); WinAwtProbe.windowState("InputRetire115",5);
                WinAwtProbe.input("InputRetire115",false,513,1,10,10);
                for(int i=0;!retired && i<200;i++) Thread.sleep(10);
                if(!retired) throw new RuntimeException("callback retirement "+cycle);
            }
            System.gc(); System.out.println("input callback disposal and pending native retirement passed"); System.exit(0);
        } catch(Throwable t) { t.printStackTrace(); System.exit(1); }
    }
}
