import java.awt.*;
import java.io.FileDescriptor;

public class Warning115 {
    static native boolean banner(Window window,String text);
    static class Restricted extends SecurityManager {
        public boolean checkTopLevelWindow(Object window) { return false; }
        public void checkExit(int status) {}
        public void checkPropertyAccess(String property) {}
        public void checkPropertiesAccess() {}
        public void checkAccess(Thread thread) {}
        public void checkAccess(ThreadGroup group) {}
        public void checkRead(String file) {}
        public void checkRead(FileDescriptor file) {}
        public void checkLink(String library) {}
        public void checkPackageAccess(String name) {}
        public void checkAwtEventQueueAccess() {}
    }
    static class Preferred extends Canvas {
        public Dimension getPreferredSize() { return new Dimension(200,100); }
    }
    static void check(Window window) throws Exception {
        if(window instanceof Frame) {
            MenuBar bar=new MenuBar(); Menu menu=new Menu("File"); menu.add("Close"); bar.add(menu);
            ((Frame)window).setMenuBar(bar);
        }
        Canvas canvas=new Preferred(); window.add(canvas);
        Choice choice=new Choice(); choice.add("One"); choice.add("Two"); window.add(choice,BorderLayout.NORTH);
        window.pack(); window.show();
        Thread.sleep(300);
        if(window.getWarningString()==null || window.getInsets().top<=0)
            throw new RuntimeException("missing warning banner: "+window.getClass()+" "+window.getInsets());
        Graphics graphics=window.getGraphics(); graphics.setColor(Color.red);
        graphics.fillRect(0,0,1000,1000); graphics.dispose(); Toolkit.getDefaultToolkit().sync();
        if(!banner(window,window.getWarningString())) throw new RuntimeException("warning text, color, clipping or visibility");
        if(canvas.getSize().width!=200 || canvas.getSize().height!=100)
            throw new RuntimeException("warning clips packed content: "+canvas.getSize());
        Point outer=window.getLocationOnScreen(),inner=canvas.getLocationOnScreen();
        if(inner.y-outer.y!=window.getInsets().top+choice.getSize().height)
            throw new RuntimeException("warning client origin: "+window.getClass()+" "+outer+" "+inner+" "+window.getInsets());
        if(choice.getLocationOnScreen().y-outer.y!=window.getInsets().top)
            throw new RuntimeException("choice ignores window insets");
        window.setSize(window.getSize().width+20,window.getSize().height+20);
        window.validate(); window.hide(); window.show(); Thread.sleep(100);
        if(!banner(window,window.getWarningString())) throw new RuntimeException("warning lost after resize/show");
        window.dispose();
    }
    public static void main(String[] args) {
        try {
            Toolkit.getDefaultToolkit();
            System.loadLibrary("motif118xprobe");
            System.setSecurityManager(new Restricted());
            for(int i=0;i<3;i++) {
                Frame owner=new Frame("Warning owner"); owner.setSize(250,160); owner.show();
                check(new Frame("Warning frame"));
                check(new Dialog(owner,"Warning dialog",false));
                check(new Window(owner)); owner.dispose();
            }
            System.out.println("untrusted window warning banners and client bounds passed");
            System.exit(0);
        } catch(Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
