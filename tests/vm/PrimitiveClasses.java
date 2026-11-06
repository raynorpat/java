public class PrimitiveClasses {
    public static void main(String[] args) throws Exception {
        Class[] types = {void.class, boolean.class, byte.class, char.class,
            short.class, int.class, long.class, float.class, double.class};
        Class[] wrappers = {Void.TYPE, Boolean.TYPE, Byte.TYPE, Character.TYPE,
            Short.TYPE, Integer.TYPE, Long.TYPE, Float.TYPE, Double.TYPE};
        for (int i = 0; i < types.length; i++) {
            Class type = types[i];
            System.out.println(type.getName() + " primitive=" + type.isPrimitive()
                + " super=" + type.getSuperclass() + " interface=" + type.isInterface()
                + " interfaces=" + type.getInterfaces().length
                + " loader=" + type.getClassLoader() + " same=" + (type == wrappers[i]));
        }
        System.out.println("object=" + Object.class.isPrimitive());
        System.out.println("array=" + int[].class.isPrimitive());
        try {
            System.out.println("forName=" + Class.forName("int").getName());
        } catch (ClassNotFoundException expected) {
            System.out.println("forName=" + expected.getClass().getName());
        }
        System.gc();
        System.out.println("afterGC=" + (int.class == Integer.TYPE) + " " + Integer.TYPE.getName());
    }
}
