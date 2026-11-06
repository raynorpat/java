public class Interrupt {
    static class Worker extends Thread {
        final Object gate = new Object();
        final int mode;
        final Thread target;
        boolean ready;
        String result;
        Worker(int mode, Thread target) { this.mode = mode; this.target = target; }
        public void run() {
            synchronized (gate) { ready = true; gate.notifyAll(); }
            try {
                if (mode == 0) Thread.sleep(30000);
                else if (mode == 1) synchronized (gate) { gate.wait(30000); }
                else target.join();
                result = "returned without interrupt";
            } catch (InterruptedException expected) {
                result = expected.getClass().getName() + " flag=" + isInterrupted()
                    + " cleared=" + Thread.interrupted();
            }
        }
    }
    public static void main(String[] args) throws Exception {
        final Object stop = new Object();
        Thread target = new Thread(new Runnable() {
            public void run() {
                synchronized (stop) {
                    try { stop.wait(30000); } catch (InterruptedException done) { }
                }
            }
        });
        target.setDaemon(true); target.start();
        String[] names = {"sleep", "wait", "join"};
        for (int i = 0; i < names.length; i++) {
            Worker worker = new Worker(i, target);
            worker.setDaemon(true);
            synchronized (worker.gate) {
                worker.start();
                while (!worker.ready) worker.gate.wait();
            }
            Thread.sleep(100);
            worker.interrupt(); worker.join(5000);
            if (worker.isAlive()) throw new RuntimeException(names[i] + " interrupt timed out");
            System.out.println(names[i] + "=" + worker.result);
        }
        target.interrupt(); target.join(5000);
        if (target.isAlive()) throw new RuntimeException("target did not stop");
    }
}
