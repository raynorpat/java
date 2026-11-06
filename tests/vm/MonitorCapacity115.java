public class MonitorCapacity115 {
    static native int capacity();
    public static void main(String[] args) {
        System.load(args[0]);
        System.out.println("capacity=" + capacity());
    }
}
