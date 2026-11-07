import java.awt.*;
import java.awt.event.*;

public class Scrollbar115 {
    static volatile int changes, type;
    static native int value(Scrollbar scrollbar,int property);
    static native void press(Scrollbar scrollbar,boolean down);
    public static void main(String[] args) throws Exception {
        System.getProperties().put("awt.toolkit", "sun.awt.motif.MToolkit");
        Toolkit.getDefaultToolkit(); System.loadLibrary("motif118xprobe");
        for (int orientation=0;orientation<2;orientation++) {
            Frame frame=new Frame("Scrollbar");
            Scrollbar bar=new Scrollbar(orientation,41,10,0,1000);
            bar.setUnitIncrement(3); bar.setBlockIncrement(7);
            bar.addAdjustmentListener(new AdjustmentListener() {
                public void adjustmentValueChanged(AdjustmentEvent event) {
                    type=event.getAdjustmentType(); changes++;
                }
            });
            frame.setLayout(null);
            bar.setBounds(10,10,orientation==0 ? 140 : 18,orientation==0 ? 18 : 140);
            frame.add(bar); frame.setSize(180,180); frame.show(); Toolkit.getDefaultToolkit().sync();
            if (value(bar,0)!=41 || value(bar,1)!=10 || value(bar,2)!=3 || value(bar,3)!=7)
                throw new RuntimeException("scrollbar resources");
            int before=changes;
            Window115.click(bar);
            long deadline=System.currentTimeMillis()+3000;
            while (changes==before && System.currentTimeMillis()<deadline) Thread.sleep(10);
            if (changes==before || bar.getValue()!=38 || value(bar,0)!=38 || type!=AdjustmentEvent.UNIT_DECREMENT)
                throw new RuntimeException("scrollbar arrow");
            bar.setValues(100,20,-10,500);
            if (value(bar,0)!=100 || value(bar,1)!=20 || value(bar,4)!=-10 || value(bar,5)!=500)
                throw new RuntimeException("scrollbar range");
            press(bar,true); Thread.sleep(700); press(bar,false);
            Thread.sleep(400);
            int stopped=value(bar,0);
            Thread.sleep(400);
            if (stopped>=100 || value(bar,0)!=stopped) throw new RuntimeException("scrollbar repeat release");
            press(bar,true); Thread.sleep(100); frame.dispose(); press(bar,false);
            System.gc(); Toolkit.getDefaultToolkit().sync();
        }
        System.out.println("scrollbar range, callbacks, repeat and disposal passed");
        System.exit(0);
    }
}
