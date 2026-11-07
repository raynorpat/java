import java.awt.*;
import sun.awt.*;
public class DrawingSurface115 {
    static void unlocked(Win32DrawingSurface surface) {
        try { surface.getHDC(); throw new RuntimeException("unlocked HDC accepted"); } catch(NullPointerException expected) {}
        try { surface.getHWnd(); throw new RuntimeException("unlocked HWND accepted"); } catch(NullPointerException expected) {}
    }
    public static void main(String[] args) {
        try {
            Frame frame=new Frame(); Canvas canvas=new Canvas(); frame.add(canvas); frame.setSize(220,170); frame.show();
            DrawingSurfaceInfo info=((DrawingSurface)canvas.getPeer()).getDrawingSurfaceInfo();
            Win32DrawingSurface surface=(Win32DrawingSurface)info.getSurface();
            unlocked(surface);
            int first=info.lock();
            if(surface.getHWnd()==0 || surface.getHDC()==0 || surface.getHBitmap()!=0 || surface.getPBits()!=0) throw new RuntimeException("component native handles");
            int depth=surface.getDepth();
            Rectangle bounds=info.getBounds(); if(bounds.width!=canvas.getSize().width || bounds.height!=canvas.getSize().height) throw new RuntimeException("surface bounds");
            info.unlock(); unlocked(surface);
            if(info.lock()!=first) throw new RuntimeException("unchanged state"); info.unlock();
            canvas.setSize(canvas.getSize().width+10,canvas.getSize().height+10);
            int resized=info.lock(); info.unlock(); if(resized!=first+1) throw new RuntimeException("resized state");
            Image image=canvas.createImage(12,10);
            DrawingSurfaceInfo imageInfo=((DrawingSurface)image).getDrawingSurfaceInfo();
            Win32DrawingSurface imageSurface=(Win32DrawingSurface)imageInfo.getSurface();
            int imageState=imageInfo.lock();
            if(imageSurface.getHWnd()!=0 || imageSurface.getHDC()==0 || imageSurface.getHBitmap()==0) throw new RuntimeException("offscreen native handles");
            boolean bits=imageSurface.getPBits()!=0;
            System.gc(); imageInfo.unlock(); unlocked(imageSurface);
            frame.dispose();
            try { info.lock(); throw new RuntimeException("retired component accepted"); } catch(NullPointerException expected) {}
            image.flush();
            System.out.println("drawing surfaces passed state="+first+"/"+resized+" image="+imageState+" depth="+depth+" bits="+bits); System.exit(0);
        } catch(Throwable t) { t.printStackTrace(); System.exit(1); }
    }
}
