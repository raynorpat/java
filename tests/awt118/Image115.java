import java.awt.*;
import java.awt.image.*;
import java.util.Hashtable;

public class Image115 {
    static class RemovingConsumer implements ImageConsumer {
        Image image; ImageProducer producer; int rows;
        RemovingConsumer(Image value) { image=value; producer=image.getSource(); }
        public void setDimensions(int width,int height) {}
        public void setProperties(Hashtable properties) {}
        public void setColorModel(ColorModel model) {}
        public void setHints(int hints) { check(hints==14,"offscreen delivery hints"); }
        public void imageComplete(int status) {}
        void row() { rows++; producer.removeConsumer(this); image.flush(); System.gc(); }
        public void setPixels(int x,int y,int width,int height,ColorModel model,byte[] pixels,int offset,int scan) { row(); }
        public void setPixels(int x,int y,int width,int height,ColorModel model,int[] pixels,int offset,int scan) { row(); }
    }
    static void check(boolean condition,String message) { if (!condition) throw new RuntimeException(message); }
    public static void main(String[] args) throws Exception {
        try { run(); } catch (Throwable error) { error.printStackTrace(); System.exit(1); }
    }
    static void run() throws Exception {
        System.getProperties().put("awt.toolkit","sun.awt.motif.MToolkit");
        Toolkit.getDefaultToolkit(); System.loadLibrary("motif118xprobe");
        Frame frame=new Frame("Images"); Canvas canvas=new Canvas();
        canvas.setBackground(Color.white); frame.setLayout(null); canvas.setBounds(0,0,120,100);
        frame.add(canvas); frame.setSize(120,100); frame.show(); Thread.sleep(400);
        for (int cycle=0;cycle<8;cycle++) {
            Image image=canvas.createImage(8,6);
            Graphics offscreen=image.getGraphics();
            offscreen.setColor(Color.red); offscreen.fillRect(0,0,8,6);
            offscreen.setColor(Color.blue); offscreen.fillRect(2,2,3,2);
            Graphics copy=offscreen.create(); copy.translate(1,1); copy.setColor(Color.green); copy.fillRect(0,0,1,1);
            copy.dispose();
            int[] pixels=new int[48];
            PixelGrabber grabber=new PixelGrabber(image,0,0,8,6,pixels,0,8);
            check(grabber.grabPixels(2000),"offscreen pixel delivery");
            check((pixels[0]&0xffffff)==0xff0000 && (pixels[9]&0xffffff)==0x00ff00 &&
                (pixels[18]&0xffffff)==0x0000ff,"offscreen pixel colors");
            Graphics screen=canvas.getGraphics(); screen.setColor(Color.white); screen.fillRect(0,0,120,100);
            check(screen.drawImage(image,10,10,null),"offscreen image completeness");
            check((Window115.pixel(canvas,10,10)&0xffffff)==0xff0000,"image copy");
            check((Window115.pixel(canvas,12,12)&0xffffff)==0x0000ff,"image copy interior");
            offscreen.clearRect(0,0,1,1); screen.drawImage(image,30,10,null);
            check((Window115.pixel(canvas,30,10)&0xffffff)==0xffffff,"offscreen background");
            RemovingConsumer consumer=new RemovingConsumer(image); consumer.producer.startProduction(consumer);
            check(consumer.rows==1,"consumer removal during delivery");
            image.flush(); offscreen.fillRect(0,0,8,6); offscreen.dispose(); screen.dispose(); System.gc();
        }
        frame.dispose();
        System.out.println("offscreen image painting, delivery and retirement passed"); System.exit(0);
    }
}
