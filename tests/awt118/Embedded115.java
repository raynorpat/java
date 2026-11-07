import java.awt.*;
import java.awt.event.*;
import sun.awt.motif.MEmbeddedFrame;

public class Embedded115 {
    static native int host();
    static native int retired();
    static native void destroyHost(int handle);
    static volatile int actions;
    public static void main(String[] args) throws Exception {
        try {
            Toolkit.getDefaultToolkit(); System.loadLibrary("motif118xprobe");
            for(int cycle=0;cycle<4;cycle++) {
                int handle=host();
                MEmbeddedFrame frame=new MEmbeddedFrame(handle);
                frame.setLayout(null); frame.setSize(160,100);
                Button button=new Button("Embedded"); button.setBounds(10,10,110,30); frame.add(button);
                button.addActionListener(new ActionListener(){public void actionPerformed(ActionEvent event){actions++;}});
                frame.show();
                Insets insets=frame.getInsets();
                if(insets.top!=0 || insets.left!=0 || insets.bottom!=0 || insets.right!=0) throw new RuntimeException("embedded insets");
                Window115.click(button); long end=System.currentTimeMillis()+2000;
                while(actions<=cycle && System.currentTimeMillis()<end) Thread.sleep(10);
                if(actions!=cycle+1) throw new RuntimeException("embedded callback");
                frame.hide(); frame.show();
                if((cycle&1)!=0) { destroyHost(handle); frame.setSize(180,120); }
                frame.dispose(); System.gc();
                if(retired()!=cycle+1) throw new RuntimeException("host retirement");
            }
            System.out.println("embedded host shells, callbacks and retirement passed"); System.exit(0);
        } catch(Throwable error) { error.printStackTrace(); System.exit(1); }
    }
}
