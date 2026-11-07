import java.awt.*; import java.awt.image.*;
public class ImageRaster115 {
 static void show(String name,Image i) throws Exception { int[] p=new int[64*64]; if(!new PixelGrabber(i,0,0,64,64,p,0,64).grabPixels(5000)) throw new RuntimeException("pixels");int h=0,c=0;for(int n=0;n<p.length;n++){h=h*31+(p[n]&0xffffff);if((p[n]&0xffffff)!=0xffffff)c++;}System.out.println(name+" "+h+" "+c); }
 static Image source(Frame f,ImageProducer p) throws Exception {Image i=f.createImage(p);MediaTracker t=new MediaTracker(f);t.addImage(i,1);t.waitForAll();if(t.isErrorAny())throw new RuntimeException("producer");return i;}
 static Graphics reset(Image i){Graphics g=i.getGraphics();g.setColor(Color.white);g.fillRect(0,0,64,64);return g;}
 public static void main(String[] a){try{
 Frame f=new Frame();f.setSize(100,100);f.show();Image dst=f.createImage(64,64);
 int[] p={0xff000000,0xffffffff,0xff0000ff,0xff112233,0xffabcdef,0xff00ff00,0xff000000,0xffc08040,0xff404040,0xff808080,0xff00ffff,0xffff00ff,0xffffff00,0xff556677,0xff999999};
 Image i=source(f,new MemoryImageSource(5,3,p,0,5));Graphics g=reset(dst);g.drawImage(i,2,2,null);g.drawImage(i,10,3,17,11,null);show("int-scale",dst);g.dispose();
 g=reset(dst);g.drawImage(i,2,2,17,11,5,3,0,0,null);g.drawImage(i,33,22,18,13,0,0,5,3,null);show("mirror",dst);g.dispose();
 g=reset(dst);g.setClip(4,4,10,8);g.drawImage(i,0,0,21,17,-2,-1,7,4,null);show("crop",dst);g.dispose();
 byte[] r=new byte[256],gr=new byte[256],b=new byte[256],alpha=new byte[256];for(int n=0;n<256;n++){r[n]=(byte)n;gr[n]=(byte)(255-n);b[n]=(byte)(n^0x55);alpha[n]=(byte)255;}
 byte[] indices={0,1,2,(byte)200,(byte)255,4,5,6,7,8,9,10,11,12,13};
 Image indexed=source(f,new MemoryImageSource(5,3,new IndexColorModel(8,256,r,gr,b),indices,0,5));g=reset(dst);g.drawImage(indexed,2,2,null);g.drawImage(indexed,10,10,17,11,null);show("indexed",dst);g.dispose();
 int[] transparent=new int[64];for(int n=0;n<64;n++)transparent[n]=(n%6==0?0:n%6==1?64:n%6==2?127:n%6==3?128:n%6==4?192:255)<<24|0x102030;
 Image trans=source(f,new MemoryImageSource(8,8,transparent,0,8));g=reset(dst);g.drawImage(trans,2,2,null);g.drawImage(trans,15,3,19,17,null);g.drawImage(trans,2,30,Color.blue,null);show("alpha",dst);g.dispose();
 g=reset(dst);g.setColor(Color.black);g.fillRect(0,0,64,64);g.setXORMode(Color.white);g.drawImage(i,2,2,null);g.drawImage(indexed,20,2,null);g.drawImage(trans,2,20,null);show("image-xor",dst);g.dispose();
 Image off=f.createImage(5,3);g=off.getGraphics();g.setColor(Color.black);g.fillRect(0,0,5,3);g.setColor(Color.blue);g.fillRect(1,1,2,1);g.dispose();g=reset(dst);g.drawImage(off,2,2,17,11,null);show("off-scale",dst);g.dispose();
 dst.flush();f.dispose();System.out.println("scaled, indexed, ARGB, mirrored, cropped and XOR images passed");System.exit(0);
 }catch(Throwable t){t.printStackTrace();System.exit(1);}}
}
