// Threads + allocation + monitors + GC + finalization, with checksums to catch corruption.
public class Stress implements Runnable {
    static final int WORKERS = 6, ITEMS = 20000;
    static Object queueLock = new Object();
    static int[] queue = new int[64];
    static int head = 0, tail = 0, count = 0;
    static long consumed = 0;
    static int finalized = 0;
    static Object finLock = new Object();

    int id;
    long checksum;
    Stress(int id) { this.id = id; }

    public void run() {
        StressNode keep = null;
        for (int i = 0; i < ITEMS; i++) {
            StressNode n = new StressNode(i, null);                   // garbage
            if (i % 10 == 0) keep = new StressNode(i, keep);    // kept alive
            if (i % 500 == 0) new StressFinalizable();
            String s = "w" + id + ":" + i;                 // string churn
            if (s.length() == 0) throw new RuntimeException();
            produce(id * ITEMS + i);
        }
        for (StressNode n = keep; n != null; n = n.next) {
            if (n.payload[0] != n.value) throw new RuntimeException("corrupt node " + n.value);
            checksum += n.value;
        }
    }

    static void produce(int v) {
        synchronized (queueLock) {
            while (count == queue.length) {
                try { queueLock.wait(); } catch (InterruptedException e) { }
            }
            queue[tail] = v; tail = (tail + 1) % queue.length; count++;
            queueLock.notifyAll();
        }
    }

    static int take() {
        synchronized (queueLock) {
            while (count == 0) {
                try { queueLock.wait(); } catch (InterruptedException e) { }
            }
            int v = queue[head]; head = (head + 1) % queue.length; count--;
            queueLock.notifyAll();
            return v;
        }
    }

    public static void main(String[] args) throws Exception {
        Thread consumer = new StressConsumer();
        consumer.start();
        Stress[] w = new Stress[WORKERS];
        Thread[] t = new Thread[WORKERS];
        for (int i = 0; i < WORKERS; i++) { w[i] = new Stress(i); t[i] = new Thread(w[i]); t[i].start(); }
        for (int i = 0; i < WORKERS; i++) t[i].join();
        consumer.join();

        long expectKept = 0;
        for (int i = 0; i < ITEMS; i += 10) expectKept += i;
        long n = (long) WORKERS * ITEMS;
        boolean ok = consumed == n * (n - 1) / 2;
        for (int i = 0; i < WORKERS; i++) ok &= w[i].checksum == expectKept;

        for (int i = 0; i < 5; i++) { System.gc(); System.runFinalization(); }
        System.out.println("stress " + (ok ? "ok" : "FAILED") + " finalizers=" + (finalized > 0));
    }
}

// Java 1.0 has no inner classes, so the helpers are top-level.
class StressNode {
    int value; StressNode next; int[] payload;
    StressNode(int v, StressNode n) { value = v; next = n; payload = new int[v % 50 + 1]; payload[0] = v; }
}

class StressFinalizable {
    protected void finalize() { synchronized (Stress.finLock) { Stress.finalized++; } }
}

class StressConsumer extends Thread {
    public void run() {
        for (int i = 0; i < Stress.WORKERS * Stress.ITEMS; i++) {
            Stress.consumed += Stress.take();
            if (i % 20000 == 0) System.gc();
        }
    }
}
