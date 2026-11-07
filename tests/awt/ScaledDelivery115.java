import java.awt.*;import java.awt.image.*;
public class ScaledDelivery115 {
 public static void main(String[] args){try{
  Frame frame=new Frame();frame.show();Image image=frame.createImage(4,4);
  sun.awt.image.ImageRepresentation r=new sun.awt.image.ImageRepresentation((sun.awt.image.Image)image,1,1,0);r.setDimensions(4,4);
  ColorModel model=ColorModel.getRGBdefault();int[] pixel={0xff000000,0xff000000,0xff000000,0xff000000};
  r.setPixels(0,0,1,1,model,pixel,0,1);if((r.check(null)&8)!=0)throw new RuntimeException("unmapped source pixels marked delivered");
  r.setPixels(2,2,1,1,model,pixel,0,1);if((r.check(null)&8)==0)throw new RuntimeException("mapped source pixel not delivered");
  r.imageComplete(ImageConsumer.STATICIMAGEDONE);image.flush();frame.dispose();System.out.println("centered scaled delivery no-op and update status passed");System.exit(0);
 }catch(Throwable t){t.printStackTrace();System.exit(1);}}
}

