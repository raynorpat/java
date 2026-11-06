import java.lang.reflect.*;

public class Reflect {
    static void sorted(Object[] items) {
        String[] values = new String[items.length];
        for (int i = 0; i < items.length; i++) values[i] = items[i].toString();
        for (int i = 0; i < values.length; i++)
            for (int j = i + 1; j < values.length; j++)
                if (values[i].compareTo(values[j]) > 0) {
                    String t = values[i]; values[i] = values[j]; values[j] = t;
                }
        for (int i = 0; i < values.length; i++) System.out.println(values[i]);
    }
    public static void main(String[] args) throws Exception {
        Class c = ReflectTarget.class;
        sorted(c.getDeclaredFields());
        sorted(c.getDeclaredMethods());
        sorted(c.getDeclaredConstructors());
        Object target = c.getConstructor(new Class[] {Integer.TYPE}).newInstance(new Object[] {new Integer(7)});
        System.out.println("invoke=" + c.getMethod("add", new Class[] {Integer.TYPE}).invoke(target, new Object[] {new Integer(5)}));
        try {
            c.getDeclaredField("hidden").setInt(target, 9);
            throw new RuntimeException("private write allowed");
        } catch (IllegalAccessException expected) {
            System.out.println("private=" + expected.getClass().getName());
        }
        Object array = Array.newInstance(Integer.TYPE, 3);
        Array.setInt(array, 1, 42);
        System.out.println("array=" + array.getClass().getName() + " " + Array.getLength(array) + " " + Array.getInt(array, 1));
        Object multi = Array.newInstance(String.class, new int[] {2, 3});
        System.out.println("multi=" + multi.getClass().getName() + " " + Array.getLength(Array.get(multi, 0)));
        System.out.println("primitive=" + Integer.TYPE.getName() + " " + Integer.TYPE.isPrimitive());
    }
}
