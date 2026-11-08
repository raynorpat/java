import com.apple.yellow.foundation.NSRuntime;
public class BootstrapRuntime {
    private static void check(boolean result) { if (!result) throw new RuntimeException("bootstrap check"); }
    public static void main(String[] args) {
        String[] initial = NSRuntime.librarySearchPaths();
        check(initial.length > 0 && initial[0].equals("./"));
        for (int i = 0; i < initial.length; ++i) {
            check(initial[i] != null);
            check(initial[i].indexOf("/usr/lib/java") < 0);
        }
        check(initial[1].equals(args[0] + "/"));
        NSRuntime.loadLibrary("BootstrapFixture.A");
        String[] paths = {args[0] + "/"};
        NSRuntime.setLibrarySearchPaths(paths);
        NSRuntime.setLibrarySearchPaths(null);
        check(NSRuntime.librarySearchPaths() == paths);
        NSRuntime.addPathToLibrarySearchPaths("example");
        check(NSRuntime.librarySearchPaths()[0].equals("example/"));
        NSRuntime.removePathFromLibrarySearchPaths("example");
        check(NSRuntime.librarySearchPaths().length == 2);
        NSRuntime.removePathFromLibrarySearchPaths("example/");
        check(NSRuntime.librarySearchPaths().length == 1);
        NSRuntime.loadLibrary("BootstrapFixture.A");
        NSRuntime.loadLibrary("BootstrapFixture.A");
        NSRuntime.loadLibrary("BootstrapFallback.A");
        try { NSRuntime.loadLibrary("BootstrapMissing"); throw new RuntimeException("missing initializer accepted"); }
        catch (UnsatisfiedLinkError expected) { System.out.println("missing initializer rejected"); }
        System.out.println("PASS private NSRuntime bootstrap");
    }
}
