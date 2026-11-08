public class NativeEntry115 {
    static { System.loadLibrary("nativeentry115"); }
    static native void initialize();
    static native void enter(int depth);
    static void callback(int depth) {
        System.gc();
        if (depth > 0) enter(depth - 1);
    }
    public static void main(String[] args) throws Exception {
        initialize();
        for (int thread = 0; thread < 4; thread++) {
            final Throwable[] failure = new Throwable[1];
            Thread worker = new Thread(new Runnable() {
                public void run() {
                    try {
                        for (int cycle = 0; cycle < 20; cycle++) enter(2);
                    } catch (Throwable value) { failure[0] = value; }
                }
            });
            worker.start();
            worker.join();
            if (failure[0] != null) throw new Error(failure[0].toString());
        }
        System.out.println("DR2 JNI native entry survives nested callbacks and GC");
    }
}
