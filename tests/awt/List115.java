import java.awt.*;
import java.awt.event.*;

public class List115 {
    static volatile int items, actions;
    static volatile String command;
    static void waitEvent(boolean action) throws Exception {
        for (int i=0; (action ? actions : items) == 0 && i < 100; i++) Thread.sleep(10);
        if ((action ? actions : items) != 1) throw new RuntimeException("missing list event");
    }
    public static void main(String[] args) {
        try {
            Frame f = new Frame("List115"); f.setLayout(null); f.setBounds(50,60,320,250);
            List list = new List(4,false); list.add("zero"); list.add("one"); list.add("caf\u00e9");
            list.setBounds(20,45,240,150); f.add(list);
            list.addItemListener(new ItemListener() { public void itemStateChanged(ItemEvent e) { System.gc(); items++; } });
            list.addActionListener(new ActionListener() { public void actionPerformed(ActionEvent e) { command=e.getActionCommand(); actions++; } });
            f.show(); list.select(1);
            if (!list.isIndexSelected(1) || list.isIndexSelected(0)) throw new RuntimeException("single selection");
            list.setMultipleMode(true); list.select(0); list.select(2);
            if (!list.isIndexSelected(0) || !list.isIndexSelected(2)) throw new RuntimeException("multiple selection");
            list.deselect(0); if (list.isIndexSelected(0)) throw new RuntimeException("deselect");
            list.setMultipleMode(false); list.select(0);
            WinAwtProbe.listEvent("List115",1,false); waitEvent(false);
            if (!list.isIndexSelected(1)) throw new RuntimeException("native selection");
            WinAwtProbe.listEvent("List115",2,true); waitEvent(true);
            if (!"caf\u00e9".equals(command)) throw new RuntimeException("action command");
            list.add("insert",1); list.remove(0); list.makeVisible(2); list.removeAll(); list.add("new"); list.select(0);
            if (!list.isIndexSelected(0) || list.getPreferredSize().width <= 0) throw new RuntimeException("repopulate");
            f.dispose(); System.gc();
            System.out.println("list items, selection modes, native item/action events and retirement passed"); System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
