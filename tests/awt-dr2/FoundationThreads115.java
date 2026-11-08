public class FoundationThreads115 {
    static { System.loadLibrary("exceptionbridge115"); }
    static native void mark();
    static native int released();
    public static void main(String[] args) throws Exception {
        for (int cycle = 0; cycle < 20; cycle++) {
            int before = released();
            Thread worker = new Thread(new Runnable() {
                public void run() { mark(); }
            });
            worker.start();
            worker.join();
            for (int tries = 0; tries < 100 && released() == before; tries++)
                Thread.sleep(10);
            if (released() != before + 1)
                throw new Error("Foundation thread dictionary not released: " + cycle);
        }
        System.out.println("DR2 Java thread exit releases Foundation thread dictionaries");
    }
}
