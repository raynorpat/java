public class Threads implements Runnable {
    static int count = 0;
    static Object lock = new Object();
    public void run() {
        for (int i = 0; i < 10000; i++) synchronized (lock) { count++; }
        synchronized (lock) { lock.notifyAll(); }
    }
    public static void main(String[] a) throws Exception {
        Thread[] t = new Thread[8];
        for (int i = 0; i < t.length; i++) { t[i] = new Thread(new Threads()); t[i].start(); }
        for (int i = 0; i < t.length; i++) t[i].join();
        System.out.println("count=" + count + " (expect 80000)");
        try { Object o = null; o.hashCode(); } catch (NullPointerException e) { System.out.println("caught NPE"); }
        for (int i = 0; i < 2000; i++) { byte[] b = new byte[10000]; }
        System.gc();
        System.out.println("gc ok, free=" + (Runtime.getRuntime().freeMemory() > 0));
    }
}
