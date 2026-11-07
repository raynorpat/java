package sun.awt.motif;
import java.awt.*;
import java.awt.image.*;
import sun.awt.image.ImageRepresentation;

public class Surface115 {
    static void check(boolean value,String message) { if (!value) throw new RuntimeException(message); }
    public static void main(String[] args) throws Exception {
        try { run(); } catch (Throwable error) { error.printStackTrace(); System.exit(1); }
    }
    static void run() throws Exception {
        for (int cycle=0;cycle<4;cycle++) {
            Frame frame=new Frame(); frame.setLayout(null); frame.setSize(160,120);
            Canvas canvas=new Canvas(); canvas.setBounds(10,10,40,30); frame.add(canvas); frame.show();
            MDrawingSurfaceInfo surface=new MDrawingSurfaceInfo((MComponentPeer)canvas.getPeer());
            int state=surface.lock();
            try {
                check(surface.getDisplay()!=0 && surface.getDrawable()!=0,"window drawable");
                check(surface.getDepth()==24 && surface.getVisualID()!=0 && surface.getColormapID()!=0,"visual");
                check(surface.getBounds().equals(new Rectangle(0,0,40,30)),"bounds");
            } finally { surface.unlock(); }
            check(surface.lock()==state,"stable state"); surface.unlock();
            canvas.setSize(50,35);
            check(surface.lock()==state+1,"resized state"); surface.unlock();
            sun.awt.image.Image image=(sun.awt.image.Image)frame.createImage(new MemoryImageSource(2,2,new int[]{-1,-1,-1,-1},0,2));
            ImageRepresentation rep=new ImageRepresentation(image,2,2,0);
            rep.setDimensions(2,2);
            rep.setPixels(0,0,2,2,ColorModel.getRGBdefault(),new int[]{-1,-1,-1,-1},0,2);
            MDrawingSurfaceInfo offscreen=new MDrawingSurfaceInfo(rep);
            offscreen.lock();
            try { check(offscreen.getDrawable()!=0,"image drawable"); }
            finally { offscreen.unlock(); }
            rep.doFinalization(); image.flush(); frame.dispose();
            boolean retired=false;
            surface.lock();
            try { surface.getDrawable(); } catch (NullPointerException expected) { retired=true; }
            finally { surface.unlock(); }
            check(retired,"retired window"); System.gc();
        }
        System.out.println("drawing surface state, visuals and retirement passed"); System.exit(0);
    }
}
