import com.apple.yellow.foundation.NSObject;
public class HybridFullBase extends NSObject {
 public HybridFullBase() { super(); }
 public void disposeNative() throws Throwable { super.finalize(); }
 protected HybridFullBase(boolean allocate,int object) { super(allocate,object); }
}
