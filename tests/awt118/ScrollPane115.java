import java.awt.*;
import java.awt.event.*;

public class ScrollPane115 {
    static volatile int adjustments;
    static native Point nativePosition(ScrollPane pane);
    static native void press(ScrollPane pane,boolean vertical,boolean down);
    static void arrow(ScrollPane pane,boolean vertical) { press(pane,vertical,true); press(pane,vertical,false); }
    static class Surface extends Canvas {
        public Dimension getPreferredSize() { return new Dimension(400,300); }
    }
    public static void main(String[] args) throws Exception {
        try { run(); } catch (Throwable error) { error.printStackTrace(); System.exit(1); }
    }
    static void run() throws Exception {
        System.getProperties().put("awt.toolkit","sun.awt.motif.MToolkit");
        Toolkit.getDefaultToolkit(); System.loadLibrary("motif118xprobe");
        for (int policy=0;policy<3;policy++) {
            Frame frame=new Frame("Scroll pane");
            ScrollPane pane=new ScrollPane(policy);
            Surface child=new Surface(); pane.add(child);
            frame.setLayout(null); pane.setBounds(10,10,160,110); frame.add(pane);
            frame.setSize(190,140); frame.show(); pane.validate(); Thread.sleep(300);
            Insets insets=pane.getInsets();
            if (policy==2 ? pane.getHScrollbarHeight()!=0 || pane.getVScrollbarWidth()!=0 :
                pane.getHScrollbarHeight()<=0 || pane.getVScrollbarWidth()<=0 || insets.right<=insets.left || insets.bottom<=insets.top)
                throw new RuntimeException("scrollpane insets "+policy+"/"+insets);
            pane.setScrollPosition(45,30); Thread.sleep(300);
            Point position=nativePosition(pane);
            if (position.x!=45 || position.y!=30 || !pane.getScrollPosition().equals(position))
                throw new RuntimeException("scroll position "+policy+"/"+position+"/"+pane.getScrollPosition()+"/"+child.getSize());
            if (policy!=2) {
                pane.getHAdjustable().setUnitIncrement(7); pane.getVAdjustable().setUnitIncrement(9);
                AdjustmentListener listener=new AdjustmentListener() {
                    public void adjustmentValueChanged(AdjustmentEvent event) { adjustments++; }
                };
                pane.getHAdjustable().addAdjustmentListener(listener);
                pane.getVAdjustable().addAdjustmentListener(listener);
                int old=adjustments; arrow(pane,true);
                long end=System.currentTimeMillis()+2000;
                while (adjustments==old && System.currentTimeMillis()<end) Thread.sleep(10);
                if (adjustments==old || pane.getScrollPosition().y!=21) throw new RuntimeException("vertical callback");
                old=adjustments; arrow(pane,false); end=System.currentTimeMillis()+2000;
                while (adjustments==old && System.currentTimeMillis()<end) Thread.sleep(10);
                if (adjustments==old || pane.getScrollPosition().x!=38) throw new RuntimeException("horizontal callback");
            }
            pane.setScrollPosition(10000,10000); Thread.sleep(300);
            if (!pane.getScrollPosition().equals(nativePosition(pane))) throw new RuntimeException("scroll limits");
            pane.remove(child); pane.add(new Surface()); pane.validate();
            if (policy!=2) { press(pane,true,true); Thread.sleep(300); }
            frame.dispose(); System.gc();
            if (policy!=2) press(pane,true,false);
        }
        System.out.println("scroll pane policies, geometry, callbacks and disposal passed");
        System.exit(0);
    }
}
