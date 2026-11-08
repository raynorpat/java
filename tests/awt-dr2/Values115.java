import java.awt.Color;
public class Values115 {
    static { System.loadLibrary("values115"); }
    static native boolean stringMatches(String value);
    static native int colorRGB(Color value);
    public static void main(String[] args) {
        String[] strings = {null, "", "Frame title", "a\u0000b", "\u00e9\u03bb\u4e2d", "\ud83d\ude00", "\ud800"};
        for (int cycle = 0; cycle < 20; cycle++) {
            for (int i = 0; i < strings.length; i++)
                if (!stringMatches(strings[i])) throw new Error("string conversion " + i);
            for (int i = 0; i < 256; i++) {
                int rgb = (i << 16) | ((255 - i) << 8) | ((i * 37) & 255);
                if (colorRGB(new Color(rgb)) != rgb) throw new Error("color conversion " + rgb);
            }
            System.gc();
        }
        if (colorRGB(null) != -1) throw new Error("null color conversion");
        System.out.println("DR2 Java strings and colors preserve native values");
    }
}
