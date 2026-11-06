class JNI_Base {
    public static final int INHERITED = 19;
}
public class JNI_Test extends JNI_Base {
    public static final boolean FLAG = true;
    public static final byte BYTE = -7;
    public static final char CHAR = '\u1234';
    public static final short SHORT = -123;
    public static final int INT = -2147483647;
    public static final long LONG = -1234567890123L;
    public static final float FLOAT = 1.25f;
    public static final double DOUBLE = 2.5;
    public static final String TEXT = "ignored";
    public native int call(int value);
    public native String call(String[] value);
    public native void arrays(boolean[] z, byte[] b, char[] c, short[] s,
                              int[] i, long[] j, float[] f, double[] d,
                              Object[] objects, int[][] nested);
    public static native Class static_call(String value, Class type, Throwable error);
    public native double primitive(boolean z, byte b, char c, short s,
                                    int i, long j, float f, double d);
    public native void m_\u00e9();
    public native void mixed(int value);
    public void mixed(String value) { }
    public native void error(java.io.IOException error);
}
