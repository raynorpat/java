import java.awt.*;
import java.awt.event.*;
public class Input115 {
    static volatile int pressed,released,clicked,typed,down,up,changes;
    static volatile String error;
    public static void main(String[] args) {
        try {
            Frame frame=new Frame("Input115"); frame.setLayout(null); frame.setSize(300,220);
            Canvas canvas=new Canvas(); canvas.setBounds(15,40,120,80); frame.add(canvas);
            TextField text=new TextField(); text.setBounds(15,135,180,30); frame.add(text);
            canvas.addMouseListener(new MouseAdapter() {
                public void mousePressed(MouseEvent e) { pressed++; if(e.getX()!=11 || e.getY()!=12 || e.getClickCount()!=1) error="mouse coordinates/count"; System.gc(); }
                public void mouseReleased(MouseEvent e) { released++; }
                public void mouseClicked(MouseEvent e) { clicked++; }
            });
            text.addTextListener(new TextListener() { public void textValueChanged(TextEvent e) { changes++; } });
            text.addKeyListener(new KeyAdapter() {
                public void keyPressed(KeyEvent e) { down++; }
                public void keyReleased(KeyEvent e) { up++; }
                public void keyTyped(KeyEvent e) { typed++; if(e.getKeyChar()=='x') e.consume(); System.gc(); }
            });
            frame.show(); WinAwtProbe.windowState("Input115",5); Thread.sleep(100);
            WinAwtProbe.input("Input115",false,513,1,11,12);
            WinAwtProbe.input("Input115",false,514,0,11,12);
            for(int i=0;clicked==0 && i<200;i++) Thread.sleep(10);
            if(pressed!=1 || released!=1 || clicked!=1 || error!=null) throw new RuntimeException("mouse delivery "+pressed+":"+released+":"+clicked+" "+error);
            text.requestFocus(); Thread.sleep(100);
            WinAwtProbe.input("Input115",true,258,'x',0,0);
            for(int i=0;typed==0 && i<200;i++) Thread.sleep(10);
            Thread.sleep(100);
            if(typed!=1 || !text.getText().equals("")) throw new RuntimeException("consumed typed input "+typed+"/"+text.getText());
            WinAwtProbe.input("Input115",true,258,233,0,0);
            for(int i=0;changes<1 && i<200;i++) Thread.sleep(10);
            if(!text.getText().equals("\u00e9")) throw new RuntimeException("non-ASCII native replay "+text.getText());
            text.dispatchEvent(new KeyEvent(text,400,System.currentTimeMillis(),0,0,'z'));
            for(int i=0;changes<2 && i<200;i++) Thread.sleep(10);
            if(!text.getText().equals("\u00e9z")) throw new RuntimeException("synthetic typed replay "+text.getText());
            WinAwtProbe.input("Input115",true,256,37,0,0);
            WinAwtProbe.input("Input115",true,257,37,0,0);
            for(int i=0;(down==0 || up==0) && i<200;i++) Thread.sleep(10);
            if(down!=1 || up!=1) throw new RuntimeException("key down/up "+down+":"+up);
            frame.dispose(); System.out.println("mouse, consumed keys, Unicode and synthetic input passed"); System.exit(0);
        } catch(Throwable t) { t.printStackTrace(); System.exit(1); }
    }
}
