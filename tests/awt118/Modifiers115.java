import java.awt.*;
import java.awt.event.*;

public class Modifiers115 {
    static volatile int seen=-1;
    static volatile int keypadCode,keypadChar,typed;
    static volatile boolean released,letterReleased;
    static native boolean remap(boolean alternate);
    static native void key(boolean meta);
    static native void keypad(boolean locked);
    static void expectKeypad(boolean locked) throws Exception {
        keypadCode=-1; typed=0; released=false; keypad(locked);
        long end=System.currentTimeMillis()+2000;
        while(!released && System.currentTimeMillis()<end) Thread.sleep(10);
        if(!released || keypadCode!=(locked ? KeyEvent.VK_NUMPAD7 : KeyEvent.VK_HOME) ||
           (locked && keypadChar!='7') || typed!=(locked ? 1 : 0))
            throw new RuntimeException("keypad Num Lock: "+keypadCode+"/"+keypadChar+"/"+typed);
    }
    static void expect(boolean meta,int wanted) throws Exception {
        seen=-1; letterReleased=false; key(meta); long end=System.currentTimeMillis()+2000;
        while((seen<0 || !letterReleased) && System.currentTimeMillis()<end) Thread.sleep(10);
        if(seen!=wanted || !letterReleased) throw new RuntimeException("X11 modifier mapping: "+seen+" expected "+wanted);
    }
    public static void main(String[] args) {
        try {
            Toolkit.getDefaultToolkit(); System.loadLibrary("motif118xprobe");
            Frame frame=new Frame("Modifier mappings"); TextField field=new TextField();
            field.addKeyListener(new KeyAdapter() {
                public void keyPressed(KeyEvent event) {
                    if(event.getKeyCode()==KeyEvent.VK_A) seen=event.getModifiers();
                    else if(event.getKeyCode()==KeyEvent.VK_HOME || event.getKeyCode()==KeyEvent.VK_NUMPAD7)
                        { keypadCode=event.getKeyCode(); keypadChar=event.getKeyChar(); }
                    event.consume();
                }
                public void keyTyped(KeyEvent event) { typed++; }
                public void keyReleased(KeyEvent event) {
                    if(event.getKeyCode()==KeyEvent.VK_A) letterReleased=true;
                    if(event.getKeyCode()==KeyEvent.VK_HOME || event.getKeyCode()==KeyEvent.VK_NUMPAD7) released=true;
                }
            });
            frame.add(field); frame.setSize(240,90); frame.show();
            Window115.click(field); Thread.sleep(400); field.requestFocus(); Thread.sleep(100);
            for(int i=0;i<4;i++) {
                if(!remap((i&1)==0)) throw new RuntimeException("test modifier map rejected");
                Thread.sleep(300); expect(false,InputEvent.ALT_MASK); expect(true,InputEvent.META_MASK);
            }
            expectKeypad(false); expectKeypad(true); expectKeypad(false);
            frame.dispose(); System.out.println("dynamic modifier mappings and keypad Num Lock passed"); System.exit(0);
        } catch(Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
