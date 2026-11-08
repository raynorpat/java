public class ProxyFixture extends ProxyBase {
    private int value;
    public ProxyFixture() { value = 7; }
    public ProxyFixture(int value) { this.value = value; }
    public int add(int other) { return value + other; }
    public static int twice(int value) { return value * 2; }
    public long wide(long value) { return value ^ 0x1122334455667788L; }
    public float single(float value) { return value + 0.5f; }
    public double fraction(double value) { return value * 2.0; }
    public String greeting(String value) { return "Hello " + value; }
    public Object identity(Object value) { return value; }
    public int arrayArgument(int[] values) { return values.length; }
    public void fail() { throw new IllegalStateException("proxy failure"); }
    public int collect() {
        for (int i = 0; i < 20; i++) {
            byte[] garbage = new byte[8192];
            garbage[0] = (byte)i;
        }
        System.gc();
        return value;
    }
    public String toString() { return "proxy:" + value; }
}
class ProxyBase {
    public int inherited() { return 115; }
}
