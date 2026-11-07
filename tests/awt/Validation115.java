import java.awt.*;
import java.awt.peer.ContainerPeer;

public class Validation115 {
    static { System.loadLibrary("awt115probe"); }
    static native boolean visible(String title);
    static native int width(String title);
    static native int ink(String title, String label);
    static native void showWindow(String title);
    static void painted(String label) throws Exception {
        for(int i=0;i<100;i++) {
            if(ink("Validation115",label)>10) return;
            Thread.sleep(20);
        }
        throw new RuntimeException("natural label paint missing: "+label);
    }
    public static void main(String[] args) {
        Frame frame=null;
        try {
            frame=new Frame("Validation115");
            frame.setLayout(new BorderLayout());
            Label top=new Label("Validation heading");
            Label bottom=new Label("Validation footer");
            top.setBackground(Color.white); top.setForeground(Color.black);
            bottom.setBackground(Color.white); bottom.setForeground(Color.black);
            frame.add("North",top); frame.add("South",bottom);
            frame.add("Center",new TextArea("Content"));
            frame.setBounds(80,90,340,200); frame.addNotify();
            ContainerPeer peer=(ContainerPeer)frame.getPeer();
            peer.beginValidate(); Thread.sleep(100); peer.endValidate(); Thread.sleep(100);
            if(visible("Validation115")) throw new RuntimeException("validation showed hidden frame");
            frame.show(); showWindow("Validation115");
            painted("Validation heading"); painted("Validation footer");
            peer.beginValidate(); peer.beginValidate(); Thread.sleep(100);
            boolean remainedVisible=visible("Validation115");
            int oldWidth=width("Validation115");
            peer.reshape(80,90,oldWidth+20,220); Thread.sleep(100);
            boolean deferred=width("Validation115")==oldWidth;
            peer.endValidate();
            if(!remainedVisible) throw new RuntimeException("validation hid visible frame");
            Thread.sleep(100);
            if(!deferred || width("Validation115")!=oldWidth+20)
                throw new RuntimeException("validation did not commit deferred bounds");
            peer.endValidate();
            for(int i=0;i<3;i++) {
                frame.setSize(360+i*20,220+i*15); frame.invalidate(); frame.validate();
                painted("Validation heading"); painted("Validation footer");
            }
            peer.beginValidate(); peer.reshape(80,90,430,280); Thread.sleep(100);
            frame.dispose(); peer.endValidate(); Thread.sleep(100);
            if(visible("Validation115")) throw new RuntimeException("pending validation survived disposal");
            System.out.println("validation visibility and natural initial/resize paint passed"); System.exit(0);
        } catch(Throwable failure) {
            failure.printStackTrace(); if(frame!=null)frame.dispose(); System.exit(1);
        }
    }
}
