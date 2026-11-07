import java.awt.*;
import java.awt.image.*;

public class Icon115 {
    static native void sizes();
    static native int pixel(Frame frame);
    static Image image(Frame frame,int width,int height,int color) {
        int[] pixels=new int[width*height];
        for(int i=0;i<pixels.length;i++) pixels[i]=color;
        return frame.createImage(new MemoryImageSource(width,height,pixels,0,width));
    }
    public static void main(String[] args) throws Exception {
        try {
            Toolkit.getDefaultToolkit(); System.loadLibrary("motif118xprobe"); sizes();
            for(int cycle=0;cycle<4;cycle++) {
                Frame frame=new Frame(); frame.setSize(120,80); frame.show();
                Image red=image(frame,17,19,0xffff0000); frame.setIconImage(red);
                red.flush(); System.gc();
                if(pixel(frame)!=0xff0000) throw new RuntimeException("red icon");
                Image blue=image(frame,3,3,0xff0000ff); frame.setIconImage(blue);
                blue.flush(); System.gc();
                if(pixel(frame)!=0x0000ff) throw new RuntimeException("replacement icon");
                frame.dispose();
            }
            System.out.println("frame icon sizing, replacement and retirement passed"); System.exit(0);
        } catch(Throwable error) { error.printStackTrace(); System.exit(1); }
    }
}
