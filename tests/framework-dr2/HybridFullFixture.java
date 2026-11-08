import com.apple.yellow.foundation.NSObject;
public class HybridFullFixture extends NSObject {
 public int value=31;
 public HybridFullFixture() { super(); }
 public HybridFullFixture(int n) { super(); value=n; }
 public int value() { return value; }
 public HybridFullFixture identity() { return this; }
 public static HybridFullFixture make() { return new HybridFullFixture(73); }
 public int nativeHash() { return super.hashCode(); }
 public boolean nativeSame(Object other) { return super.equals(other); }
 public String nativeText() { return super.toString(); }
 public void disposeNative() throws Throwable { super.finalize(); }
 public static Object nativeRoundTrip(Object value) { return value; }
 public static boolean isNativeBase(Object value) { return value instanceof HybridFullBase; }
 public static int objectHash(Object value) { return value.hashCode(); }
 public static void disposeBase(HybridFullBase value) throws Throwable { value.disposeNative(); }
 public int nativeKVC() { super.takeValueForKey(new Integer(49),"value"); return ((Number)super.valueForKey("value")).intValue(); }
}
