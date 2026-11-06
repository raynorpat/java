public class FinalizeQueue115 {
    static Object keep;
    static class Item {
        int id;
        byte[] payload = new byte[1024];
        Item(int value) { id = value; payload[1023] = (byte)value; }
        protected void finalize() {
            if (id == 1) keep = null;
            System.gc();
            System.runFinalization();
            System.out.println("final=" + id + ":" + payload[1023]);
            if (id == 1) keep = new Item(4);
        }
    }
    public static void main(String[] args) {
        keep = new Item[] { new Item(1), new Item(2), new Item(3) };
        Runtime.runFinalizersOnExit(true);
        System.out.println("main");
    }
}
