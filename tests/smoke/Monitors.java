// Monitor and thread-control edge cases for the native thread layer.
public class Monitors {
    static int fails = 0;
    static void check(String what, boolean ok) {
        if (!ok) { fails++; System.out.println("FAIL " + what); }
    }

    public static void main(String[] args) throws Exception {
        Object lock = new Object();

        long t0 = System.currentTimeMillis();
        synchronized (lock) { lock.wait(200); }
        long waited = System.currentTimeMillis() - t0;
        check("timed wait", waited >= 150 && waited < 2000);

        t0 = System.currentTimeMillis();
        Thread.sleep(100);
        check("sleep", System.currentTimeMillis() - t0 >= 80);

        // Reentrant monitors
        synchronized (lock) { synchronized (lock) { lock.notifyAll(); } }

        try { lock.notify(); check("notify unowned", false); }
        catch (IllegalMonitorStateException e) { }

        // Thread.stop() wakes a thread blocked in wait()
        MonitorsWaiter w = new MonitorsWaiter();
        w.start();
        while (!w.waiting) Thread.sleep(10);
        Thread.sleep(50);
        w.stop();
        w.join(5000);
        check("stop waiter", !w.isAlive() && w.died);

        // Timed join on a thread that keeps running
        MonitorsSpinner s = new MonitorsSpinner();
        s.setPriority(Thread.MIN_PRIORITY);
        s.start();
        t0 = System.currentTimeMillis();
        s.join(100);
        check("timed join", s.isAlive() && System.currentTimeMillis() - t0 >= 80);
        check("priority", s.getPriority() == Thread.MIN_PRIORITY);
        s.stop();
        s.join(5000);
        check("stop spinner", !s.isAlive());

        // A daemon thread must not keep the VM alive.
        MonitorsSpinner d = new MonitorsSpinner();
        d.setDaemon(true);
        d.start();

        System.out.println("monitors " + (fails == 0 ? "ok" : "FAILED"));
    }
}

class MonitorsWaiter extends Thread {
    volatile boolean waiting, died;
    public void run() {
        Object o = new Object();
        try {
            synchronized (o) { waiting = true; o.wait(); }
        } catch (InterruptedException e) {
        } catch (ThreadDeath e) {
            died = true;
            throw e;
        }
    }
}

class MonitorsSpinner extends Thread {
    public void run() {
        while (true) { Thread.yield(); }
    }
}
