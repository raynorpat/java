import java.awt.*; import java.awt.image.*;
public class ImageBounds115 {
 public static void main(String[] a) { try {
  Frame f=new Frame();f.show();Image image=f.createImage(8,8);Graphics g=image.getGraphics();
  g.setColor(Color.white);g.fillRect(0,0,8,8);g.setColor(Color.black);
  g.fillRect(0,0,0,8);g.fillRect(0,0,-1,8);g.drawOval(0,0,-1,8);g.fillArc(0,0,8,8,0,0);g.copyArea(0,0,-1,4,1,1);
  int[] p=new int[64];new PixelGrabber(image,0,0,8,8,p,0,8).grabPixels();for(int n=0;n<64;n++)if(p[n]!=-1)throw new RuntimeException("invalid extent drew");
  try{g.drawBytes(null,0,1,0,0);throw new RuntimeException("null bytes");}catch(NullPointerException e){}
  try{g.drawBytes(new byte[1],1,1,0,0);throw new RuntimeException("byte range");}catch(ArrayIndexOutOfBoundsException e){}
  try{g.drawPolygon(null,new int[1],1);throw new RuntimeException("null polygon");}catch(NullPointerException e){}
  try{g.drawPolygon(new int[1],new int[1],2);throw new RuntimeException("polygon range");}catch(ArrayIndexOutOfBoundsException e){}
  sun.awt.image.ImageRepresentation r=new sun.awt.image.ImageRepresentation((sun.awt.image.Image)image,-1,-1,0);r.setDimensions(4,4);
  ColorModel cm=ColorModel.getRGBdefault();r.setPixels(0,0,0,4,cm,new int[0],0,0);r.setPixels(0,0,4,0,cm,new byte[0],0,0);
  try{r.setPixels(0,0,4,4,cm,new int[4],0,4);throw new RuntimeException("pixel range");}catch(ArrayIndexOutOfBoundsException e){}
  try{r.setPixels(0,0,4,4,cm,(int[])null,0,4);throw new RuntimeException("null pixels");}catch(NullPointerException e){}
  if(System.getProperty("awt.correctedBounds")!=null){
   try{r.setPixels(0,0,2,2,cm,new int[4],3,2);throw new RuntimeException("pixel offset");}catch(ArrayIndexOutOfBoundsException e){}
   try{r.setPixels(0,0,2,2,cm,new int[4],0,Integer.MAX_VALUE);throw new RuntimeException("pixel overflow");}catch(ArrayIndexOutOfBoundsException e){}
   try{g.drawPolygon(new int[1],new int[1],-1);throw new RuntimeException("negative polygon count");}catch(ArrayIndexOutOfBoundsException e){}
   try{g.drawBytes(new byte[2],1,Integer.MAX_VALUE,0,0);throw new RuntimeException("byte overflow");}catch(ArrayIndexOutOfBoundsException e){}
  }
  r.setPixels(0,0,4,4,cm,new int[16],0,4);r.imageComplete(ImageConsumer.STATICIMAGEDONE);
  Graphics copy=g.create();image.flush();g.dispose();System.gc();copy.fillRect(0,0,2,2);copy.dispose();copy.dispose();
  f.dispose();System.out.println("image and raster bounds, disposal and GC passed");System.exit(0);
 }catch(Throwable t){t.printStackTrace();System.exit(1);} }
}
