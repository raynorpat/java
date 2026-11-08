import java.awt.*;
import java.awt.event.*;
import java.awt.peer.ComponentPeer;

public class Dialog115 {
    static { System.loadLibrary("dialog115"); }
    static native boolean nativeState(ComponentPeer peer, String title, int style);
    static native boolean textFocused(ComponentPeer peer);
    static native boolean modalWindow(ComponentPeer peer);
    static native void clickButton(ComponentPeer peer);
    static volatile Throwable failure;
    static volatile int nestedActions, ownerActions;
    static void awaitModal(Dialog dialog) throws Exception {
        for (int wait = 0; wait < 200 && (dialog.getPeer() == null || !modalWindow(dialog.getPeer())); wait++) Thread.sleep(10);
        require(dialog.getPeer() != null && modalWindow(dialog.getPeer()), "native modal session");
    }
    static void failedConstruction(Frame owner, int stage) throws Exception {
        Dialog dialog = new Dialog(owner, "Failure", false); dialog.setBounds(370, 130, 260, 180);
        if (stage == 0) dialog.setFont(new BareWindow115.BadFont());
        BareWindow115.failAt(stage);
        try { dialog.addNotify(); throw new Error("dialog construction succeeded"); }
        catch (RuntimeException problem) {
            if (stage == 0) require(problem == BareWindow115.marker, "dialog throwable identity");
            else require(problem.getMessage().indexOf("construction failure") >= 0, "native dialog exception");
        } finally { BareWindow115.failAt(0); }
        require(dialog.getPeer() == null, "failed dialog peer"); BareWindow115.awaitObjects(2);
    }
    static void require(boolean value, String message) { if (!value) throw new Error(message); }
    public static void main(String[] args) {
        try {
            final boolean tracked = args.length != 0;
            if (tracked) { Toolkit.getDefaultToolkit(); BareWindow115.track(); }
            final Frame owner = new Frame("Dialog owner"); owner.setLayout(null); owner.setBounds(350, 100, 300, 240);
            final Button ownerButton = new Button("Owner"); ownerButton.setBounds(20, 40, 100, 30); owner.add(ownerButton);
            ownerButton.addActionListener(new ActionListener() { public void actionPerformed(ActionEvent event) { ownerActions++; } });
            owner.show();
            if (tracked) for (int stage = 0; stage < 3; stage++) failedConstruction(owner, stage);
            for (int mode = 0; mode < 2; mode++) {
                Dialog dialog = new Dialog(owner, "Dialog", false); dialog.setLayout(null);
                dialog.setResizable(mode != 0); dialog.setBounds(370, 130, 260, 180);
                TextField disabled = new TextField("disabled"), field = new TextField("editable");
                disabled.setEnabled(false); disabled.setBounds(20, 40, 180, 24); field.setBounds(20, 80, 180, 24);
                dialog.add(disabled); dialog.add(field); dialog.show();
                require(nativeState(dialog.getPeer(), "Dialog", mode == 0 ? 3 : 11), "dialog native style/title");
                require(textFocused(field.getPeer()), "dialog initial enabled text responder");
                dialog.setTitle("Renamed"); dialog.setResizable(mode == 0);
                require(nativeState(dialog.getPeer(), "Renamed", mode == 0 ? 3 : 11), "donor ignores dynamic dialog resize flag");
                dialog.hide(); dialog.show(); Toolkit.getDefaultToolkit().sync(); System.gc(); dialog.dispose();
            }
            final Dialog modal = new Dialog(owner, "Modal", true); modal.setBounds(370, 130, 260, 180);
            modal.setLayout(null);
            final Button closeButton = new Button("Close"); closeButton.setBounds(30, 60, 100, 30); modal.add(closeButton);
            closeButton.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent event) {
                    try {
                        Toolkit.getDefaultToolkit().sync(); System.gc();
                        final Dialog inner = new Dialog(owner, "Nested modal", true); inner.setLayout(null); inner.setBounds(390, 150, 260, 180);
                        final Button innerClose = new Button("Close inner"); innerClose.setBounds(30, 60, 120, 30); inner.add(innerClose);
                        innerClose.addActionListener(new ActionListener() {
                            public void actionPerformed(ActionEvent event) {
                                Toolkit.getDefaultToolkit().sync(); System.gc(); inner.hide(); inner.dispose(); nestedActions++;
                            }
                        });
                        Thread innerDriver = new Thread(new Runnable() {
                            public void run() {
                                try { awaitModal(inner); clickButton(innerClose.getPeer()); }
                                catch (Throwable problem) { failure = problem; inner.hide(); }
                            }
                        });
                        innerDriver.start(); inner.show(); innerDriver.join(5000);
                        require(!innerDriver.isAlive() && nestedActions == 1 && failure == null, "nested modal completion");
                        require(modalWindow(modal.getPeer()), "outer modal session restored");
                        modal.hide(); if (tracked) modal.dispose();
                    } catch (Throwable problem) { failure = problem; modal.hide(); }
                }
            });
            Thread closer = new Thread(new Runnable() {
                public void run() {
                    try {
                        awaitModal(modal);
                        clickButton(ownerButton.getPeer()); Thread.sleep(150);
                        require(ownerActions == 0, "modal owner input was not blocked");
                        clickButton(closeButton.getPeer());
                    } catch (Throwable problem) { failure = problem; modal.hide(); }
                }
            });
            closer.start(); modal.show(); closer.join(5000);
            require(!closer.isAlive() && failure == null && !modal.isVisible(), "modal show/hide completion: " + failure);
            modal.dispose(); owner.dispose(); if (tracked) BareWindow115.awaitObjects(0);
            System.out.println("DR2 dialogs preserve styles, initial text focus and modal sessions");
            System.exit(0);
        } catch (Throwable problem) { problem.printStackTrace(); System.exit(1); }
    }
}
