import java.awt.*;
import java.awt.image.*;

public class Codec115 {
    static byte[] bytes(String hex) {
        byte[] data=new byte[hex.length()/2];
        for (int i=0;i<data.length;i++) data[i]=(byte)Integer.parseInt(hex.substring(i*2,i*2+2),16);
        return data;
    }
    public static void main(String[] args) throws Exception {
        try { run(); } catch (Throwable error) { error.printStackTrace(); System.exit(1); }
    }
    static void run() throws Exception {
        System.getProperties().put("awt.toolkit","sun.awt.motif.MToolkit");
        Frame frame=new Frame(); frame.setSize(100,100); frame.show();
        Image source=Toolkit.getDefaultToolkit().createImage(bytes("47494638396101000100800000000000ffffff21f90401000000002c00000000010001000002024401003b"));
        MediaTracker tracker=new MediaTracker(frame); tracker.addImage(source,0); tracker.waitForAll(2000);
        if (!tracker.checkAll() || tracker.isErrorAny()) throw new RuntimeException("GIF completion");
        Image target=frame.createImage(1,1); Graphics graphics=target.getGraphics();
        graphics.setColor(Color.red); graphics.fillRect(0,0,1,1); graphics.drawImage(source,0,0,null); graphics.dispose();
        int[] pixels=new int[1];
        if (!new PixelGrabber(target,0,0,1,1,pixels,0,1).grabPixels(2000) || (pixels[0]&0xffffff)!=0xff0000)
            throw new RuntimeException("GIF transparency");
        target.flush(); source.flush(); frame.dispose();
        System.out.println("GIF decoding and transparency passed"); System.exit(0);
    }
}
