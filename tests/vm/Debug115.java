public class Debug115 {
    static int target() { return 115; }
    static native int exercise(boolean sunLayout);
    public static void main(String[] args) {
        System.load(args[0]);
        int result = exercise(args.length > 1);
        System.out.println("breakpoint=" + ((result & 1) != 0));
        System.out.println("dispatch=" + ((result & 2) != 0));
        System.out.println("restored=" + ((result & 4) != 0));
        System.out.println("duplicate=" + ((result & 8) != 0));
        System.out.println("missing=" + ((result & 16) != 0));
        System.out.println("locks=" + ((result & 32) != 0));
    }
}
