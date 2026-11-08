import java.awt.*;
import java.awt.event.*;

public class Popup115 {
    static volatile boolean selected;
    static volatile Throwable failure;
    public static void main(String[] args) {
        try {
            boolean tracked = args.length != 0;
            if (tracked) { Toolkit.getDefaultToolkit(); Menu115.track(); }
            final Frame frame = new Frame("Popup fixture"); frame.setBounds(350, 100, 300, 240);
            PopupMenu popup = new PopupMenu("Popup");
            MenuItem first = new MenuItem("First"), second = new MenuItem("Second");
            ActionListener listener = new ActionListener() {
                public void actionPerformed(ActionEvent event) {
                    try {
                        MenuItem item = (MenuItem)event.getSource();
                        if (!event.getActionCommand().equals(item.getLabel())) throw new Error("popup action command");
                        Toolkit.getDefaultToolkit().sync(); System.gc();
                        frame.dispose(); selected = true;
                    } catch (Throwable problem) { failure = problem; }
                }
            };
            first.addActionListener(listener); second.addActionListener(listener);
            popup.add(first); popup.add(second); frame.add(popup); frame.show();
            Menu115.popupKeys(frame); popup.show(frame, 40, 70);
            for (int wait = 0; wait < 500 && !selected && failure == null; wait++) Thread.sleep(10);
            if (failure != null) throw new Error(failure.toString());
            if (!selected) throw new Error("missing popup action");
            if (tracked) {
                Frame pending = new Frame("Pending popup"); pending.setBounds(350, 100, 300, 240);
                PopupMenu cancelled = new PopupMenu("Cancelled"); cancelled.add("Item");
                pending.add(cancelled); pending.show(); cancelled.show(pending, 40, 70); pending.dispose();
                for (int wait = 0; wait < 100 && Menu115.live() != 0; wait++) {
                    Thread.sleep(50); Toolkit.getDefaultToolkit().sync(); System.gc();
                }
                if (Menu115.live() != 0) throw new Error("native popup retained");
            }
            System.out.println("DR2 popup menus deliver native actions and survive callback disposal");
            System.exit(0);
        } catch (Throwable problem) { problem.printStackTrace(); System.exit(1); }
    }
}
