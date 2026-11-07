import java.awt.*;
import java.awt.image.*;
import java.util.Hashtable;

public class Image115 {
    static class Incremental implements ImageProducer {
        ImageConsumer consumer;
        public void addConsumer(ImageConsumer value) { consumer=value; }
        public boolean isConsumer(ImageConsumer value) { return consumer==value; }
        public void removeConsumer(ImageConsumer value) { if (consumer==value) consumer=null; }
        public void startProduction(ImageConsumer value) {
            addConsumer(value); value.setDimensions(2,4);
            value.setHints(ImageConsumer.TOPDOWNLEFTRIGHT|ImageConsumer.COMPLETESCANLINES);
            value.setProperties(new Hashtable());
        }
        public void requestTopDownLeftRightResend(ImageConsumer value) {}
    }
    static class FlushingColorModel extends ColorModel {
        Image image; int calls;
        FlushingColorModel(Image value) { super(32); image=value; }
        public int getRed(int pixel) { return 255; }
        public int getGreen(int pixel) { return 0; }
        public int getBlue(int pixel) { return 0; }
        public int getAlpha(int pixel) { return 255; }
        public int getRGB(int pixel) { calls++; image.flush(); System.gc(); return 0xffff0000; }
    }
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
        int[] input={0xffff0000,0xff00ff00,0xff0000ff,0xffffffff};
        Image produced=Toolkit.getDefaultToolkit().createImage(new MemoryImageSource(2,2,input,0,2));
        MediaTracker tracker=new MediaTracker(canvas); tracker.addImage(produced,0); tracker.waitForAll(2000);
        check(tracker.checkAll() && !tracker.isErrorAny(),"producer image completion");
        Graphics output=canvas.getGraphics(); output.drawImage(produced,60,60,null);
        check((Window115.pixel(canvas,60,60)&0xffffff)==0xff0000,"producer red pixel");
        check((Window115.pixel(canvas,61,61)&0xffffff)==0xffffff,"producer white pixel");
        output.setColor(Color.yellow); output.fillRect(70,60,8,8);
        byte[] indices={0,1,1,0}; byte[] red={(byte)255,0}, green={0,(byte)255}, blue={0,0};
        IndexColorModel palette=new IndexColorModel(8,2,red,green,blue,1);
        Image transparent=Toolkit.getDefaultToolkit().createImage(new MemoryImageSource(2,2,palette,indices,0,2));
        tracker.addImage(transparent,1); tracker.waitForAll(2000);
        check(!tracker.isErrorAny(),"indexed image completion");
        output.drawImage(transparent,70,60,null);
        check((Window115.pixel(canvas,70,60)&0xffffff)==0xff0000,"indexed red pixel");
        check((Window115.pixel(canvas,71,60)&0xffffff)==0xffff00,"transparent pixel preserves destination");
        output.drawImage(transparent,74,60,Color.blue,null);
        check((Window115.pixel(canvas,75,60)&0xffffff)==0x0000ff,"transparent background pixel");
        output.setXORMode(Color.green); output.drawImage(produced,60,60,null);
        check((Window115.pixel(canvas,60,60)&0xffffff)==0x00ff00,"image XOR color");
        output.drawImage(produced,60,60,null); output.setPaintMode();
        check((Window115.pixel(canvas,60,60)&0xffffff)==0xff0000,"image XOR reversible");
        output.drawImage(produced,80,60,84,64,0,0,2,2,null);
        check((Window115.pixel(canvas,81,61)&0xffffff)==0xff0000,"scaled upper pixel");
        check((Window115.pixel(canvas,82,63)&0xffffff)==0xffffff,"scaled lower pixel");
        output.drawImage(produced,90,60,94,64,2,0,0,2,null);
        check((Window115.pixel(canvas,90,60)&0xffffff)==0x00ff00,"flipped source pixel");
        output.drawImage(produced,104,64,100,60,0,0,2,2,null);
        check((Window115.pixel(canvas,100,60)&0xffffff)==0xffffff,"flipped destination pixel");
        Incremental source=new Incremental(); Image partial=Toolkit.getDefaultToolkit().createImage(source);
        output.setColor(Color.yellow); output.fillRect(0,70,20,10);
        output.drawImage(partial,0,70,null);
        check(source.consumer!=null,"incremental producer starts");
        source.consumer.setPixels(0,2,2,1,ColorModel.getRGBdefault(),new int[]{0xff0000ff,0xff0000ff},0,2);
        output.drawImage(partial,0,70,null);
        check((Window115.pixel(canvas,0,70)&0xffffff)==0x0000ff,"incremental row replication");
        check((Window115.pixel(canvas,0,73)&0xffffff)==0xffff00,"undelivered row remains untouched");
        try {
            source.consumer.setPixels(0,0,2,2,ColorModel.getRGBdefault(),new int[3],0,2);
            throw new RuntimeException("short pixel array accepted");
        } catch (ArrayIndexOutOfBoundsException expected) {}
        ImageConsumer active=source.consumer;
        FlushingColorModel flushing=new FlushingColorModel(partial);
        try {
            active.setPixels(0,0,2,1,flushing,new int[]{1,2},0,2);
            throw new RuntimeException("flushed representation retained newbits");
        } catch (NullPointerException expected) {
            // The unchanged ImageRepresentation reads newbits after flush cleared it.
        }
        check(flushing.calls==1,"conversion stops after callback flush");
        output.dispose(); produced.flush(); transparent.flush(); partial.flush();
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
