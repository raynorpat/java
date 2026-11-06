public class VMReserve115 implements Runnable {
    static Object[] retained;
    public void run() {
        retained = new Object[2000];
        int state = 1;
        int index = 0;
        while (state != 3 && index < retained.length) {
            retained[index++] = new byte[8192];
            int next = sun.misc.VM.getState();
            if (next != state) {
                state = next;
                if (state == 2) System.out.println("yellow");
                if (state == 3) System.out.println("red");
            }
        }
    }
    public static void main(String[] args) throws Exception {
        System.out.println("initial=" + sun.misc.VM.getState());
        System.out.println("suspended=" + sun.misc.VMState115.suspended());
        // End the allocator's native stack lifetime before testing recovery:
        // conservative collectors can retain stale references in live frames.
        Thread worker = new Thread(new VMReserve115());
        worker.start();
        worker.join();
        retained = null;
        System.gc();
        System.out.println("recovered=" + sun.misc.VM.getState());
        System.out.println("suspended=" + sun.misc.VMState115.suspended());
        sun.misc.VM.unsuspendSomeThreads();
        System.out.println("some=" + sun.misc.VMState115.suspended());
        sun.misc.VM.unsuspendThreads();
        System.out.println("all=" + sun.misc.VMState115.suspended());
        System.exit(0);
    }
}

