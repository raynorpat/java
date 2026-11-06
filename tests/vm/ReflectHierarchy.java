import java.lang.reflect.*;
import java.util.Vector;

public class ReflectHierarchy {
    public interface Root { int ROOT = 1; String speak(); }
    public interface Left extends Root { int LEFT = 2; void left(); }
    public interface Right extends Root { int RIGHT = 3; void right(); }
    public static class Parent implements Root {
        public int inherited;
        public String speak() { return "parent"; }
        public static int staticMethod() { return 5; }
    }
    public static class Leaf extends Parent implements Left, Right {
        public int own;
        public String speak() { return "leaf"; }
        public void left() { }
        public void right() { }
    }
    static void members(String label, Class type) {
        System.out.println(label + " fields");
        Reflect.sorted(type.getFields());
        Method[] methods = type.getMethods();
        Vector kept = new Vector();
        for (int i = 0; i < methods.length; i++)
            if (methods[i].getDeclaringClass() != Object.class) kept.addElement(methods[i]);
        Object[] values = new Object[kept.size()]; kept.copyInto(values);
        System.out.println(label + " methods");
        Reflect.sorted(values);
    }
    public static void main(String[] args) throws Exception {
        members("leaf", Leaf.class);
        members("left", Left.class);
        Leaf leaf = new Leaf();
        System.out.println("virtual=" + Parent.class.getMethod("speak", new Class[0]).invoke(leaf, new Object[0]));
        System.out.println("lookup=" + Leaf.class.getMethod("speak", new Class[0]).getDeclaringClass().getName());
        System.out.println("interfaceField=" + Leaf.class.getField("ROOT").getInt(null));
        System.out.println("interfaceMethod=" + Left.class.getMethod("speak", new Class[0]).getDeclaringClass().getName());
    }
}
