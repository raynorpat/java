import com.apple.yellow.foundation.NSArray;
import com.apple.yellow.foundation.NSData;
public class WrapperDependencyFixture {
 public static String probe(int which) {
  try { return String.valueOf(which == 0 ? new NSArray().count() : new NSData().length()); }
  catch (Throwable failure) { return failure.getClass().getName()+":"+failure.getMessage(); }
 }
}
