public class Debuggee115 {
    public static int value = 7;
    public static void target(int argument) {
        int local = argument + value;
        value = local + 1;
        System.out.println("debuggee value=" + value);
    }
    public static void main(String[] args) {
        target(10);
    }
}
