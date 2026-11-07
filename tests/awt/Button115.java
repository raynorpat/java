import java.awt.*;
import java.awt.event.*;

public class Button115 {
    static volatile int actions;
    static volatile String command;
    public static void main(String[] args) {
        try {
            final Frame f = new Frame("Button115"); f.setLayout(null); f.setBounds(60,70,260,180);
            Button button = new Button("Original"); button.setBounds(20,50,180,50); f.add(button);
            button.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) { System.gc(); command = e.getActionCommand(); actions++; }
            });
            f.show();
            WinAwtProbe.click("Button115","Original");
            for (int i = 0; actions < 1 && i < 100; i++) Thread.sleep(10);
            if (actions != 1 || !"Original".equals(command)) throw new RuntimeException("initial action");
            button.setLabel("Renamed \u00e9"); button.setActionCommand("custom");
            WinAwtProbe.click("Button115","Renamed \u00e9");
            for (int i = 0; actions < 2 && i < 100; i++) Thread.sleep(10);
            if (actions != 2 || !"custom".equals(command)) throw new RuntimeException("updated action");
            button.disable(); button.enable(); button.requestFocus();
            f.dispose(); System.gc();
            System.out.println("button labels, native actions and retirement passed"); System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
