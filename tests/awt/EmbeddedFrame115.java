import java.awt.*;
public class EmbeddedFrame115 {
    static int prints;
    public static void main(String[] args) {
        try {
            Toolkit.getDefaultToolkit();
            if (RemainingProbe115.exports(0)!=4) throw new RuntimeException("missing embedded exports");
            int host=RemainingProbe115.host();
            Frame frame=RemainingProbe115.embed(host);
            if (frame==null || frame.getPeer()==null || frame.getParent()!=null || frame.isResizable())
                throw new RuntimeException("embedded Java peer");
            if (!RemainingProbe115.child(host,true) || frame.size().width!=120 || frame.size().height!=90)
                throw new RuntimeException("embedded native parent or bounds");
            frame.setLayout(null);
            Canvas canvas=new Canvas() { public void print(Graphics g) {
                prints++; System.gc();
                if (System.getProperty("awt.correctedBounds")!=null) { g.setColor(Color.red);g.fillRect(0,0,20,20); }
            }};
            canvas.setBounds(0,0,40,40);frame.add(canvas);frame.validate();
            int dc=RemainingProbe115.dc();RemainingProbe115.print(frame,dc);
            if (prints!=1) throw new RuntimeException("host print ABI callback");
            if (System.getProperty("awt.correctedBounds")!=null && RemainingProbe115.pixel(4,4)!=255)
                throw new RuntimeException("host borrowed HDC drawing");
            if (!RemainingProbe115.closeDC()) throw new RuntimeException("borrowed DC destroyed");
            frame.dispose();frame.dispose();System.gc();
            for(int i=0;i<100 && !RemainingProbe115.child(host,false);i++) Thread.sleep(10);
            if (!RemainingProbe115.child(host,false) || !RemainingProbe115.destroy(host))
                throw new RuntimeException("embedded disposal destroyed host");
            host=RemainingProbe115.host();frame=RemainingProbe115.embed(host);
            if (!RemainingProbe115.destroy(host)) throw new RuntimeException("host destruction");
            if(System.getProperty("awt.correctedBounds")!=null) frame.dispose();
            System.out.println("embedded parent, peer, host ABI, print and disposal passed");System.exit(0);
        } catch (Throwable t) {t.printStackTrace();System.exit(1);}
    }
}
