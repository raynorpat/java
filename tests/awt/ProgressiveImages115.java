import java.awt.*; import java.awt.image.*; import java.util.*;
public class ProgressiveImages115 implements ImageProducer {
 int hints; ProgressiveImages115(int h){hints=h;} ImageConsumer consumer;ColorModel model=ColorModel.getRGBdefault();
 public void addConsumer(ImageConsumer c){consumer=c;} public boolean isConsumer(ImageConsumer c){return consumer==c;}public void removeConsumer(ImageConsumer c){if(c==consumer)consumer=null;}
 public void requestTopDownLeftRightResend(ImageConsumer c){}
 public void startProduction(ImageConsumer c){addConsumer(c);c.setDimensions(4,4);c.setProperties(new Hashtable());c.setColorModel(model);c.setHints(hints);c.setPixels(0,1,4,1,model,new int[]{0xff000000,0xff000000,0xff000000,0xff000000},0,4);}
 static int hash(Image i) throws Exception{int[] p=new int[64];new PixelGrabber(i,0,0,8,8,p,0,8).grabPixels();int h=0;for(int n=0;n<64;n++)h=h*31+(p[n]&0xffffff);return h;}
 public static void main(String[] a){try{Frame f=new Frame();f.show();for(int hints=0;hints<=4;hints+=4){ ProgressiveImages115 producer=new ProgressiveImages115(hints);Image src=f.createImage(producer);f.prepareImage(src,null);Image dst=f.createImage(8,8);Graphics g=dst.getGraphics();g.setColor(Color.white);g.fillRect(0,0,8,8);g.drawImage(src,1,1,null);System.out.println("partial "+hash(dst));
 producer.consumer.setPixels(0,3,4,1,producer.model,new int[]{0xff0000ff,0xff0000ff,0xff0000ff,0xff0000ff},0,4);g.drawImage(src,1,1,null);System.out.println("partial2 "+hash(dst));
 producer.consumer.imageComplete(ImageConsumer.STATICIMAGEDONE);g.drawImage(src,1,1,null);System.out.println("complete "+hash(dst));
 g.dispose();dst.flush();src.flush();} f.dispose();System.out.println("partial image delivery and completion passed");System.exit(0);
 }catch(Throwable t){t.printStackTrace();System.exit(1);}}
}

