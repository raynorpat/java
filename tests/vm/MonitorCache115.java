public class MonitorCache115 extends Thread {
    static Object shared = new Object();
    static int completed;
    Object[] locks = new Object[256];
    MonitorCache115() {
        for (int i = 0; i < locks.length; i++) locks[i] = new Object();
    }
    public void run() {
        for (int i = 0; i < 512; i++) {
            synchronized (locks[i & 255]) {
                synchronized (shared) { completed++; }
                if ((i & 31) == 0) Thread.yield();
            }
            if ((i & 127) == 0) System.gc();
        }
    }
    public static void main(String[] args) throws Exception {
        MonitorCache115[] workers = new MonitorCache115[4];
        for (int i = 0; i < workers.length; i++) workers[i] = new MonitorCache115();
        for (int i = 0; i < workers.length; i++) workers[i].start();
        for (int i = 0; i < workers.length; i++) workers[i].join();
        System.out.println("monitor churn=" + completed);
    }
}
