import java.awt.*;
import java.awt.event.*;
public class MotionLifetime115 {
    static volatile int lastX,lastY;
    static volatile boolean entered,blocked;
    static final Object gate=new Object();
    static void bursts(int first,int last,boolean drag) throws Exception {
        synchronized(gate) { blocked=true; entered=false; }
        WinAwtProbe.motionBurst("MotionLifetime115",first,drag);
        for(int i=0;!entered && i<200;i++) Thread.sleep(10);
        if(!entered) throw new RuntimeException("motion handler did not block");
        for(int cycle=first+1;cycle<=last;cycle++) { WinAwtProbe.motionBurst("MotionLifetime115",cycle,drag); Thread.sleep(20); }
        Thread.sleep(100);
        synchronized(gate) { blocked=false; gate.notifyAll(); }
        for(int i=0;(lastX!=999 || lastY!=last) && i<500;i++) Thread.sleep(10);
        if(lastX!=999 || lastY!=last) throw new RuntimeException("motion delivery stalled");
    }
    static void motion(MouseEvent e) {
        synchronized(gate) { entered=true; while(blocked) try { gate.wait(); } catch(InterruptedException ignored) {} }
        lastX=e.getX(); lastY=e.getY();
    }
    public static void main(String[] args) {
        try {
            if(System.getProperty("awt.correctedBounds")!=null) {
                Frame frame=new Frame("MotionLifetime115"); Canvas canvas=new Canvas(); frame.add(canvas); frame.setSize(220,170);
                canvas.addMouseMotionListener(new MouseMotionAdapter() {
                    public void mouseMoved(MouseEvent e) { motion(e); }
                    public void mouseDragged(MouseEvent e) { motion(e); }
                });
                frame.show(); bursts(1,5,false);
                System.gc(); Thread.sleep(100); int before=WinAwtProbe.heapBytes();
                bursts(6,25,false); bursts(26,45,true);
                System.gc(); Thread.sleep(100); int growth=WinAwtProbe.heapBytes()-before;
                if(growth>512*1024) throw new RuntimeException("coalesced motion retained native memory: "+growth);
                frame.dispose();
            }
            System.out.println("coalesced motion has bounded native lifetime passed"); System.exit(0);
        } catch(Throwable t) { t.printStackTrace(); System.exit(1); }
    }
}
