public class Hello {
    public static void main(String[] args) {
        System.out.println("Hello from the 1.0.2 VM: " + Math.rint(2.5) + " " + Math.rint(3.7) + " " + new java.util.Date(0).getTime());
        int[] a = new int[100000];
        for (int i = 0; i < a.length; i++) a[i] = i * 2;
        long s = 0; for (int i = 0; i < a.length; i++) s += a[i];
        System.out.println("sum=" + s + " java.version=" + System.getProperty("java.version"));
    }
}
