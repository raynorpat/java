import java.awt.*;
import java.awt.image.*;

public class Visual115 {
    public static void main(String[] args) throws Exception {
        try {
            Toolkit toolkit=Toolkit.getDefaultToolkit(); ColorModel model=toolkit.getColorModel();
            int depth=Integer.parseInt(args[0]);
            if(model.getPixelSize()!=depth || (depth==8 && !(model instanceof IndexColorModel))) throw new RuntimeException("visual color model");
            Frame frame=new Frame(); frame.setSize(100,100); frame.show();
            int[] colors={0xff000000,0xffffffff,0xffff0000,0xff00ff00,0xff0000ff,0xff00ffff,0xffff00ff,0xffffff00,0xff808080};
            for(int cycle=0;cycle<4;cycle++) {
                Image image=frame.createImage(colors.length,2); Graphics graphics=image.getGraphics();
                for(int i=0;i<colors.length;i++) { graphics.setColor(new Color(colors[i]));graphics.fillRect(i,0,1,1); }
                Image source=toolkit.createImage(new MemoryImageSource(colors.length,1,colors,0,colors.length));
                MediaTracker tracker=new MediaTracker(frame);tracker.addImage(source,0);tracker.waitForAll(2000);
                if(tracker.isErrorAny()) throw new RuntimeException("visual image conversion");
                graphics.drawImage(source,0,1,null); graphics.dispose();
                int[] pixels=new int[colors.length*2];
                if(!new PixelGrabber(image,0,0,colors.length,2,pixels,0,colors.length).grabPixels(2000)) throw new RuntimeException("visual delivery");
                for(int i=0;i<pixels.length;i++) for(int shift=0;shift<=16;shift+=8)
                    if(Math.abs(((pixels[i]>>shift)&255)-((colors[i%colors.length]>>shift)&255))>8) throw new RuntimeException("visual pixel "+i+" "+Integer.toHexString(pixels[i]));
                source.flush();image.flush();System.gc();
            }
            frame.dispose();System.out.println("visual color model, image conversion and delivery passed");System.exit(0);
        } catch(Throwable error) {error.printStackTrace();System.exit(1);}
    }
}
