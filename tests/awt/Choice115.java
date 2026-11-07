import java.awt.*;
import java.awt.event.*;

public class Choice115 {
    static volatile int events;
    static volatile String item;
    public static void main(String[] args) {
        try {
            Frame f = new Frame("Choice115"); f.setLayout(null); f.setBounds(50,60,300,220);
            Choice c = new Choice(); c.add("first"); c.add("second"); c.add("caf\u00e9");
            c.setBounds(20,45,200,40); f.add(c);
            c.addItemListener(new ItemListener() {
                public void itemStateChanged(ItemEvent e) { System.gc(); item = (String)e.getItem(); events++; }
            });
            f.show(); c.select(2);
            if (WinAwtProbe.choice("Choice115",-1) != 2) throw new RuntimeException("native selection");
            c.insert("inserted",1); c.remove(0); c.select("inserted");
            if (WinAwtProbe.choice("Choice115",-1) != 0) throw new RuntimeException("insert/remove selection");
            WinAwtProbe.choice("Choice115",2);
            for (int i=0; events < 1 && i < 100; i++) Thread.sleep(10);
            if (events != 1 || c.getSelectedIndex() != 2 || !"caf\u00e9".equals(item)) throw new RuntimeException("native item event");
            c.removeAll(); c.add("new"); c.select(0);
            if (WinAwtProbe.choice("Choice115",-1) != 0 || c.getPreferredSize().width <= 0) throw new RuntimeException("repopulate");
            f.dispose(); System.gc();
            System.out.println("choice items, native selection, item events and retirement passed"); System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
