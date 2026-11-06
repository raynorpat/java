public class Intern115 {
    static class Other { static String value() { return "pool115"; } }
    public static void main(String[] args) {
        String original = "pool115";
        System.out.println("literal=" + (original == Other.value()));
        String copy = new String(original);
        System.out.println("copy=" + (copy != original && copy.intern() == original));
        String part = "xxpool115yy".substring(2, 9);
        System.out.println("slice=" + (part.intern() == original));
        String text = new String(new char[] { '\u0000', '\u00e9', '\u4e2d' });
        System.out.println("unicode=" + (text.intern() == "\u0000\u00e9\u4e2d".intern()));
        String saved = new String(new char[] { 'u', 'n', 'i', 'q' }).intern();
        System.gc();
        System.out.println("gc=" + (saved == new String(saved).intern()));
    }
}
