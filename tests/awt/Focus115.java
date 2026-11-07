import java.awt.*;
import java.awt.event.*;
public class Focus115 {
    static volatile boolean gained, lost, temporary;
    static void check(boolean panels) throws Exception {
        Frame frame=new Frame("Focus115"); frame.setLayout(new FlowLayout());
        Button first=new Button("First"), second=new Button("Second");
        if(panels) {
            Panel a=new Panel(), b=new Panel(); a.add(first); b.add(second);
            frame.add(a); frame.add(b);
        } else { frame.add(first); frame.add(second); }
        first.addFocusListener(new FocusAdapter() {
            public void focusGained(FocusEvent e) { gained=true; }
            public void focusLost(FocusEvent e) { temporary=e.isTemporary(); lost=true; }
        });
        gained=false; lost=false;
        frame.setBounds(50,70,300,160); frame.show();
        WinAwtProbe.windowState("Focus115",9); frame.toFront();
        first.requestFocus();
        for(int i=0;!gained && i<200;i++) Thread.sleep(10);
        if(!gained) throw new RuntimeException("first focus");
        lost=false; second.requestFocus();
        for(int i=0;!lost && i<200;i++) Thread.sleep(10);
        if(!lost || !temporary) throw new RuntimeException("sibling focus temporary="+temporary+" lost="+lost+" panels="+panels);
        frame.dispose();
    }
    public static void main(String[] args) {
        try {
            check(false); check(true);
            System.out.println("legacy sibling and nested control focus transitions passed"); System.exit(0);
        } catch(Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}

