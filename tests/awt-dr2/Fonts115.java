import java.awt.*;

public class Fonts115 {
    static void check(boolean condition) {
        if (!condition) throw new RuntimeException("Font metrics mismatch");
    }

    static void arrays(FontMetrics metrics, boolean safety) {
        char[] chars = {'x', 'A', '\u03a9', 'B', 'y'};
        byte[] bytes = {65, 0, (byte)0xa9, 3, 66, 0};
        check(metrics.charsWidth(chars, 1, 3) == metrics.bytesWidth(bytes, 0, 3));
        check(metrics.bytesWidth(new byte[] {120, 65, 66, 67, 121}, 1, 3) ==
              metrics.bytesWidth(new byte[] {65, 66, 67}, 0, 3));
        check(metrics.charsWidth(chars, chars.length, 0) == 0);
        check(metrics.bytesWidth(bytes, bytes.length, 0) == 0);
        check(metrics.stringWidth("") == 0);
        if (!safety) return;
        char[] large = new char[1024];
        byte[] storage = new byte[2048];
        for (int i = 0; i < large.length; i++) {
            large[i] = (char)(65 + i % 26);
            storage[2 * i] = (byte)large[i];
        }
        int[] lengths = {127, 128, 129, 255, 256, 257, 1023};
        for (int i = 0; i < lengths.length; i++)
            check(metrics.charsWidth(large, 1, lengths[i]) == metrics.bytesWidth(storage, 2, lengths[i]));
        int[][] ranges = {{-1, 1}, {0, -1}, {4, 2}, {6, 0}, {1, 2147483647}};
        for (int i = 0; i < ranges.length; i++) {
            try { metrics.charsWidth(chars, ranges[i][0], ranges[i][1]); throw new RuntimeException("Missing char bounds check"); }
            catch (ArrayIndexOutOfBoundsException expected) { }
            try { metrics.bytesWidth(new byte[5], ranges[i][0], ranges[i][1]); throw new RuntimeException("Missing byte bounds check"); }
            catch (ArrayIndexOutOfBoundsException expected) { }
        }
        try { metrics.charsWidth(null, 0, 0); throw new RuntimeException("Missing char null check"); }
        catch (NullPointerException expected) { }
        try { metrics.bytesWidth(null, 0, 0); throw new RuntimeException("Missing byte null check"); }
        catch (NullPointerException expected) { }
        check(metrics.stringWidth("ABC") > 0);
    }

    public static void main(String[] args) {
        try {
            Toolkit toolkit = Toolkit.getDefaultToolkit();
            String[] names = {"Serif", "SansSerif", "Monospaced", "Dialog"};
            int[] sizes = {8, 12, 17};
            for (int name = 0; name < names.length; name++) for (int style = 0; style < 4; style++) for (int size = 0; size < sizes.length; size++) {
                FontMetrics metrics = toolkit.getFontMetrics(new Font(names[name], style, sizes[size]));
                int[] widths = metrics.getWidths();
                arrays(metrics, args.length != 0);
                System.out.println(names[name] + ":" + style + ":" + sizes[size] + ":" +
                    metrics.getAscent() + "," + metrics.getDescent() + "," + metrics.getLeading() + "," + metrics.getHeight() + "," +
                    metrics.getMaxAscent() + "," + metrics.getMaxDescent() + "," + metrics.getMaxAdvance() + ":" +
                    widths.length + "," + widths[32] + "," + widths[65] + "," + widths[87] + "," + widths[105] + ":" +
                    metrics.stringWidth("AW i") + "," + metrics.stringWidth("caf\u00e9 \u03a9") + "," +
                    metrics.charsWidth(new char[] {'A', 'B', 'C'}, 0, 3) + "," + metrics.bytesWidth(new byte[] {65, 66, 67}, 0, 3));
                System.gc();
            }
            System.out.println("DR2 font metrics preserve native sizing and string widths");
            System.exit(0);
        } catch (Throwable problem) { problem.printStackTrace(); System.exit(1); }
    }
}
