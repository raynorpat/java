import java.awt.*;
import java.awt.event.*;

public class Scrollbar115 {
    static volatile int events, type, value;
    static void scroll(int code,int position,int count,int expectedType,int expectedValue) throws Exception {
        WinAwtProbe.scroll("Scrollbar115",code,position);
        for (int i=0;events<count && i<100;i++) Thread.sleep(10);
        if (events != count || type != expectedType || value != expectedValue)
            throw new RuntimeException("adjustment " + events + ":" + type + ":" + value);
    }
    public static void main(String[] args) {
        try {
            for (int orientation=0;orientation<2;orientation++) {
                Frame f=new Frame("Scrollbar115"); f.setLayout(null); f.setBounds(50,60,300,240);
                Scrollbar bar=new Scrollbar(orientation,30,10,0,100);
                bar.setBounds(20,45,orientation==0?220:20,orientation==0?20:150); f.add(bar);
                bar.setUnitIncrement(3); bar.setBlockIncrement(12); events=0;
                bar.addAdjustmentListener(new AdjustmentListener() {
                    public void adjustmentValueChanged(AdjustmentEvent e) { System.gc(); type=e.getAdjustmentType(); value=e.getValue(); events++; }
                });
                f.show();
                scroll(0,0,1,AdjustmentEvent.UNIT_DECREMENT,27);
                scroll(1,0,2,AdjustmentEvent.UNIT_INCREMENT,30);
                scroll(2,0,3,AdjustmentEvent.BLOCK_DECREMENT,18);
                scroll(3,0,4,AdjustmentEvent.BLOCK_INCREMENT,30);
                scroll(5,65,5,AdjustmentEvent.TRACK,65);
                bar.setValues(20,5,10,50); bar.setUnitIncrement(4);
                scroll(1,0,6,AdjustmentEvent.UNIT_INCREMENT,24);
                if (bar.getValue()!=24) throw new RuntimeException("target value");
                f.dispose();
            }
            System.out.println("scrollbar ranges, increments and native adjustment events passed"); System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
