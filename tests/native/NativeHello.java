// A user native library, loaded with System.loadLibrary from <java.home>\bin.
public class NativeHello {
    static { System.loadLibrary("nativehello"); }

    native int add(int a, int b);
    static native String greet(String who);

    public static void main(String[] args) {
        int sum = new NativeHello().add(40, 2);
        System.out.println("native add=" + sum + " greet=" + greet("java"));
    }
}
