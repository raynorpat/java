public class JniAbi {
    boolean z = true; byte b = -11; char c = '\u8123'; short s = -1234;
    int i = 1234567; long j = 0x123456789abcdef0L;
    float f = 1.25f; double d = -2.5; Object o = "field";
    static long sj = 0x1122334455667788L;
    static double sd = 8.5;
    static native String check(JniAbi value);
    static native long wide(long a, double b, float c, Object d, int e);
    static native double real(double a, float b, long c);
    static native int over(int value);
    static native int over(String value);
    static native int under_score(int value);
    static native int registered(int value);
    int number(int x) { return x + 3; }
    long wideCallback(long x, double y, float z) { return x + (long)y + (long)z; }
    double realCallback(double x, float y) { return x + y; }
    static int staticCallback(int x) { return x * 7; }
    static void throwsCallback() { throw new IllegalArgumentException("callback"); }
    public static void main(String[] args) throws Exception {
        System.load(args[0]);
        System.out.println(check(new JniAbi()));
        System.out.println("wide=" + wide(0x1122334455667788L, 5.5, 2.25f, "x", 4));
        System.out.println("real=" + real(3.25, 1.5f, 7L));
        System.out.println("over=" + over(4) + "/" + over("abc"));
        System.out.println("under=" + under_score(8));
        System.out.println("registered=" + registered(9));
    }
}
