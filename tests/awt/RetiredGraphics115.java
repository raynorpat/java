import java.awt.*;
public class RetiredGraphics115 {
 public static void main(String[] args){try{
  if(System.getProperty("awt.correctedBounds")!=null){
   Frame frame=new Frame();Canvas canvas=new Canvas();frame.add(canvas);frame.show();
   java.awt.peer.ComponentPeer peer=canvas.getPeer();canvas.removeNotify();System.gc();
   Graphics g=sun.awt.windows.HDCAccess115.create(peer);g.setColor(Color.blue);g.drawLine(0,0,2,2);g.fillRect(0,0,2,2);g.dispose();g.dispose();frame.dispose();
   try{sun.awt.windows.HDCAccess115.create((java.awt.peer.ComponentPeer)null);throw new RuntimeException("null peer accepted");}catch(NullPointerException expected){}
  }
  System.out.println("retired component graphics are inert and null sources fail passed");System.exit(0);
 }catch(Throwable t){t.printStackTrace();System.exit(1);}}
}
