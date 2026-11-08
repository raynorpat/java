import java.io.Serializable;
import com.apple.yellow.eocontrol.EOKeyValueUnarchiver;
public class ForwardFixture implements Serializable {
    public int count;
    public ForwardFixture() {}
    public ForwardFixture(EOKeyValueUnarchiver coder) { count = 77; }
    public int tally() { return count; }
    public int sum(int a, int b) { return a + b; }
    public static int twice(int value) { return value * 2; }
    public String word() { return "word"; }
    public String one(String value) { return "one:" + value; }
    public String two(String a, String b) { return a + ":" + b; }
    public float single(float value) { return value + 0.5f; }
    public double fraction(double value) { return value + 0.25; }
    public long wide() { return 0x0102030405060708L; }
    public void mark(int value) { count = value; }
    public Object validateValueForKey(Object value, String key) { return "validated:" + value; }
    public ForwardR0 r0() { return new ForwardR0(); }
    public ForwardR4 r4() { return new ForwardR4(); }
    public ForwardR8 r8() { return new ForwardR8(); }
    public ForwardR12 r12() { return new ForwardR12(); }
}
class ForwardR0 { public int value = 115; }
class ForwardR4 { public int value = 115; }
class ForwardR8 { public int value = 115; }
class ForwardR12 { public int value = 115; }
