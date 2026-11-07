import java.awt.*;
public class GraphicsResources115 {
 static {System.loadLibrary("awtgraphics115probe");}
 static native int gdiObjects();
 public static void main(String[] a){try{Frame f=new Frame();f.show();for(int n=0;n<8;n++){Image warm=f.createImage(8,8);Graphics g=warm.getGraphics();g.setFont(new Font("Dialog",n%4,12));g.drawString("A\u03a9",0,5);g.drawLine(0,0,3,3);g.dispose();warm.flush();}System.gc();Thread.sleep(50);
 int before=gdiObjects();for(int n=0;n<100;n++){
  Image i=f.createImage(32,32);Graphics first=i.getGraphics(),second=first.create();first.dispose();System.gc();second.setFont(new Font("Dialog",n%4,12));second.drawString("A\u03a9",0,18);i.flush();second.drawLine(0,0,3,3);second.dispose();second.dispose();
 }System.gc();Thread.sleep(50);int after=gdiObjects();if(after>before+4)throw new RuntimeException("GDI leak "+before+" -> "+after);
 f.dispose();System.out.println("100 offscreen surfaces, graphics copies, font draws and GC resource counts passed");System.exit(0);
 }catch(Throwable t){t.printStackTrace();System.exit(1);}}
}

