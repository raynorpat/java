import java.awt.*;
import java.awt.event.*;
public class Dialog115 {
    static volatile Throwable failure;
    static volatile boolean returned;
    static volatile boolean disposed;
    public static void main(String[] args) {
        try {
            Frame owner=new Frame("Dialog115 owner"); owner.setBounds(50,60,300,220); owner.show();
            for (int i=0;i<3;i++) {
                final Dialog dialog=new Dialog(owner,"Dialog115",i!=0); dialog.setBounds(80,90,220,150);
                dialog.add(new Button("Close")); dialog.setResizable(i!=1);
                dialog.addWindowListener(new WindowAdapter() { public void windowClosing(WindowEvent e) { System.gc(); dialog.dispose(); disposed=true; } });
                returned=false; disposed=false; failure=null;
                Thread worker=new Thread(new Runnable() {
                    public void run() { try { dialog.show(); returned=true; } catch (Throwable problem) { failure=problem; } }
                });
                worker.start();
                for (int j=0;(!dialog.isShowing() || (dialog.isModal() && WinAwtProbe.windowEnabled("Dialog115 owner"))) && j<100;j++) Thread.sleep(10);
                if (failure!=null) throw new RuntimeException("show failed: "+failure);
                if (!dialog.isShowing() || (dialog.isModal() && returned)) throw new RuntimeException("modal show");
                if (WinAwtProbe.windowEnabled("Dialog115 owner")==dialog.isModal()) throw new RuntimeException("owner enable state");
                WinAwtProbe.close("Dialog115"); worker.join(2000);
                /* Sun clears peer before removing the dialog from the owner's
                 * window list. Wait for that whole callback before disposing
                 * the owner, whose dispose() snapshots the same list. */
                for (int j=0;(!disposed || dialog.getPeer()!=null) && j<100;j++) Thread.sleep(10);
                if (worker.isAlive() || failure!=null || !disposed || dialog.getPeer()!=null) throw new RuntimeException("modal retirement "+failure);
                if (!WinAwtProbe.windowEnabled("Dialog115 owner")) throw new RuntimeException("owner remains disabled");
            }
            owner.dispose();
            System.out.println("modeless and modal dialogs, owner disabling and close callbacks passed"); System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
