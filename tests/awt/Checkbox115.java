import java.awt.*;
import java.awt.event.*;

public class Checkbox115 {
    static volatile int events;
    static volatile int lastState;
    static void click(String label, int count) throws Exception {
        WinAwtProbe.click("Checkbox115",label);
        for (int i = 0; events < count && i < 100; i++) Thread.sleep(10);
        if (events != count) throw new RuntimeException("missing item event");
    }
    public static void main(String[] args) {
        try {
            Frame f = new Frame("Checkbox115"); f.setLayout(null); f.setBounds(60,70,300,230);
            Checkbox one = new Checkbox("One"); one.setBounds(20,40,220,40); f.add(one);
            Checkbox two = new Checkbox("Two"); two.setBounds(20,90,220,40); f.add(two);
            ItemListener listener = new ItemListener() {
                public void itemStateChanged(ItemEvent e) { System.gc(); lastState = e.getStateChange(); events++; }
            };
            one.addItemListener(listener); two.addItemListener(listener); f.show();
            click("One",1);
            if (!one.getState() || lastState != ItemEvent.SELECTED) throw new RuntimeException("check");
            click("One",2);
            if (one.getState() || lastState != ItemEvent.DESELECTED) throw new RuntimeException("uncheck");
            CheckboxGroup group = new CheckboxGroup(); one.setCheckboxGroup(group); two.setCheckboxGroup(group);
            click("One",3); click("Two",4);
            if (one.getState() || !two.getState() || group.getSelectedCheckbox() != two) throw new RuntimeException("group");
            two.setCheckboxGroup(null);
            if (two.getState()) throw new RuntimeException("detaching must clear selection");
            two.setLabel("Changed \u00e9"); click("Changed \u00e9",5);
            if (!two.getState()) throw new RuntimeException("detached group state");
            one.setState(false); one.disable(); one.enable();
            if (one.getPreferredSize().width <= 0) throw new RuntimeException("metrics");
            f.dispose(); System.gc();
            System.out.println("checkbox state, groups, native item events and retirement passed"); System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
