import java.lang.reflect.*;

public class ReflectValues {
    public int number;
    public long wide;
    public double real;
    public String text;
    public final int fixed = 4;
    public static long shared;
    public double combine(long a, double b) { return a + b; }
    public static void main(String[] args) throws Exception {
        ReflectValues target = new ReflectValues();
        Class c = ReflectValues.class;
        Field number = c.getField("number");
        number.setByte(target, (byte)7);
        System.out.println("int=" + number.getInt(target) + " boxed=" + number.get(target));
        Field wide = c.getField("wide");
        wide.set(target, new Integer(123));
        System.out.println("long=" + wide.getLong(target) + " double=" + wide.getDouble(target));
        Field real = c.getField("real");
        real.setFloat(target, 1.5f);
        System.out.println("double=" + real.getDouble(target));
        Field text = c.getField("text"); text.set(target, "text");
        System.out.println("reference=" + text.get(target));
        Field shared = c.getField("shared"); shared.setLong(null, 9876543210L);
        System.out.println("static=" + shared.getLong(null));
        try { number.setLong(target, 1L); }
        catch (IllegalArgumentException e) { System.out.println("narrowing=" + e.getClass().getName()); }
        try { c.getField("fixed").setInt(target, 9); }
        catch (IllegalAccessException e) { System.out.println("final=" + e.getClass().getName()); }
        Object bytes = Array.newInstance(Byte.TYPE, 2);
        Array.set(bytes, 0, new Byte((byte)-3));
        System.out.println("byte=" + Array.get(bytes, 0) + " widened=" + Array.getLong(bytes, 0));
        Object longs = Array.newInstance(Long.TYPE, 1);
        Array.setInt(longs, 0, 42);
        System.out.println("arrayLong=" + Array.getLong(longs, 0));
        Object doubles = Array.newInstance(Double.TYPE, 1);
        Array.setFloat(doubles, 0, 2.5f);
        System.out.println("arrayDouble=" + Array.getDouble(doubles, 0));
        Method combine = c.getMethod("combine", new Class[] {Long.TYPE, Double.TYPE});
        System.out.println("invokeWide=" + combine.invoke(target, new Object[] {new Integer(7), new Float(0.5f)}));
        System.out.println("assignable=" + Number.class.isAssignableFrom(Integer.class)
            + " " + int.class.isAssignableFrom(long.class));
        System.out.println("instance=" + Number.class.isInstance(new Integer(3))
            + " " + int.class.isInstance(new Integer(3)));
    }
}
