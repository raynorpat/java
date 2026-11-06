public class Numeric115 {
    static void parse(String label, String value) {
        try {
            System.out.println(label + "=" + Double.valueOf(value).doubleValue());
        } catch (Throwable error) {
            System.out.println(label + "=" + error.getClass().getName());
        }
    }
    public static void main(String[] args) {
        parse("decimal", "1.25");
        parse("signed", " \t\n\f\r-1.25e+2D \t");
        parse("fraction", ".5f");
        parse("point", "1.");
        parse("negative-zero", "-0.0");
        parse("long", "00000000000000000000000000000000000000000000000000000000000000000001.25");
        parse("overflow", "1e400");
        parse("underflow", "1e-400");
        parse("null", null);
        parse("empty", "");
        parse("junk", "1.25xyz");
        parse("exponent", "1e+");
        parse("suffix", "1ff");
        parse("vertical-tab", "\u000b1");
        parse("nul", "1\u0000");
        parse("unicode", "\u0131");
        parse("nan", "NaN");
        parse("infinity", "Infinity");
        parse("hex", "0x1p0");
        System.out.println("float=" + Float.valueOf("1.25f").floatValue());
    }
}
