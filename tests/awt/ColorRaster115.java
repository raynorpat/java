import java.awt.*; import java.awt.image.*;
public class ColorRaster115 {
 public static void main(String[] args) { try {
  Frame f=new Frame(); f.show(); Image i=f.createImage(16,16); Graphics g=i.getGraphics();
  Color[] cs={Color.black,Color.white,Color.red,Color.green,Color.blue,Color.yellow,Color.cyan,Color.magenta,new Color(0x123456),new Color(0x808080)};
  for(int n=0;n<cs.length;n++){g.setColor(cs[n]);g.fillRect(n,0,1,16);}
  int[] p=new int[256]; new PixelGrabber(i,0,0,16,16,p,0,16).grabPixels();
  for(int n=0;n<cs.length;n++)System.out.println(Integer.toHexString(cs[n].getRGB())+" "+Integer.toHexString(p[n]));
  f.dispose(); System.out.println("color raster passed");System.exit(0);
 }catch(Throwable t){t.printStackTrace();System.exit(1);} }
}
