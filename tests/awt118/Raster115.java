import java.awt.*;

public class Raster115 {
    static native int[] pixels(Canvas canvas,int x,int y,int width,int height);
    static native void singleFont(Graphics graphics,char[] chars,int offset,int length,int x,int y);
    static void pixel(Canvas canvas,int x,int y,int rgb) {
        int actual=Window115.pixel(canvas,x,y)&0xffffff;
        if (actual!=rgb) throw new RuntimeException("pixel "+x+","+y+": "+Integer.toHexString(actual)+" expected "+Integer.toHexString(rgb));
    }
    public static void main(String[] args) throws Exception {
        try { run(); } catch (Throwable error) { error.printStackTrace(); System.exit(1); }
    }
    static void run() throws Exception {
        System.getProperties().put("awt.toolkit","sun.awt.motif.MToolkit");
        Toolkit.getDefaultToolkit(); System.loadLibrary("motif118xprobe");
        Frame frame=new Frame("Raster"); Canvas canvas=new Canvas();
        frame.setLayout(null); canvas.setBounds(0,0,240,190); canvas.setBackground(Color.white);
        frame.add(canvas); frame.setSize(240,190); frame.show(); Thread.sleep(400);
        for (int cycle=0;cycle<5;cycle++) {
            Graphics g=canvas.getGraphics(); g.setColor(Color.white); g.fillRect(0,0,240,190);
            g.setColor(Color.red); g.drawLine(2,2,20,2); pixel(canvas,10,2,0xff0000);
            g.drawRect(2,5,15,10); pixel(canvas,17,15,0xff0000); pixel(canvas,10,10,0xffffff);
            g.setColor(Color.blue); g.fillRect(25,2,10,10); g.copyArea(25,2,10,10,15,0); pixel(canvas,44,6,0x0000ff);
            g.setXORMode(Color.white); g.fillRect(25,2,10,10); pixel(canvas,29,6,0xffffff);
            g.fillRect(25,2,10,10); pixel(canvas,29,6,0x0000ff); g.setPaintMode();
            g.setColor(Color.black); g.drawOval(2,25,20,16); pixel(canvas,12,25,0); pixel(canvas,12,33,0xffffff);
            g.fillOval(30,25,20,16); pixel(canvas,40,33,0); pixel(canvas,30,25,0xffffff);
            g.drawArc(60,25,20,20,0,90); pixel(canvas,70,25,0); pixel(canvas,60,35,0xffffff);
            g.fillArc(90,25,20,20,0,90); pixel(canvas,103,32,0); pixel(canvas,97,38,0xffffff);
            g.drawRoundRect(2,55,30,20,10,10); pixel(canvas,17,55,0); pixel(canvas,2,55,0xffffff);
            g.fillRoundRect(40,55,30,20,10,10); pixel(canvas,55,65,0); pixel(canvas,40,55,0xffffff);
            g.drawPolygon(new int[]{80,100,90},new int[]{55,55,75},3); pixel(canvas,90,55,0);
            g.fillPolygon(new int[]{110,130,120},new int[]{55,55,75},3); pixel(canvas,120,60,0);
            g.drawPolyline(new int[]{140,160,160},new int[]{55,55,75},3); pixel(canvas,150,55,0);
            try { g.drawPolygon(new int[1],new int[1],2); throw new RuntimeException("polygon bounds"); }
            catch (IllegalArgumentException expected) {}
            try { g.fillPolygon(null,new int[1],1); throw new RuntimeException("polygon null"); }
            catch (NullPointerException expected) {}
            Graphics copy=g.create(); copy.translate(10,90); copy.setClip(0,0,10,10); copy.setColor(Color.green);
            copy.fillOval(-5,-5,20,20); pixel(canvas,15,95,0x00ff00); pixel(canvas,21,95,0xffffff);
            copy.dispose(); g.dispose(); System.gc();
        }
        Graphics text=canvas.getGraphics(); text.setColor(Color.white); text.fillRect(0,0,240,190);
        text.setFont(new Font("Monospaced",Font.PLAIN,12)); text.setColor(Color.black);
        text.drawString("Hello",5,25);
        text.drawChars(new char[]{'x','H','e','l','l','o','x'},1,5,5,55);
        text.drawBytes(new byte[]{0,72,101,108,108,111,0},1,5,5,85);
        singleFont(text,new char[]{'x','H','e','l','l','o','x'},1,5,5,115);
        int[] first=pixels(canvas,0,5,80,25),second=pixels(canvas,0,35,80,25),third=pixels(canvas,0,65,80,25),fourth=pixels(canvas,0,95,80,25);
        int ink=0;
        for (int i=0;i<first.length;i++) {
            if (first[i]!=second[i] || first[i]!=third[i] || first[i]!=fourth[i]) throw new RuntimeException("text raster mismatch");
            if ((first[i]&0xffffff)==0) ink++;
        }
        if (ink==0) throw new RuntimeException("missing text raster");
        try { text.drawBytes(new byte[2],1,Integer.MAX_VALUE,0,0); throw new RuntimeException("text bounds"); }
        catch (ArrayIndexOutOfBoundsException expected) {}
        try { text.drawChars(null,0,1,0,0); throw new RuntimeException("text null"); }
        catch (NullPointerException expected) {}
        text.setClip(1,2,-1,8);
        if (!text.getClipBounds().equals(new Rectangle(1,2,0,0))) throw new RuntimeException("empty clip dimensions "+text.getClipBounds());
        text.setClip(65535,65535,65537,65536);
        if (!text.getClipBounds().equals(new Rectangle(-1,-1,1,0))) throw new RuntimeException("X11 clip bounds "+text.getClipBounds());
        text.dispose();
        frame.dispose();
        System.out.println("graphics primitives, text, clipping, XOR and copying passed"); System.exit(0);
    }
}
