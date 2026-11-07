import java.awt.*;
import java.awt.event.*;
import sun.awt.motif.X11InputMethod;

public class InputMethod115 {
    static native void compose();
    static class Method extends X11InputMethod {
        Component client;
        Method(Component value) throws AWTException { client=value; }
        protected Component getClientComponent() { return client; }
    }
    static StringBuffer typed=new StringBuffer();
    public static void main(String[] args) throws Exception {
        try {
            Toolkit.getDefaultToolkit(); System.loadLibrary("motif118xprobe");
            for(int cycle=0;cycle<4;cycle++) {
                Frame frame=new Frame(); Canvas canvas=new Canvas(); frame.add(canvas); frame.setSize(160,100); frame.show();
                canvas.addKeyListener(new KeyAdapter(){public void keyTyped(KeyEvent event){typed.append(event.getKeyChar());}});
                Method method=new Method(canvas); canvas.requestFocus(); Window115.click(canvas); Thread.sleep(150); method.activate();
                typed.setLength(0); Input115.key('a',0);
                long end=System.currentTimeMillis()+2000;
                while(typed.length()==0 && System.currentTimeMillis()<end) Thread.sleep(10);
                if(!typed.toString().equals("a")) throw new RuntimeException("XIM keyboard delivery "+typed);
                typed.setLength(0); compose(); end=System.currentTimeMillis()+2000;
                while(typed.length()==0 && System.currentTimeMillis()<end) Thread.sleep(10);
                if(!typed.toString().equals("\u00e9")) throw new RuntimeException("XIM composed text "+typed);
                method.endComposition(); method.deactivate();
                typed.setLength(0); compose(); end=System.currentTimeMillis()+2000;
                while(typed.length()<2 && System.currentTimeMillis()<end) Thread.sleep(10);
                if(!typed.toString().equals("'e")) throw new RuntimeException("XIM deactivation "+typed);
                method.activate();
                frame.dispose(); method.endComposition(); method.deactivate(); method.dispose(); method.dispose(); System.gc();
            }
            System.out.println("XIM activation, keyboard delivery and disposal passed"); System.exit(0);
        } catch(Throwable error) { error.printStackTrace(); System.exit(1); }
    }
}
