public class Suspend115 {
    static final Object lock = new Object();
    static boolean ready, released;
    public static void main(String[] args) throws Exception {
        Thread worker = new Thread(new Runnable() {
            public void run() {
                synchronized (lock) {
                    ready = true;
                    lock.notifyAll();
                    while (!released) {
                        try { lock.wait(); } catch (InterruptedException ignored) { }
                    }
                }
            }
        });
        worker.setDaemon(true);
        worker.start();
        synchronized (lock) { while (!ready) lock.wait(); }
        worker.suspend();
        worker.suspend();
        synchronized (lock) { released = true; lock.notifyAll(); }
        worker.resume();
        worker.join(2000);
        boolean resumed = !worker.isAlive();
        if (!resumed) { worker.resume(); worker.join(2000); }
        if (!resumed) throw new RuntimeException("resume did not clear repeated suspension");
        System.out.println("repeated suspension cleared by one resume");
    }
}
