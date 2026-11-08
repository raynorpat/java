import java.awt.*;
import java.awt.event.*;

public class KeyReplay115 {
    static void expect(TextField field,String wanted) throws Exception {
        long end=System.currentTimeMillis()+2000;
        while(!field.getText().equals(wanted) && System.currentTimeMillis()<end) Thread.sleep(10);
        if(!field.getText().equals(wanted)) throw new RuntimeException("edited key replay: "+field.getText()+" expected "+wanted);
    }
    public static void main(String[] args) {
        try {
            Toolkit.getDefaultToolkit(); System.loadLibrary("motif118xprobe");
            for(int i=0;i<3;i++) {
                final Frame frame=new Frame("Edited key replay");
                TextField field=new TextField(); frame.add(field); frame.setSize(240,90); frame.show();
                field.addKeyListener(new KeyAdapter() {
                    public void keyPressed(KeyEvent event) {
                        if(event.getKeyChar()=='a') { event.setKeyChar('b'); event.setKeyCode(KeyEvent.VK_B); }
                        if(event.getKeyChar()=='c') { event.setKeyChar('D'); event.setKeyCode(KeyEvent.VK_D); }
                        if(event.getKeyChar()=='e') event.consume();
                        if(event.getKeyChar()=='f') frame.dispose();
                    }
                });
                Window115.click(field); Thread.sleep(400); field.requestFocus(); Thread.sleep(100);
                Input115.key('a',0); expect(field,"b");
                Input115.key('c',0); expect(field,"bD");
                Input115.key('e',0); Thread.sleep(300); expect(field,"bD");
                Input115.key('f',0); Thread.sleep(300);
                if(frame.isShowing()) throw new RuntimeException("key callback did not dispose frame");
            }
            System.out.println("edited key replay, consumption and disposal passed"); System.exit(0);
        } catch(Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
