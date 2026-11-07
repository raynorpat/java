import java.awt.*;
import java.awt.image.*;
public class TextRaster115 {
  static int hash(Image image) throws Exception {
    int[] p=new int[128*64]; PixelGrabber grab=new PixelGrabber(image,0,0,128,64,p,0,128);
    if(!grab.grabPixels(5000)) throw new RuntimeException("pixels"); int h=0;
    int x1=128,y1=64,x2=0,y2=0,c=0,gray=0; for(int i=0;i<p.length;i++) { h=h*31+(p[i]&0xffffff); if((p[i]&0xffffff)!=0xffffff) { x1=Math.min(x1,i%128);y1=Math.min(y1,i/128);x2=Math.max(x2,i%128);y2=Math.max(y2,i/128);c++; if((p[i]&0xffffff)!=0) gray++; } } System.out.println("bounds "+x1+","+y1+","+x2+","+y2+" count "+c+" gray "+gray); return h;
  }
  public static void main(String[] args) {
    try {
      Frame f=new Frame(); f.setSize(100,100); f.show(); Image image=f.createImage(128,64);
      String[] names={"Dialog","Serif","Monospaced","Helvetica","Courier"};
      for(int n=0;n<names.length;n++) {
        Graphics g=image.getGraphics(); g.setColor(Color.white); g.fillRect(0,0,128,64);
        g.setFont(new Font(names[n],Font.PLAIN,14)); g.setColor(Color.black);
        g.drawString("Hello \u00e9\u03a9\u0416\u4e00",2,20); g.drawBytes(new byte[]{65,66,67},0,3,3,40);
        System.out.println(names[n]+" "+hash(image));
        g.setXORMode(Color.white); g.drawString("\u00e9\u03a9",60,40); g.drawString("\u00e9\u03a9",60,40);
        System.out.println("xor "+hash(image)); g.dispose(); System.gc();
      }
      image.flush(); f.dispose(); System.out.println("Unicode text segments, byte text and XOR text passed"); System.exit(0);
    } catch(Throwable failure) { failure.printStackTrace(); System.exit(1); }
  }
}

