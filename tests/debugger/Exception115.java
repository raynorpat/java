public class Exception115 {
    static void fail() {
        throw new IllegalArgumentException("debugger exception");
    }
    public static void main(String[] args) {
        if (args.length == 0) {
            fail();
        } else {
            try { fail(); }
            catch (IllegalArgumentException expected) {
                System.out.println("debugger exception caught");
            }
        }
    }
}
