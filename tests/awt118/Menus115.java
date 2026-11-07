import java.awt.*;
import java.awt.event.*;

public class Menus115 {
    static native void click(MenuComponent item);
    static volatile int actions,changes;
    static void check(boolean value,String message) { if (!value) throw new RuntimeException(message); }
    public static void main(String[] args) throws Exception {
        try { run(); } catch (Throwable error) { error.printStackTrace(); System.exit(1); }
    }
    static void run() throws Exception {
        System.getProperties().put("awt.toolkit","sun.awt.motif.MToolkit");
        Toolkit.getDefaultToolkit(); System.loadLibrary("motif118xprobe");
        for (int cycle=0;cycle<4;cycle++) {
            final Frame frame=new Frame("Menus");
            MenuBar bar=new MenuBar(); Menu menu=new Menu("File");
            MenuItem item=new MenuItem("Action"); CheckboxMenuItem toggle=new CheckboxMenuItem("Toggle");
            item.addActionListener(new ActionListener() { public void actionPerformed(ActionEvent event) { actions++; } });
            toggle.addItemListener(new ItemListener() { public void itemStateChanged(ItemEvent event) { changes++; } });
            menu.add(item); menu.addSeparator(); menu.add(toggle);
            Menu nested=new Menu("More"); nested.add(new MenuItem("Child")); menu.add(nested);
            bar.add(menu); frame.setMenuBar(bar);
            Canvas content=new Canvas(); frame.add(content); frame.setSize(240,160); frame.show(); Thread.sleep(300);
            check(frame.getInsets().top>0,"menu bar inset");
            check(content.getLocationOnScreen().y-frame.getLocationOnScreen().y==frame.getInsets().top,
                "menu client origin "+content.getLocationOnScreen()+" frame "+frame.getLocationOnScreen()+" insets "+frame.getInsets());
            int before=actions; click(menu); Thread.sleep(150); click(item); Thread.sleep(400);
            check(actions==before+1,"menu activation");
            before=changes; click(menu); Thread.sleep(150); click(toggle); Thread.sleep(400);
            check(changes==before+1 && toggle.getState(),"checkbox menu activation");
            PopupMenu popup=new PopupMenu("Popup"); MenuItem popupItem=new MenuItem("Popup action");
            popupItem.addActionListener(new ActionListener() { public void actionPerformed(ActionEvent event) { actions++; } });
            popup.add(popupItem); content.add(popup); before=actions;
            popup.show(content,20,20); Thread.sleep(150); click(popupItem); Thread.sleep(400);
            check(actions==before+1,"popup activation");
            popup.show(content,30,25); Thread.sleep(100); content.remove(popup);
            toggle.setState(false); item.setLabel("Renamed"); item.setEnabled(false);
            menu.remove(item); menu.add(new MenuItem("Replacement"));
            frame.setMenuBar(null); check(frame.getInsets().top==0,"menu bar detach inset");
            frame.setMenuBar(bar);
            MenuItem close=new MenuItem("Close");
            close.addActionListener(new ActionListener() { public void actionPerformed(ActionEvent event) { frame.dispose(); actions++; } });
            menu.add(close); before=actions; click(menu); Thread.sleep(150); click(close); Thread.sleep(400);
            check(actions==before+1 && !frame.isShowing(),"menu callback disposal");
            frame.dispose(); System.gc();
        }
        System.out.println("menu activation, mutation and disposal passed"); System.exit(0);
    }
}
