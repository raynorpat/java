package sun.awt;

import java.awt.*;
import java.io.*;
import java.util.Enumeration;

/* Test-only observer; invokes unchanged demo mains and reads public UI state. */
public class DemoObserver115 {
    static native int windowId(Window window);
    static String clean(String text) {
        return text==null ? "" : text.replace('\t',' ').replace('\n',' ').replace('\r',' ');
    }
    static void record(Component component,PrintStream out,String owner) {
        if (!component.isShowing()) return;
        Point point=component.getLocationOnScreen();
        Dimension size=component.getSize();
        String label="";
        if(component instanceof Frame) label=((Frame)component).getTitle();
        else if(component instanceof Button) label=((Button)component).getLabel();
        else if(component instanceof Checkbox) label=((Checkbox)component).getLabel();
        else if(component instanceof Choice) label=((Choice)component).getSelectedItem();
        else if(component instanceof TextComponent) label=((TextComponent)component).getText();
        else if(component instanceof Label) label=((Label)component).getText();
        out.println(component.getClass().getName()+"\t"+point.x+"\t"+point.y+"\t"+
            size.width+"\t"+size.height+"\t"+clean(label)+"\t"+owner);
        if(component instanceof Container) {
            Component[] children=((Container)component).getComponents();
            for(int i=0;i<children.length;i++) record(children[i],out,owner);
        }
    }
    public static void main(String[] args) throws Exception {
        Toolkit.getDefaultToolkit();
        System.load(System.getProperty("demo.probe"));
        final String output=System.getProperty("demo.snapshot");
        Thread observer=new Thread(new Runnable() {
            public void run() {
                for(;;) {
                    try {
                        Thread.sleep(300);
                        File temporary=new File(output+".tmp");
                        PrintStream stream=new PrintStream(new FileOutputStream(temporary));
                        Enumeration targets=SunToolkit.peerMap.keys();
                        while(targets.hasMoreElements()) {
                            Object target=targets.nextElement();
                            if(target instanceof Window) {
                                Component window=(Component)target;
                                synchronized(window.getTreeLock()) {
                                    String owner="0x"+Long.toHexString(windowId((Window)window)&0xffffffffL);
                                    record(window,stream,owner);
                                }
                            }
                        }
                        stream.close();
                        temporary.renameTo(new File(output));
                    } catch(Exception failure) { System.err.println("observer: "+failure); }
                }
            }
        });
        observer.setDaemon(true); observer.start();
        String[] forwarded=new String[args.length-1];
        System.arraycopy(args,1,forwarded,0,forwarded.length);
        Class.forName(args[0]).getMethod("main",new Class[]{forwarded.getClass()})
            .invoke(null,new Object[]{forwarded});
    }
}
