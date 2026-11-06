import java.io.*;

public class SerialValues {
    public static class Base {
        static int calls;
        int base;
        public Base() { calls++; base = 81; }
    }
    public static class Values extends Base implements Serializable {
        private static final long serialVersionUID = 115L;
        static int calls;
        boolean z = true;
        byte b = -117;
        char c = '\u2345';
        short s = -23456;
        int i = -123456789;
        long j = 0x123456789abcdef0L;
        float f = -1.25f;
        double d = 3.125;
        Object empty;
        transient int skipped = 9;
        int[] ints = {3, -7, 123456};
        long[] longs = {0x123456789abcdef0L, -1L};
        double[] doubles = {-0.0, 2.5};
        String[][] strings = {{"one", null}, {"two"}};
        public Values() { calls++; }
    }
    public static void main(String[] args) throws Exception {
        Values value = new Values();
        value.base = 999;
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        ObjectOutputStream out = new ObjectOutputStream(bytes);
        out.writeObject(value); out.close();
        ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(bytes.toByteArray()));
        Values copy = (Values)in.readObject(); in.close();
        System.out.println("small=" + copy.z + " " + copy.b + " " + (int)copy.c + " " + copy.s);
        System.out.println("numbers=" + copy.i + " " + copy.j + " " + copy.f + " " + copy.d);
        System.out.println("empty=" + (copy.empty == null) + " transient=" + copy.skipped);
        System.out.println("constructors=" + Base.calls + " " + Values.calls + " base=" + copy.base);
        System.out.println("ints=" + copy.ints[0] + " " + copy.ints[1] + " " + copy.ints[2]);
        System.out.println("longs=" + copy.longs[0] + " " + copy.longs[1]);
        System.out.println("doubles=" + Double.doubleToLongBits(copy.doubles[0]) + " " + copy.doubles[1]);
        System.out.println("strings=" + copy.strings[0][0] + " " + copy.strings[0][1] + " " + copy.strings[1][0]);
    }
}
