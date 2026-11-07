import java.awt.*;
public class PaneRace115 {
    static volatile boolean done;
    static volatile Throwable failure;
    public static void main(String[] args) {
        try {
            if ("true".equals(System.getProperty("awt.correctedBounds"))) {
                for (int round=0;round<30;round++) {
                    Frame frame=new Frame("PaneRace115"); frame.setLayout(null); frame.setBounds(50,60,300,220);
                    ScrollPane pane=new ScrollPane(); pane.setBounds(20,40,220,160); pane.add(new Canvas()); frame.add(pane); frame.show();
                    final Object peer=pane.getPeer(); done=false; failure=null;
                    Thread worker=new Thread(new Runnable() {
                        public void run() {
                            try {
                                while (!done) {
                                    try { WinAwtProbe.paneSpans(peer); }
                                    catch (NullPointerException expectedAfterDisposal) {}
                                    Thread.yield();
                                }
                            } catch (Throwable problem) { failure=problem; }
                        }
                    });
                    worker.start(); Thread.sleep(2); frame.dispose(); System.gc(); done=true;
                    worker.join(2000);
                    if (worker.isAlive()) throw new RuntimeException("span/dispose deadlock");
                    if (failure!=null) throw new RuntimeException("span worker: "+failure);
                }
            }
            System.out.println("scroll pane span/disposal race and GC passed"); System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
