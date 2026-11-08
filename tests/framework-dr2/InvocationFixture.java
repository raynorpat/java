class InvocationBase {
    public int inherited() { return 41; }
    public static int inheritedStatic() { return 42; }
}

public class InvocationFixture extends InvocationBase {
    public int add(int x) { return x + 7; }
    public static int addStatic(int x) { return x + 9; }
    public static long wide(long x) { return x ^ 0x123456789abcdef0L; }
    public static float single(float x) { return x + 0.5f; }
    public static double twice(double x) { return x * 2.0; }
    public static Object identity(Object x) { return x; }
    public static void fail() { throw new IllegalArgumentException("fixture"); }
}
