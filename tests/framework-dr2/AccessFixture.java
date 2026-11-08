public class AccessFixture {
 public static native int constructorAccess();
 public static int call() { return constructorAccess(); }
 private AccessFixture() { }
 private AccessFixture(float value) { if (value != 1.25f) throw new IllegalArgumentException("float promotion"); }
}
class AccessPackage { AccessPackage() { } }
class AccessProtected { protected AccessProtected() { } }
class AccessPublic { public AccessPublic() { } }
class AccessThrow { private AccessThrow() { throw new IllegalArgumentException("access ctor"); } }
abstract class AccessAbstract { private AccessAbstract() { } }
class AccessMissing { private AccessMissing(int value) { } }

class AccessCaller { public static native int constructorAccess();
 public static int call() { return constructorAccess(); } }
