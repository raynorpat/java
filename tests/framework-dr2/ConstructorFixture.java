public class ConstructorFixture {
    private int result;
    public ConstructorFixture() { result = 0; }
    public ConstructorFixture(float f, int i, double d, long l) {
        result = (int)(f * 100) + i + (int)(d * 10) + (int)l + (int)(l >>> 32);
    }
    public ConstructorFixture(int i) {
        if (i < 0) throw new IllegalArgumentException("constructor");
        result = i;
    }
    public ConstructorFixture(Object o, float[] values, float f) {
        result = (o == null ? 100 : 200) + (values == null ? 10 : 20) + (int)(f * 10);
    }
    public int value() { return result; }
}
