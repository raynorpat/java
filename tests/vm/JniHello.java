public class JniHello {
    static native int add(int a, int b);
    static native String greet(String name);
    static native int callback(int value);
    static native void fail();
    static int twice(int value) { return value * 2; }
    public static void main(String[] args) {
        System.load(args[0]);
        System.out.println("add=" + add(19, 23));
        System.out.println("greet=" + greet("world"));
        System.out.println("callback=" + callback(21));
        try { fail(); }
        catch (IllegalStateException error) {
            System.out.println("exception=" + error.getMessage());
        }
    }
}
