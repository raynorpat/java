import java.awt.*;
import sun.awt.*;
public class SurfaceLocks115 {
    static volatile boolean started,done;
    static volatile Throwable failure;
    public static void main(String[] args) {
        try {
            Frame frame=new Frame(); frame.setSize(220,170); frame.show();
            final Image image=frame.createImage(12,10);
            Graphics initial=image.getGraphics(); initial.setColor(Color.blue); initial.fillRect(0,0,12,10); initial.dispose();
            DrawingSurfaceInfo info=((DrawingSurface)image).getDrawingSurfaceInfo();
            Win32DrawingSurface surface=(Win32DrawingSurface)info.getSurface();
            if(surface.getHBitmap()==0) throw new RuntimeException("unlocked bitmap missing");
            int palette=surface.getHPalette();
            info.lock();
            if(WinAwtProbe.surfacePixel(surface.getHDC(),-1)!=0xff0000) throw new RuntimeException("surface is not image DC");
            if(WinAwtProbe.surfacePixel(surface.getHDC(),0xff)!=0xff) throw new RuntimeException("native surface drawing");
            Thread worker=new Thread(new Runnable() {
                public void run() {
                    try { Graphics g=image.getGraphics(); g.setColor(Color.green); started=true; g.fillRect(0,0,12,10); g.dispose(); done=true; }
                    catch(Throwable t) { failure=t; }
                }
            });
            worker.start();
            for(int i=0;!started && failure==null && i<200;i++) Thread.sleep(10);
            Thread.sleep(100); System.gc();
            if(!started || done || failure!=null) throw new RuntimeException("native surface exclusion "+started+"/"+done+"/"+failure);
            if(surface.getHPalette()!=palette) throw new RuntimeException("palette identity while drawing waits");
            // The locking thread may reenter drawing while another caller waits.
            Graphics nested=image.getGraphics(); nested.setColor(Color.red);
            nested.fillRect(0,0,12,10); nested.dispose();
            info.unlock(); worker.join(3000);
            if(!done || failure!=null) throw new RuntimeException("surface unlock did not release drawing "+failure);
            info.lock();
            if(WinAwtProbe.surfacePixel(surface.getHDC(),-1)!=0xff00) throw new RuntimeException("unblocked image drawing");
            if(System.getProperty("awt.correctedBounds")!=null) {
                final Graphics doomed=image.getGraphics(); doomed.setColor(Color.blue); started=false; done=false; failure=null;
                Thread retiring=new Thread(new Runnable() {
                    public void run() {
                        started=true;
                        try { doomed.fillRect(0,0,2,2); done=true; }
                        catch(NullPointerException expected) { done=true; }
                        catch(Throwable t) { failure=t; }
                    }
                });
                retiring.start();
                for(int i=0;!started && i<200;i++) Thread.sleep(10);
                Thread.sleep(100); doomed.dispose(); info.unlock(); retiring.join(3000);
                if(!done || failure!=null) throw new RuntimeException("waiting graphics retirement "+failure);
                info.lock();
                if(WinAwtProbe.surfacePixel(surface.getHDC(),-1)!=0xff00) throw new RuntimeException("retired graphics changed surface");
                // Retain the locked surface even when the Java image detaches it.
                image.flush(); System.gc();
                if(WinAwtProbe.surfacePixel(surface.getHDC(),-1)!=0xff00) throw new RuntimeException("locked image flush lifetime");
            }
            info.unlock();
            try { info.unlock(); throw new RuntimeException("duplicate unlock accepted"); } catch(NullPointerException expected) {}
            frame.dispose(); image.flush();
            System.out.println("drawing surface native access, exclusion, GC and flush lifetime passed"); System.exit(0);
        } catch(Throwable t) { t.printStackTrace(); System.exit(1); }
    }
}
