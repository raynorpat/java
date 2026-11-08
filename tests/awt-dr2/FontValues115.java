import java.awt.Font;
import java.awt.Toolkit;

public class FontValues115 {
    static { System.loadLibrary("fontvalues115"); }
    static native String describe(Font font);
    public static void main(String[] args) {
        try {
            Toolkit.getDefaultToolkit();
            if (describe(null) != null) throw new Error("null font conversion");
            String[] names = {"TimesRoman", "Helvetica", "Courier", "ZapfDingbats", "Serif", "SansSerif", "Monospaced", "Dialog", "DialogInput", "dialog", "MissingFont115"};
            for (int n = 0; n < names.length; n++) {
                for (int style = 0; style < 4; style++) {
                    for (int size = 9; size <= 18; size += 9) {
                        String value = describe(new Font(names[n], style, size));
                        if (value == null) throw new Error("font conversion failed");
                        System.out.println(names[n] + ":" + style + ":" + size + "=" + value);
                    }
                }
                System.gc();
            }
            String[] families = Toolkit.getDefaultToolkit().getFontList();
            if (families.length == 0) throw new Error("empty font list");
            for (int i = 0; i < families.length; i++)
                System.out.println("family=" + families[i]);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
        System.exit(0);
    }
}
