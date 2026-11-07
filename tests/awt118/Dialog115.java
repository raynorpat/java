import java.awt.*;
import java.awt.event.*;

public class Dialog115 {
    static volatile int ownerClicks,dialogClicks;
    static volatile Throwable failure;
    static void check(boolean value,String message) { if (!value) throw new RuntimeException(message); }
    public static void main(String[] args) throws Exception {
        try { run(); } catch (Throwable error) { error.printStackTrace(); System.exit(1); }
    }
    static void run() throws Exception {
        System.getProperties().put("awt.toolkit","sun.awt.motif.MToolkit");
        Toolkit.getDefaultToolkit(); System.loadLibrary("motif118xprobe");
        final Frame owner=new Frame("Dialog owner"); final Button ownerButton=new Button("Owner");
        ownerButton.addActionListener(new ActionListener() { public void actionPerformed(ActionEvent event) { ownerClicks++; } });
        owner.add(ownerButton); owner.setBounds(10,10,100,100); owner.show();
        for (int cycle=0;cycle<4;cycle++) {
            final Dialog dialog=new Dialog(owner,"Modal",true); final Button close=new Button("Close");
            close.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent event) { dialogClicks++; dialog.dispose(); }
            });
            dialog.add(close); dialog.setBounds(150,100,120,100);
            Thread interaction=new Thread(new Runnable() {
                public void run() {
                    try {
                        Thread.sleep(500); int before=ownerClicks; Window115.click(ownerButton); Thread.sleep(400);
                        check(ownerClicks==before,"modal owner input suppressed");
                        Window115.click(close);
                    } catch (Throwable error) { failure=error; dialog.dispose(); }
                }
            });
            int before=dialogClicks; interaction.start(); dialog.show(); interaction.join(2000);
            if (failure!=null) throw new RuntimeException(failure.toString());
            check(dialogClicks==before+1,"modal callback and wait release");
            dialog.dispose(); System.gc();
        }
        Dialog modeless=new Dialog(owner,"Modeless",false); modeless.setBounds(150,100,130,110);
        modeless.show(); modeless.setTitle("Renamed"); modeless.setResizable(false); modeless.hide(); modeless.show(); modeless.dispose();
        final Dialog hidden=new Dialog(owner,"Hide modal",true); hidden.setBounds(150,100,120,100);
        Thread hiding=new Thread(new Runnable() { public void run() {
            try { Thread.sleep(500); hidden.hide(); } catch (Throwable error) { failure=error; }
        } });
        hiding.start(); hidden.show(); hiding.join(2000); hidden.dispose();
        if (failure!=null) throw new RuntimeException(failure.toString());
        int before=ownerClicks; Window115.click(ownerButton); Thread.sleep(400);
        check(ownerClicks==before+1,"owner input restored after modal hide");
        owner.dispose(); System.out.println("dialog modality, callbacks and disposal passed"); System.exit(0);
    }
}
