import java.awt.*;
import java.awt.event.*;

public class List115 {
    static volatile int changes, actions;
    static native int itemCount(List list);
    static native int topIndex(List list);
    public static void main(String[] args) throws Exception {
        System.getProperties().put("awt.toolkit", "sun.awt.motif.MToolkit");
        Toolkit.getDefaultToolkit(); System.loadLibrary("motif118xprobe");
        for (int cycle=0;cycle<6;cycle++) {
            Frame frame=new Frame("List");
            List list=new List(4,true);
            for (int i=0;i<20;i++) list.add("Item "+i);
            list.addItemListener(new ItemListener() {
                public void itemStateChanged(ItemEvent event) { changes++; }
            });
            list.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent event) { actions++; }
            });
            frame.setLayout(null); list.setBounds(10,10,140,95); frame.add(list);
            frame.setSize(170,130); frame.show(); Toolkit.getDefaultToolkit().sync();
            if (itemCount(list)!=20) throw new RuntimeException("native items");
            list.select(1); list.select(3);
            int[] selected=list.getSelectedIndexes();
            if (selected.length!=2 || selected[0]!=1 || selected[1]!=3) throw new RuntimeException("multiple selection");
            list.deselect(1);
            if (list.getSelectedIndex()!=3) throw new RuntimeException("deselect");
            list.add("Inserted",2); list.remove(2);
            if (list.getItemCount()!=20 || list.getSelectedIndex()!=3) throw new RuntimeException("item mutation");
            list.makeVisible(19);
            if (topIndex(list)<=0) throw new RuntimeException("scroll to bottom");
            list.makeVisible(0);
            if (topIndex(list)!=0) throw new RuntimeException("scroll to top");
            list.setMultipleMode(false); list.deselect(3);
            int before=changes, beforeAction=actions;
            Window115.click(list);
            long deadline=System.currentTimeMillis()+3000;
            while (changes==before && System.currentTimeMillis()<deadline) Thread.sleep(10);
            if (changes==before || list.getSelectedIndex()!=0) throw new RuntimeException("list click");
            Input115.key(0xff0d,0);
            deadline=System.currentTimeMillis()+3000;
            while (actions==beforeAction && System.currentTimeMillis()<deadline) Thread.sleep(10);
            if (actions==beforeAction) throw new RuntimeException("list activation");
            list.removeAll();
            if (list.getItemCount()!=0 || itemCount(list)!=0 || list.getSelectedIndexes().length!=0)
                throw new RuntimeException("clear");
            frame.dispose(); System.gc();
        }
        System.out.println("list selection, mutation, scrolling and activation passed");
        System.exit(0);
    }
}
