import java.awt.*;
public class Fonts115 {
    public static void main(String[] args) {
        try { run(); } catch(Throwable error) { error.printStackTrace(); System.exit(1); }
    }
    static void run() {
        System.getProperties().put("awt.toolkit", "sun.awt.motif.MToolkit");
        Toolkit toolkit = Toolkit.getDefaultToolkit();
        String[] names = { "Dialog", "Helvetica", "TimesRoman", "Courier", "DialogInput" };
        for (int i=0; i<names.length; i++) for (int style=0; style<4; style++) {
            Font font = new Font(names[i], style, 12);
            FontMetrics metrics = toolkit.getFontMetrics(font);
            if (metrics.getAscent() <= 0 || metrics.getDescent() < 0 || metrics.getMaxAdvance() <= 0)
                throw new RuntimeException("metrics " + font);
            if (metrics.getHeight() != metrics.getAscent()+metrics.getDescent()+metrics.getLeading())
                throw new RuntimeException("height");
            byte[] bytes = {65, 66, 67};
            if (metrics.bytesWidth(bytes,0,3) != metrics.stringWidth("ABC"))
                throw new RuntimeException("width "+font+" bytes="+metrics.bytesWidth(bytes,0,3)+" string="+metrics.stringWidth("ABC"));
            if (metrics.bytesWidth(bytes,3,0) != 0) throw new RuntimeException("empty width");
            try { metrics.bytesWidth(bytes,2,2); throw new RuntimeException("bounds accepted"); }
            catch (ArrayIndexOutOfBoundsException expected) {}
            try { metrics.bytesWidth(null,0,0); throw new RuntimeException("null accepted"); }
            catch (NullPointerException expected) {}
        }
        System.gc();
        System.out.println("font metrics and byte bounds passed");
        System.exit(0);
    }
}
