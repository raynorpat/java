public class KVCFixture extends KVCBase {
 public int i = 12; public byte b = -5; public short s = -300;
 public char c = '\ufffe'; public boolean z = true;
 public float f = 1.25f; public double d = -2.5; public long j = 4294967298L;
 public String text = "initial"; public Object object; public Number number;
 public Integer integer; public Double decimal; public int[] array = new int[1];
 public KVCFixture peer; public java.io.Serializable serial;
 public int precedence = 3;
 public int precedence() { return 77; }
 public int rawPrecedence() { return precedence; }
 public void setPrecedence(int value) { precedence = value + 1; }
 public int method() { return i; } public void setMethod(int value) { i = value; }
 public byte byteMethod() { return b; } public short shortMethod() { return s; }
 public char charMethod() { return c; } public boolean boolMethod() { return z; }
 public float floatMethod() { return f; } public void setFloatMethod(float value) { f = value; }
 public double doubleMethod() { return d; } public void setDoubleMethod(double value) { d = value; }
 public long longMethod() { return j; } public void setLongMethod(long value) { j = value; }
 public String stringMethod() { return text; } public void setStringMethod(String value) { text = value; }
 public String numbers() { return number.getClass().getName()+":"+number+":"+integer+":"+decimal; }
 public int fail() { throw new IllegalArgumentException("kvc getter"); }
 public void setFail(int value) { throw new IllegalArgumentException("kvc setter"); }
 public int arg(int x) { return x; }
}
class KVCBase { public int inherited = 19; private int hidden = 23; }
