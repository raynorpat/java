import java.awt.*;
import java.awt.event.*;

public class Widgets115 {
    static volatile int changes;
    static native int state(Component component);
    static native int alignment(Label label);
    static native String label(Component component);
    static void click(Checkbox box) throws Exception {
        int before=changes;
        Window115.click(box);
        long deadline=System.currentTimeMillis()+3000;
        while (changes==before && System.currentTimeMillis()<deadline) Thread.sleep(10);
        if (changes==before) throw new RuntimeException("missing item callback");
    }
    public static void main(String[] args) throws Exception {
        System.getProperties().put("awt.toolkit", "sun.awt.motif.MToolkit");
        Toolkit.getDefaultToolkit(); System.loadLibrary("motif118xprobe");
        for (int cycle=0;cycle<8;cycle++) {
            Frame frame=new Frame("Widgets");
            frame.setLayout(null);
            Label title=new Label("Before",Label.CENTER);
            Checkbox box=new Checkbox("Check");
            CheckboxGroup group=new CheckboxGroup();
            Checkbox first=new Checkbox("First",group,true),second=new Checkbox("Second",group,false);
            title.setBounds(10,10,130,25); box.setBounds(10,40,130,25);
            first.setBounds(10,70,130,25); second.setBounds(10,100,130,25);
            frame.add(title); frame.add(box); frame.add(first); frame.add(second);
            ItemListener listener=new ItemListener() {
                public void itemStateChanged(ItemEvent event) { changes++; }
            };
            box.addItemListener(listener); first.addItemListener(listener); second.addItemListener(listener);
            frame.setSize(170,150); frame.show(); Toolkit.getDefaultToolkit().sync();
            if (!label(title).equals("Before") || alignment(title)!=1) throw new RuntimeException("initial label");
            title.setText("After"); title.setAlignment(Label.RIGHT);
            if (!label(title).equals("After") || alignment(title)!=2) throw new RuntimeException("label update");
            title.setText(null);
            if (!label(title).equals("")) throw new RuntimeException("null label");
            click(box);
            if (!box.getState() || state(box)!=1) throw new RuntimeException("checkbox state");
            box.setState(false); box.setLabel("Changed");
            if (state(box)!=0 || !label(box).equals("Changed")) throw new RuntimeException("checkbox update");
            click(second);
            if (group.getSelectedCheckbox()!=second || state(first)!=0 || state(second)!=1)
                throw new RuntimeException("radio selection");
            second.setCheckboxGroup(null);
            // JDK 1.1.5 clears the old group's selection when detaching it.
            if (second.getState() || state(second)!=0) throw new RuntimeException("group detach");
            click(second);
            if (!second.getState() || state(second)!=1) throw new RuntimeException("group removal");
            frame.dispose(); System.gc();
        }
        System.out.println("labels, checkboxes and radio groups passed");
        System.exit(0);
    }
}
