public class FinalizeExit {
    static Object keep;
    static String mode;
    static class Item {
        int id;
        Item(int id) { this.id = id; }
        protected void finalize() {
            System.out.println("final=" + id);
            System.runFinalization();
            if (id == 1) keep = new Item(2);
            if (mode.equals("reenter")) System.exit(0);
        }
    }
    public static void main(String[] args) {
        mode = args.length == 0 ? "zero" : args[0];
        keep = new Item(1);
        Runtime.runFinalizersOnExit(true);
        if (mode.equals("disabled")) Runtime.runFinalizersOnExit(false);
        System.out.println("main");
        if (!mode.equals("normal")) System.exit(mode.equals("nonzero") ? 7 : 0);
    }
}
