import java.awt.*;
public class ReentrantPeers115 {
    static volatile Component armed;
    static volatile boolean retired;
    static boolean retireParent;
    static Frame frame;
    static void retire(Component component) {
        if (armed==component) {
            armed=null;
            if (retireParent) frame.dispose(); else component.removeNotify();
            System.gc(); retired=true;
        }
    }
    static class RetiringLabel extends Label {
        RetiringLabel() { super("retire during paint"); }
        public Font getFont() { Font font=super.getFont(); retire(this); return font; }
    }
    static class RetiringButton extends Button {
        RetiringButton() { super("retire during paint"); }
        public Font getFont() { Font font=super.getFont(); retire(this); return font; }
    }
    static class RetiringCheckbox extends Checkbox {
        RetiringCheckbox() { super("retire during paint"); }
        public Font getFont() { Font font=super.getFont(); retire(this); return font; }
    }
    static class RetiringChoice extends Choice {
        public Font getFont() { Font font=super.getFont(); retire(this); return font; }
        public String getItem(int index) { String value=super.getItem(index); retire(this); return value; }
    }
    static class RetiringList extends List {
        public String getItem(int index) { String value=super.getItem(index); retire(this); return value; }
    }
    public static void main(String[] args) {
        try {
            /* Safety contracts: the original binary is not used for these cases. */
            if ("true".equals(System.getProperty("awt.correctedBounds"))) {
                for (int round=0;round<2;round++) for (int kind=0;kind<6;kind++) {
                    frame=new Frame("ReentrantPeers115"); frame.setLayout(null); frame.setBounds(50,60,300,180);
                    Component control;
                    if (kind==0) control=new RetiringLabel();
                    else if (kind==1) control=new RetiringButton();
                    else if (kind==2) control=new RetiringCheckbox();
                    else if (kind==4) { RetiringList list=new RetiringList(); list.add("retire during paint"); control=list; }
                    else { RetiringChoice choice=new RetiringChoice(); choice.add("retire during paint"); control=choice; }
                    control.setBounds(20,45,230,50); frame.add(control);
                    frame.show(); WinAwtProbe.windowState("ReentrantPeers115",5);
                    retired=false; retireParent=round!=0; armed=control;
                    if (kind==5) control.setBounds(20,45,220,50);
                    else WinAwtProbe.paintAll("ReentrantPeers115");
                    for (int j=0;!retired && j<100;j++) Thread.sleep(10);
                    if (!retired || control.getPeer()!=null) throw new RuntimeException("peer retirement "+kind);
                    frame.dispose();
                }
            }
            System.out.println("reentrant paint disposal and GC passed"); System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
