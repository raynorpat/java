import java.awt.*;
import java.awt.event.*;

public class Choice115 {
    static volatile int changes;
    static native int selected(Choice choice);
    static native int itemCount(Choice choice);
    static native boolean colorsMatch(Choice choice);
    public static void main(String[] args) throws Exception {
        System.getProperties().put("awt.toolkit", "sun.awt.motif.MToolkit");
        Toolkit.getDefaultToolkit(); System.loadLibrary("motif118xprobe");
        Frame live=new Frame("Late choice");
        Panel panel=new Panel(); live.add(panel); live.setSize(200,120); live.show();
        Choice late=new Choice(); late.add("Lines"); late.add("Points");
        panel.add(late); live.validate();
        Thread.sleep(100); live.dispose();
        for (int cycle=0;cycle<6;cycle++) {
            Frame frame=new Frame("Choice");
            Choice choice=new Choice();
            choice.add("First"); choice.add("Second"); choice.add("Third");
            choice.addItemListener(new ItemListener() {
                public void itemStateChanged(ItemEvent event) { changes++; }
            });
            frame.setLayout(null); frame.add(choice);
            frame.addNotify();
            choice.setBounds(10,10,130,30);
            frame.setSize(170,100); frame.show(); Toolkit.getDefaultToolkit().sync();
            choice.setBounds(0,0,0,0);
            choice.setBounds(10,10,130,30);
            choice.select(1);
            if (selected(choice)!=1 || itemCount(choice)!=3) throw new RuntimeException("choice select");
            choice.remove(0);
            if (selected(choice)!=0 || itemCount(choice)!=2) throw new RuntimeException("choice removal");
            choice.setFont(new Font("Dialog",Font.BOLD,14));
            choice.setBackground(Color.yellow); choice.setForeground(Color.blue);
            int before=changes;
            Window115.click(choice);
            Thread.sleep(400);
            Input115.key(0xff54,0); Input115.key(0xff0d,0);
            long deadline=System.currentTimeMillis()+3000;
            while (changes==before && System.currentTimeMillis()<deadline) Thread.sleep(10);
            if (changes==before || choice.getSelectedIndex()!=1 || selected(choice)!=1)
                throw new RuntimeException("choice popup selection");
            choice.removeAll();
            if (itemCount(choice)!=0) throw new RuntimeException("choice clear");
            for (int i=0;i<50;i++) choice.add("Item "+i);
            choice.select(49);
            if (itemCount(choice)!=50 || selected(choice)!=49) throw new RuntimeException("choice growth");
            if (!colorsMatch(choice)) throw new RuntimeException("new item colors");
            frame.dispose(); System.gc();
        }
        System.out.println("choice selection, popup, mutation and disposal passed");
        System.exit(0);
    }
}
