public class NumericConversions115 {
    static void d(double x) {
        System.out.println("d=" + (int)x + "," + (long)x);
    }
    static void f(float x) {
        System.out.println("f=" + (int)x + "," + (long)x);
    }
    static void divide(int x, int y, long a, long b) {
        System.out.println("div=" + (x/y) + "," + (x%y) + "," + (a/b) + "," + (a%b));
    }
    public static void main(String[] args) {
        d(Double.NaN); d(Double.POSITIVE_INFINITY); d(Double.NEGATIVE_INFINITY);
        d(2147483648.0); d(-2147483649.0);
        d(9223372036854775808.0); d(-9223372036854775808.0);
        d(9223372036854774784.0); d(-9223372036854774784.0);
        d(1234.75); d(-1234.75); d(-0.0);
        f(Float.NaN); f(Float.POSITIVE_INFINITY); f(Float.NEGATIVE_INFINITY);
        f(2147483648.0f); f(-2147483648.0f);
        f(9223372036854775808.0f); f(-9223372036854775808.0f);
        f(1234.75f); f(-1234.75f); f(-0.0f);
        divide(Integer.MIN_VALUE, -1, Long.MIN_VALUE, -1);
    }
}
