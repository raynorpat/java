import java.awt.*;
import java.awt.image.*;
public class FrameIcon115 {
    static Image image(Frame frame,Color color) {
        int[] pixels=new int[32*32];for(int i=0;i<pixels.length;i++)pixels[i]=color.getRGB();
        return Toolkit.getDefaultToolkit().createImage(new MemoryImageSource(32,32,pixels,0,32));
    }
    public static void main(String[] args) {
        try {
            Frame frame=new Frame("FrameIcon115");frame.setBounds(40,40,180,120);frame.show();
            if (RemainingProbe115.exports(1)!=1) throw new RuntimeException("missing icon export");
            Image red=image(frame,Color.red);
            frame.setIconImage(red);Thread.sleep(100);
            int pixel=RemainingProbe115.icon("FrameIcon115");
            if (pixel!=255) throw new RuntimeException("red icon pixels "+Integer.toHexString(pixel));
            int before=RemainingProbe115.resources();
            Image[] replacements=new Image[40];
            for(int i=0;i<40;i++) {replacements[i]=image(frame,(i&1)==0?Color.blue:Color.red);frame.setIconImage(replacements[i]);Thread.sleep(2);}
            Thread.sleep(100);
            if(RemainingProbe115.icon("FrameIcon115")!=255) throw new RuntimeException("icon replacement pixels");
            if (System.getProperty("awt.correctedBounds")!=null && RemainingProbe115.resources()>before+2)
                throw new RuntimeException("icon replacement resource leak");
            if(System.getProperty("awt.correctedBounds")!=null) {
                frame.setIconImage(red);Thread.sleep(20);
                if(RemainingProbe115.icon("FrameIcon115")!=255) throw new RuntimeException("reused image icon");
            }
            frame.dispose();red.flush();for(int i=0;i<40;i++)replacements[i].flush();System.gc();
            System.out.println("frame icon pixels, replacement and disposal passed");System.exit(0);
        } catch(Throwable t) {t.printStackTrace();System.exit(1);}
    }
}
