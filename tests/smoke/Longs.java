// 64-bit arithmetic edge cases (the VM's long ops are native __int64).
public class Longs {
    static int fails = 0;
    static void check(String what, boolean ok) {
        if (!ok) { fails++; System.out.println("FAIL " + what); }
    }
    public static void main(String[] args) {
        long min = Long.MIN_VALUE, max = Long.MAX_VALUE, m1 = -1;
        check(">>>", (m1 >>> 60) == 15);
        check(">>", (min >> 63) == -1);
        check("<<", (1L << 40) == 1099511627776L);
        check("min/-1", min / m1 == min);
        check("min%-1", min % m1 == 0);
        check("rem sign", (-7L % 3L) == -1);
        check("mul wrap", max * 2 == -2);
        check("l2d", (double) max == 9.223372036854775807E18);
        check("d2l", (long) 1e19 == max && (long) -1e19 == min);
        check("f2l", (long) 3.9f == 3);
        check("l2i", (int) 0x123456789L == 0x23456789);
        check("toString", Long.toString(min).equals("-9223372036854775808"));
        check("parse", Long.parseLong("-9223372036854775808") == min);
        check("cmp", min < max && max > 0 && !(min > 0));
        check("time", System.currentTimeMillis() > 820454400000L); // after 1996
        System.out.println("longs " + (fails == 0 ? "ok" : "FAILED"));
    }
}
