import java.awt.*;
public class GraphicsHDC115 {
 static {System.loadLibrary("awtgraphics115probe");}
 static native int create();static native int pixel(int x,int y);static native boolean close();
 public static void main(String[] a){try{Frame f=new Frame();f.show();Graphics g=sun.awt.windows.HDCAccess115.create(create());
 // Sun 10011CBD leaves its Graphics peer pointer null; drawing dereferences it.
 // Exercise the corrected drawing path only, and replay safe disposal on Sun.
 if(System.getProperty("awt.correctedBounds")!=null){g.setColor(Color.red);g.fillRect(1,1,3,3);g.setClip(6,6,2,2);g.setColor(Color.blue);g.fillRect(4,4,8,8);
 if(pixel(1,1)!=0xff || pixel(6,6)!=0xff0000 || pixel(5,5)!=0xffffff)throw new RuntimeException("HDC color or clip");
 Graphics c=g.create();g.dispose();System.gc();c.setClip(null);c.setColor(Color.green);c.drawLine(10,10,12,10);c.dispose();if(pixel(12,10)!=0xff00)throw new RuntimeException("copy HDC lifetime");
 if(!close())throw new RuntimeException("graphics disposed borrowed HDC");}else{g.dispose();close();}f.dispose();System.out.println("borrowed HDC graphics, true colors, clipping and copy lifetime passed");System.exit(0);
 }catch(Throwable t){t.printStackTrace();System.exit(1);}}
}

