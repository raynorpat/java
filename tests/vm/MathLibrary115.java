import java.math.BigInteger;
import java.util.Random;

public class MathLibrary115 {
    static BigInteger n(String text) { return new BigInteger(text); }
    public static void main(String[] args) {
        BigInteger a = n("12345678901234567890123456789012345678901234567890");
        BigInteger b = n("98765432109876543210987654321");
        BigInteger m = n("170141183460469231731687303715884105727");
        System.out.println("add=" + a.add(b));
        System.out.println("subtract=" + b.subtract(a));
        System.out.println("equal=" + a.subtract(a));
        System.out.println("multiply=" + a.multiply(b));
        System.out.println("square=" + b.pow(2));
        for (int i=0; i<4; i++) {
            BigInteger x = (i&1)==0 ? a : a.negate();
            BigInteger y = (i&2)==0 ? b : b.negate();
            BigInteger[] qr = x.divideAndRemainder(y);
            System.out.println("divide=" + x.divide(y) + "," + x.remainder(y));
            System.out.println("pair=" + qr[0] + "," + qr[1]);
        }
        System.out.println("gcd=" + a.gcd(b));
        System.out.println("shift=" + a.negate().shiftRight(65).shiftLeft(17));
        System.out.println("modpow=" + b.modPow(n("65537"), m));
        System.out.println("evenmod=" + n("12345").modPow(n("123"), n("100000000000000000000")));
        System.out.println("inverse=" + b.modInverse(m));
        System.out.println("negativepower=" + b.modPow(n("-7"), m));
        System.out.println("zero=" + n("0").multiply(a) + "," + a.modPow(n("0"), m));
        try { a.divide(n("0")); } catch (ArithmeticException e) { System.out.println("dividezero=" + e.getMessage()); }
        try { n("6").modInverse(n("9")); } catch (ArithmeticException e) { System.out.println("noinverse=" + e.getMessage()); }
        try { a.modPow(b, n("0")); } catch (ArithmeticException e) { System.out.println("badmod=" + e.getMessage()); }
        BigInteger prime = new BigInteger(128, 40, new Random(115));
        System.out.println("prime=" + prime + "," + prime.isProbablePrime(40));
        System.out.println("composite=" + prime.multiply(prime).isProbablePrime(40));
        Random random = new Random(115);
        int[] widths = {15,16,17,31,32,33,63,64,65,127,128,129,511,512,1024,4096};
        long checksum = 0;
        for (int i=0; i<widths.length; i++) {
            BigInteger boundary = n("1").shiftLeft(widths[i]);
            BigInteger x = new BigInteger(widths[i], random).add(boundary);
            BigInteger y = new BigInteger(widths[i]/2, random).add(n("1"));
            BigInteger[] qr = x.divideAndRemainder(y);
            if (!qr[0].multiply(y).add(qr[1]).equals(x)) throw new RuntimeException("division identity");
            if (!boundary.subtract(n("1")).add(n("1")).equals(boundary)) throw new RuntimeException("carry");
            checksum = checksum*31+x.add(y).hashCode();
            checksum = checksum*31+x.subtract(y).hashCode();
            checksum = checksum*31+x.multiply(y).hashCode();
            checksum = checksum*31+qr[0].hashCode();
            checksum = checksum*31+qr[1].hashCode();
            checksum = checksum*31+x.gcd(y).hashCode();
        }
        System.out.println("limbs=" + checksum);
        System.out.println("smallquotient=" + b.divide(a) + "," + b.remainder(a));
        System.out.println("zerogcd=" + a.gcd(n("0")) + "," + n("0").gcd(n("0")));
    }
}
