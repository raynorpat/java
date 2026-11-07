import java.awt.*;
import java.awt.image.*;
public class Raster115 {
    static int[] pixels(Image image,int w,int h) throws Exception {
        int[] p=new int[w*h]; PixelGrabber grab=new PixelGrabber(image,0,0,w,h,p,0,w);
        if (!grab.grabPixels(5000)) throw new RuntimeException("pixel production"); return p;
    }
    static void show(String name,Image image) throws Exception {
        int[] p=pixels(image,64,64); int hash=0,count=0;
        for (int i=0;i<p.length;i++) { hash=hash*31+(p[i]&0xffffff); if ((p[i]&0xffffff)!=0xffffff) count++; }
        System.out.println(name+" "+hash+" "+count);
    }
    static Graphics reset(Image image) {
        Graphics g=image.getGraphics(); g.setColor(Color.white); g.fillRect(0,0,64,64); g.setColor(Color.black); return g;
    }
    public static void main(String[] args) {
      try {
        Frame f=new Frame("Raster"); f.setSize(100,100); f.show(); Image image=f.createImage(64,64);
        Graphics g=reset(image); g.drawLine(1,1,10,1); g.drawLine(1,3,1,13); g.drawLine(2,2,16,19); g.drawLine(24,21,4,8); g.drawLine(30,30,30,30); show("line",image); g.dispose();
        g=reset(image); g.drawRect(2,2,14,17); g.drawRect(20,3,0,9); g.drawRect(28,6,9,0); g.drawRect(40,7,0,0); g.drawRect(0,0,-1,3); show("rect",image); g.dispose();
        g=reset(image); g.drawRoundRect(1,1,24,19,8,6); g.fillRoundRect(30,2,24,17,8,10); show("round",image); g.dispose();
        g=reset(image); g.drawOval(1,1,24,19); g.fillOval(30,2,24,17); g.drawOval(1,30,0,9); g.drawOval(4,30,9,0); show("oval",image); g.dispose();
        g=reset(image); g.drawArc(1,1,24,19,30,230); g.fillArc(30,2,24,17,40,-200); g.drawArc(1,30,18,17,100,400); g.fillArc(30,30,18,17,0,360); show("arc",image); g.dispose();
        int[] xs={3,28,14,4},ys={2,4,28,13}; g=reset(image); g.drawPolygon(xs,ys,4); g.translate(30,0); g.fillPolygon(xs,ys,4); g.translate(-30,30); g.drawPolyline(xs,ys,4); show("polygon",image); g.dispose();
        g=reset(image); g.fillRect(1,1,12,8); g.copyArea(1,1,12,8,6,3); g.setClip(23,3,5,8); g.copyArea(1,1,12,8,20,0); show("copy",image); g.dispose();
        g=reset(image); g.fillRect(2,2,20,20); g.setXORMode(Color.white); g.fillRect(10,10,20,20); g.drawRect(34,3,15,13); g.drawLine(33,25,48,40); show("xor",image); g.dispose();
        for(int i=0;i<30;i++) { Graphics a=image.getGraphics(),b=a.create(); a.dispose(); System.gc(); b.drawLine(0,0,1,1); b.dispose(); }
        f.dispose(); image.flush(); System.out.println("primitive raster and resource retirement passed"); System.exit(0);
      } catch(Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
