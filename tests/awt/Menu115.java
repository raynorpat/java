import java.awt.*;
import java.awt.event.*;

public class Menu115 {
    static volatile int actions, changes;
    static volatile String command;
    static volatile MenuItem retiring;
    static volatile boolean retired;
    static Frame retiringFrame;
    public static class MenuToolkit extends sun.awt.windows.WToolkit {
        public FontMetrics getFontMetrics(Font font) {
            FontMetrics metrics=super.getFontMetrics(font);
            if (retiring!=null) {
                MenuItem item=retiring; retiring=null;
                if (retiringFrame!=null) retiringFrame.dispose(); else item.removeNotify();
                System.gc(); retired=true;
            }
            return metrics;
        }
    }
    static void check(boolean condition,String message) { if (!condition) throw new RuntimeException(message); }
    static void waitActions(int expected) throws Exception {
        for (int i=0; actions<expected && i<100; i++) Thread.sleep(10);
        check(actions==expected,"menu actions "+actions+" expected "+expected);
    }
    public static void main(String[] args) {
        try {
            System.getProperties().put("awt.toolkit","Menu115$MenuToolkit");
            MenuBar detached=new MenuBar(); detached.addNotify(); detached.removeNotify();
            final Frame f=new Frame("Menu115"); f.setBounds(70,80,320,220);
            MenuBar bar=new MenuBar(); final Menu menu=new Menu("File \u00e9");
            MenuItem action=new MenuItem("Run \u4e2d",new MenuShortcut(KeyEvent.VK_R));
            action.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) { System.gc(); command=e.getActionCommand(); actions++; }
            });
            menu.add(action); menu.addSeparator(); final CheckboxMenuItem checked=new CheckboxMenuItem("Checked",true);
            checked.addItemListener(new ItemListener() {
                public void itemStateChanged(ItemEvent e) { System.gc(); changes++; }
            });
            menu.add(checked); Menu nested=new Menu("Nested"); nested.add(new MenuItem("Inside")); menu.add(nested);
            bar.add(menu); f.setMenuBar(bar); f.show();
            check(WinAwtMenuProbe.exports()==15,"menu export subset");
            check(WinAwtMenuProbe.query("Menu115",-1,0,-1,0)==1,"menubar count");
            check(WinAwtMenuProbe.query("Menu115",0,0,-1,0)==4,"item count");
            check((WinAwtMenuProbe.query("Menu115",0,1,-1,1)&0x800)!=0,"separator");
            check(WinAwtMenuProbe.query("Menu115",0,0,-1,2)==1,"owner drawn item");
            int shortcutWidth=WinAwtMenuProbe.query("Menu115",0,0,-1,3);
            check(shortcutWidth>0,"menu font measurement");
            check(WinAwtMenuProbe.query("Menu115",0,0,-1,4)>0,"Unicode owner drawing");
            System.out.println("checked menu ink "+WinAwtMenuProbe.query("Menu115",0,2,-1,4));
            action.deleteShortcut();
            check(WinAwtMenuProbe.query("Menu115",0,0,-1,3)<shortcutWidth,"shortcut measurement");
            int height=WinAwtMenuProbe.query("Menu115",0,0,-1,5);
            action.setFont(new Font("Dialog",Font.BOLD,24));
            check(WinAwtMenuProbe.query("Menu115",0,0,-1,5)>height,"explicit menu font");
            check(WinAwtMenuProbe.query("Menu115",0,3,0,2)==1,"nested owner drawn item");
            try { action.setLabel(null); throw new RuntimeException("null menu label accepted"); }
            catch (NullPointerException expected) {}
            action.setLabel("Changed \u00e9"); action.setActionCommand("custom"); action.disable();
            check((WinAwtMenuProbe.query("Menu115",0,0,-1,1)&3)!=0,"disabled item"); action.enable();
            WinAwtMenuProbe.activate("Menu115",0,0,-1); waitActions(1); check("custom".equals(command),"command");
            checked.setState(false); check((WinAwtMenuProbe.query("Menu115",0,2,-1,1)&8)==0,"unchecked");
            WinAwtMenuProbe.activate("Menu115",0,2,-1);
            for (int i=0; changes<1 && i<100; i++) Thread.sleep(10);
            check(changes==1 && checked.getState(),"checkbox callback");
            menu.remove(0); check(WinAwtMenuProbe.query("Menu115",0,0,-1,0)==3,"remove item");
            menu.add(action); check(WinAwtMenuProbe.query("Menu115",0,0,-1,0)==4,"recreate item");
            WinAwtMenuProbe.activate("Menu115",0,3,-1); waitActions(2);
            if ("true".equals(System.getProperty("awt.correctedBounds"))) {
                WinAwtMenuProbe.queueAndRetire("Menu115",0,3,action);
                check(action.getPeer()==null,"queued command retires peer");
                Thread.sleep(100); check(actions==2,"retired command suppressed");
                menu.remove(3); menu.add(action);
            }
            final PopupMenu popup=new PopupMenu("Popup"); MenuItem pop=new MenuItem("Popup action");
            pop.addActionListener(new ActionListener() { public void actionPerformed(ActionEvent e) { System.gc(); actions++; } });
            popup.add(pop); final Canvas canvas=new Canvas(); f.add(canvas); f.validate(); canvas.add(popup);
            Thread show=new Thread(new Runnable() { public void run() { popup.show(canvas,20,20); } }); show.start();
            boolean clicked=false; for (int i=0; !clicked && i<100; i++) { Thread.sleep(10); clicked=WinAwtMenuProbe.popup("Menu115"); }
            check(clicked,"popup visible/action"); show.join(2000); check(!show.isAlive(),"popup closes"); waitActions(3);
            bar.remove(menu); check(WinAwtMenuProbe.query("Menu115",-1,0,-1,0)==0,"remove menu");
            bar.add(menu); f.setMenuBar(null); check(WinAwtMenuProbe.query("Menu115",-1,0,-1,0)==0,"detach menubar");
            f.setMenuBar(bar); f.dispose(); System.gc();
            if ("true".equals(System.getProperty("awt.correctedBounds"))) {
                for (int round=0;round<3;round++) {
                    Frame frame=new Frame("MenuRetire115"); frame.setBounds(90,100,300,180);
                    MenuBar mb=new MenuBar(); Menu m=new Menu("Retire"); MenuItem item=new MenuItem("Retiring item");
                    m.add(item); mb.add(m);
                    if (round==2) {
                        frame.show(); retired=false; retiringFrame=frame; retiring=item;
                        try { frame.setMenuBar(mb); }
                        catch (NullPointerException disposedFrame) { check(retired,"unexpected attachment failure"); }
                        for (int i=0;!retired && i<100;i++) Thread.sleep(10);
                    } else {
                        frame.setMenuBar(mb); frame.show();
                        retired=false; retiringFrame=round==0 ? null:frame; retiring=item;
                        WinAwtMenuProbe.query("MenuRetire115",0,0,-1,3);
                    }
                    check(retired && item.getPeer()==null,"measurement disposal and GC"); frame.dispose();
                }
            }
            System.out.println("menu mutation, owner drawing, nested actions, checkbox, popup and retirement passed"); System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
