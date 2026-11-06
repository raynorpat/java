// java.lang.Math natives and floating-point conversions.
public class Maths {
    static int fails = 0;
    static void check(String what, boolean ok) {
        if (!ok) { fails++; System.out.println("FAIL " + what); }
    }
    public static void main(String[] args) {
        check("IEEEremainder", Math.IEEEremainder(5.0, 3.0) == -1.0 && Math.IEEEremainder(7.0, 2.0) == -1.0
              && Math.IEEEremainder(5.5, 2.0) == -0.5);
        check("IEEEremainder nan", Double.isNaN(Math.IEEEremainder(1.0, 0.0)));
        check("rint", Math.rint(2.5) == 2.0 && Math.rint(3.5) == 4.0 && Math.rint(-2.5) == -2.0);
        check("floor/ceil", Math.floor(-1.5) == -2.0 && Math.ceil(-1.5) == -1.0);
        check("sqrt/pow", Math.sqrt(2.0) * Math.sqrt(2.0) - 2.0 < 1e-15 && Math.pow(2.0, 10.0) == 1024.0);
        check("trig", Math.abs(Math.sin(Math.PI / 2) - 1.0) < 1e-15 && Math.atan2(1.0, 1.0) == Math.PI / 4);
        check("exp/log", Math.abs(Math.log(Math.exp(3.0)) - 3.0) < 1e-15);
        check("drem op", 5.5 % 2.0 == 1.5 && -5.5 % 2.0 == -1.5);
        check("d2i", (int) 1e10 == Integer.MAX_VALUE && (int) Double.NaN == 0);
        check("toString", Double.toString(0.1).equals("0.1") && Float.toString(1.5f).equals("1.5"));
        check("valueOf", Double.valueOf("1.25").doubleValue() == 1.25);
        check("bits", Double.doubleToLongBits(1.0) == 0x3ff0000000000000L
              && Float.floatToIntBits(1.0f) == 0x3f800000);
        System.out.println("maths " + (fails == 0 ? "ok" : "FAILED"));
    }
}
