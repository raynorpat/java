import com.apple.alpha.core.Selector;
import com.apple.alpha.core.ObjectiveCSelector;
public class ValuesFixture {
 public static String describe(Object value) { return value == null ? "nil" : value.getClass().getName()+":"+value.toString(); }
 public static String className(Object value) { return value == null ? "nil" : value.getClass().getName(); }
 public static Number number(int index) {
  switch(index) {
  case 0: return new Integer(-42);
  case 1: return new Float(1.25);
  case 2: return new Double(-3.5);
  case 3: return new Long("8589934595");
  case 4: return new Byte((byte)-2);
  default: return new Short((short)13);
  }
 }
 public static Selector selector(int index) {
  switch(index) {
  case 0: return new Selector("plain", new Class[0]);
  case 1: return new Selector("pair", new Class[] {String.class, Object.class});
  case 2: return new Selector("mapped", new Class[] {String.class});
  case 3: return new ObjectiveCSelector("objc:label:");
  case 4: return new ObjectiveCSelector("");
  default: return new Selector("", new Class[0]);
  }
 }
}
