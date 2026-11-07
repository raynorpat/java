import java.awt.*;
import java.awt.event.*;

public class Input115 {
    static volatile int presses, releases, clicks, actions;
    static volatile boolean consume, disposeOnPress;
    static StringBuffer keys=new StringBuffer();
    static volatile int focus;
    static volatile boolean consumeKeys;
    static volatile int lastModifiers;
    static native void key(int keysym,int modifiers);
    static void record(KeyEvent event,String kind) {
        int code=event.getKeyCode();
        if (code==16 || code==17 || code==18 || code==157) return;
        if (consumeKeys) event.consume();
        lastModifiers=event.getModifiers();
        keys.append(kind+event.getKeyChar()+code+",");
    }
    static void checkKey(int symbol,int modifiers,String expected) throws Exception {
        keys.setLength(0);
        key(symbol,modifiers);
        long deadline=System.currentTimeMillis()+3000;
        while (keys.length()<expected.length() && System.currentTimeMillis()<deadline) Thread.sleep(10);
        if (!keys.toString().equals(expected) || lastModifiers!=modifiers)
            throw new RuntimeException("keyboard delivery " + keys+"/"+lastModifiers);
    }
    public static void main(String[] args) throws Exception {
        System.getProperties().put("awt.toolkit", "sun.awt.motif.MToolkit");
        Toolkit.getDefaultToolkit();
        System.loadLibrary("motif118xprobe");
        final Frame frame = new Frame("Input");
        Button button = new Button("Input");
        frame.setLayout(null); button.setBounds(10,10,100,30);
        frame.add(button); frame.setSize(160,100);
        button.addFocusListener(new FocusAdapter() {
            public void focusGained(FocusEvent event) { focus++; }
        });
        button.addKeyListener(new KeyAdapter() {
            public void keyPressed(KeyEvent event) { record(event,"P"); }
            public void keyTyped(KeyEvent event) { record(event,"T"); }
            public void keyReleased(KeyEvent event) { record(event,"R"); }
        });
        button.addMouseListener(new MouseAdapter() {
            public void mousePressed(MouseEvent event) {
                if (event.getX()!=10 || event.getY()!=10 || event.getWhen()<=0)
                    throw new RuntimeException("mouse coordinates/time");
                if (consume) event.consume();
                if (disposeOnPress) frame.dispose();
                presses++;
            }
            public void mouseReleased(MouseEvent event) {
                if (consume) event.consume();
                releases++;
            }
            public void mouseClicked(MouseEvent event) { clicks++; }
        });
        button.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent event) { actions++; }
        });
        frame.show(); Toolkit.getDefaultToolkit().sync();
        Point origin=frame.getLocationOnScreen(), position=button.getLocationOnScreen();
        if (position.x-origin.x!=10 || position.y-origin.y!=10)
            throw new RuntimeException("screen coordinates");
        button.requestFocus();
        Window115.click(button);
        long deadline=System.currentTimeMillis()+3000;
        while ((actions<1 || clicks<1) && System.currentTimeMillis()<deadline) Thread.sleep(10);
        if (presses!=1 || releases!=1 || clicks!=1 || actions!=1)
            throw new RuntimeException("mouse delivery " + presses+"/"+releases+"/"+clicks+"/"+actions);
        checkKey(97,0,"Pa65,Ta0,Ra65,");
        if (focus<1) throw new RuntimeException("missing focus event");
        checkKey(97,1,"PA65,TA0,RA65,");
        checkKey(97,2,"P"+(char)1+"65,T"+(char)1+"0,R"+(char)1+"65,");
        checkKey(0xff51,0,"P"+(char)0+"37,R"+(char)0+"37,");
        consumeKeys=true;
        checkKey(32,0,"P 32,T 0,R 32,");
        Thread.sleep(300);
        if (actions!=1) throw new RuntimeException("consumed key activated button");
        consumeKeys=false;
        consume=true;
        Window115.click(button);
        deadline=System.currentTimeMillis()+3000;
        while (clicks<2 && System.currentTimeMillis()<deadline) Thread.sleep(10);
        Thread.sleep(300);
        if (presses!=2 || releases!=2 || clicks!=2 || actions!=1)
            throw new RuntimeException("consumed mouse activated button");
        consume=false; disposeOnPress=true;
        Window115.click(button);
        deadline=System.currentTimeMillis()+3000;
        while (presses<3 && System.currentTimeMillis()<deadline) Thread.sleep(10);
        Thread.sleep(300);
        if (presses!=3 || frame.isShowing() || actions!=1)
            throw new RuntimeException("mouse replay after disposal");
        System.gc(); Toolkit.getDefaultToolkit().sync();
        frame.dispose();
        System.out.println("mouse, keyboard, focus and consumption passed");
        System.exit(0);
    }
}
