import java.awt.*;
import java.awt.image.*;
public class Images115 {
    static int[] pixels(Image image,int w,int h) throws Exception {
        int[] result = new int[w*h];
        PixelGrabber grab = new PixelGrabber(image,0,0,w,h,result,0,w);
        if (!grab.grabPixels(5000)) throw new RuntimeException("pixel production " + grab.getStatus());
        return result;
    }
    /* The 32-bpp Sun sendPixels branch repeats source blue in all channels. */
    static void color(int[] p,int w,int x,int y,int rgb) {
        if ((p[y*w+x]&0xffffff) != (rgb&255)*0x10101) throw new RuntimeException("pixel " + x+","+y+" = "+Integer.toHexString(p[y*w+x]));
    }
    public static void main(String[] args) {
      try {
        Frame frame = new Frame("Raster contracts"); frame.setSize(100,100); frame.show();
        Image image = frame.createImage(32,32); Graphics g = image.getGraphics();
        g.setColor(Color.white); g.fillRect(0,0,32,32);
        g.setColor(Color.red); g.drawLine(1,1,5,1); g.drawRect(2,3,4,4);
        g.setColor(Color.blue); g.fillRect(10,10,4,4); g.copyArea(10,10,4,4,5,0);
        g.setXORMode(Color.white); g.fillRect(10,10,4,4); g.fillRect(10,10,4,4); g.setPaintMode();
        g.setClip(20,20,2,2); g.setColor(Color.green); g.fillRect(18,18,8,8); g.setClip(null);
        int[] p = pixels(image,32,32);
        color(p,32,1,1,0xff0000); color(p,32,5,1,0xff0000); color(p,32,6,7,0xff0000);
        color(p,32,11,11,0x0000ff); color(p,32,16,11,0x0000ff); color(p,32,20,20,0x00ff00); color(p,32,19,20,0xffffff);
        int[] argb = {0xffff0000,0xff00ff00,0xff0000ff,0xffffffff};
        Image source = frame.createImage(new MemoryImageSource(2,2,argb,0,2));
        if (!g.drawImage(source,0,24,null)) { MediaTracker tracker = new MediaTracker(frame); tracker.addImage(source,1); tracker.waitForAll(); g.drawImage(source,0,24,null); }
        p = pixels(image,32,32); color(p,32,0,24,0xff0000); color(p,32,1,25,0xffffff);
        System.gc(); g.setColor(Color.black); g.drawString("Hello \u00e9\u03a9",0,20);
        g.dispose(); image.flush(); frame.dispose();
        System.out.println("offscreen raster, copy, XOR, clipping, image pixels and text passed"); System.exit(0);
      } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}


