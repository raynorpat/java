import java.net.URL;
import sun.applet.AppletViewer;
/* Keep the shipped viewer, security initialization, HTML parser and applets
 * unchanged. The command-line mainInit copyright-acceptance UI is not a demo
 * and cannot be accepted by an automated test. */
public class WinAwtDemoApplet115 {
    static volatile boolean parsing=true;
    public static void main(String[] args) {
        try {
            AppletViewer.init();
            if(args.length>1 && args[1].equals("observe-live-parser")) {
                Thread observer=new Thread(new Runnable() {public void run() {
                    try {Thread.sleep(1500);} catch(InterruptedException e) {return;}
                    System.out.println("APPLET_LIVE_COUNT "+AppletViewer.countApplets());
                    System.out.println("PARSER_PENDING "+parsing);
                }});
                observer.setDaemon(true);observer.start();
            }
            AppletViewer.parse(new URL(args[0]));
            parsing=false;
            int count=AppletViewer.countApplets();
            if(count==0) throw new RuntimeException("no applets parsed");
            System.out.println("APPLET_COUNT "+count);
        } catch(Throwable failure) {failure.printStackTrace();System.exit(1);}
    }
}
