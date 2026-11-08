import com.apple.alpha.core.NativeException;

public class ExceptionFixture {
    public static Throwable make(int which, Object info) {
        switch (which) {
        case 0: return new IllegalArgumentException("bad argument");
        case 1: return new RuntimeException();
        case 2: return new RuntimeException("*nil*");
        case 3: return new NativeException("NativeName", "why", info);
        case 4: return new BrokenMessage();
        default: return new NativeException(null, "fallback", null);
        }
    }
    public static Object token() { return new Object(); }
    public static void fail() { throw new IllegalStateException("method failure"); }
}
class ExceptionCtor {
    public ExceptionCtor() { throw new IllegalStateException("constructor failure"); }
}
class BrokenMessage extends RuntimeException {
    public String getMessage() { throw new IllegalStateException("secondary"); }
}
