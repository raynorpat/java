import java.awt.*;

public class Fonts115 {
    public static void main(String[] args) {
        try {
            Toolkit toolkit = Toolkit.getDefaultToolkit();
            String[] names = {"Dialog","DialogInput","Serif","SansSerif","Monospaced","Helvetica","Courier"};
            for (int n = 0; n < names.length; n++) for (int style = 0; style < 4; style++) {
                Font font = new Font(names[n],style,14);
                FontMetrics m = toolkit.getFontMetrics(font);
                int width = m.stringWidth("Hello \u00e9\u03a9");
                if (m.getAscent() <= 0 || m.getDescent() < 0 || width <= 0 ||
                    m.getHeight() != m.getAscent()+m.getDescent()+m.getLeading())
                    throw new RuntimeException("invalid font metrics");
                int[] widths = m.getWidths();
                if (widths.length != 256) throw new RuntimeException("width array");
                byte[] bytes = {65,66,67};
                if (m.bytesWidth(bytes,0,3) != widths[65]+widths[66]+widths[67])
                    throw new RuntimeException("byte metrics");
                try { m.bytesWidth(bytes,2,2); throw new RuntimeException("missing range error"); }
                catch (ArrayIndexOutOfBoundsException expected) {}
                try { m.bytesWidth(null,0,1); throw new RuntimeException("missing null error"); }
                catch (NullPointerException expected) {}
                if (System.getProperty("awt.correctedBounds") != null) {
                    if (m.bytesWidth(new byte[] {(byte)233},0,1) != widths[233])
                        throw new RuntimeException("signed byte table index");
                    try { m.bytesWidth(bytes,1,Integer.MAX_VALUE); throw new RuntimeException("overflow range"); }
                    catch (ArrayIndexOutOfBoundsException expected) {}
                }
                System.out.println(names[n]+" "+style+" "+m.getAscent()+" "+m.getDescent()+" "+
                    m.getLeading()+" "+m.getMaxAscent()+" "+m.getMaxDescent()+" "+m.getMaxAdvance()+" "+width);
                System.gc();
            }
            String[] faces = {"Arial,ANSI_CHARSET","Courier New,ANSI_CHARSET"};
            char[] characters = {0,32,65,127,233,0x03a9,0x2500,0x2751,0x4e00,0xffff};
            for (int n = 0; n < faces.length; n++) {
                sun.awt.windows.WDefaultFontCharset charset = new sun.awt.windows.WDefaultFontCharset(faces[n]);
                String coverage = "";
                for (int i = 0; i < characters.length; i++) coverage += charset.canConvert(characters[i]) ? "1" : "0";
                System.out.println(faces[n]+" "+coverage);
            }
            if (System.getProperty("awt.correctedBounds") != null) {
                sun.awt.windows.WDefaultFontCharset symbol = new sun.awt.windows.WDefaultFontCharset("Symbol,SYMBOL_CHARSET");
                if (symbol.canConvert((char)0) || symbol.canConvert('A'))
                    throw new RuntimeException("missing Unicode cmap accepted");
            }
            System.out.println("font metrics, Unicode segments and bounds passed"); System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
