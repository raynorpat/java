import java.awt.*;
import java.awt.event.*;

public class Dialog115 {
    static volatile int ownerClicks,dialogClicks;
    static volatile Throwable failure;
    static volatile int nestedStage;
    static void check(boolean value,String message) { if (!value) throw new RuntimeException(message); }
    static void awaitStage(int stage) throws Exception {
        long deadline=System.currentTimeMillis()+4000;
        while (nestedStage<stage && failure==null && System.currentTimeMillis()<deadline) Thread.sleep(20);
        check(failure==null && nestedStage==stage,"nested modal stage "+stage+": "+nestedStage+" "+failure);
        Thread.sleep(300);
    }
    static void nested() throws Exception {
        final Frame owner=new Frame("EDT modal owner");
        final Dialog outer=new Dialog(owner,"Outer",true), inner=new Dialog(owner,"Inner",true);
        final Button open=new Button("Open"), next=new Button("Nested"), close=new Button("Close");
        final Button blocked=new Button("Blocked"), finish=new Button("Finish");
        final int[] blockedClicks={0};
        blocked.addActionListener(new ActionListener() { public void actionPerformed(ActionEvent event) { blockedClicks[0]++; } });
        open.addActionListener(new ActionListener() { public void actionPerformed(ActionEvent event) {
            try {
                check(Thread.currentThread().getName().indexOf("AWT-EventQueue")>=0 ||
                      Thread.currentThread().getName().indexOf("AWT-Dispatch")>=0,"modal show outside EDT");
                nestedStage=1; outer.show(); nestedStage=4;
            } catch (Throwable error) { failure=error; }
        } });
        next.addActionListener(new ActionListener() { public void actionPerformed(ActionEvent event) {
            nestedStage=2; inner.show(); nestedStage=3;
        } });
        close.addActionListener(new ActionListener() { public void actionPerformed(ActionEvent event) { inner.dispose(); } });
        finish.addActionListener(new ActionListener() { public void actionPerformed(ActionEvent event) { outer.dispose(); } });
        owner.setLayout(new GridLayout(2,1)); owner.add(open); owner.add(blocked); owner.setBounds(10,10,120,130);
        outer.setLayout(new GridLayout(2,1)); outer.add(next); outer.add(finish); outer.setBounds(160,100,120,130);
        inner.add(close); inner.setBounds(320,220,120,100);
        owner.show(); Thread.sleep(300);
        Window115.click(open); awaitStage(1);
        Window115.click(blocked); Thread.sleep(200); check(blockedClicks[0]==0,"outer failed to block owner");
        Window115.click(next); awaitStage(2);
        Window115.click(finish); Thread.sleep(200); check(nestedStage==2,"inner failed to block outer");
        Window115.click(close); awaitStage(3);
        Window115.click(finish); awaitStage(4);
        Window115.click(blocked); Thread.sleep(300); check(blockedClicks[0]==1,"nested modal owner not restored");
        owner.dispose();
    }
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
        owner.dispose(); nested(); System.out.println("dialog modality, callbacks and disposal passed"); System.exit(0);
    }
}
