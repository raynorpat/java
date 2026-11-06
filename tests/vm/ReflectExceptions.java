import java.io.IOException;
import java.lang.reflect.*;

public class ReflectExceptions {
    public ReflectExceptions() throws IOException { }
    public void checked() throws IOException, ClassNotFoundException { }
    public void unchecked() { throw new IllegalStateException("target"); }
    public static void main(String[] args) throws Exception {
        Method checked = ReflectExceptions.class.getMethod("checked", new Class[0]);
        Class[] types = checked.getExceptionTypes();
        for (int i = 0; i < types.length; i++) System.out.println("method=" + types[i].getName());
        Constructor ctor = ReflectExceptions.class.getConstructor(new Class[0]);
        types = ctor.getExceptionTypes();
        for (int i = 0; i < types.length; i++) System.out.println("constructor=" + types[i].getName());
        Method throwing = ReflectExceptions.class.getMethod("unchecked", new Class[0]);
        try {
            throwing.invoke(ctor.newInstance(new Object[0]), new Object[0]);
            throw new RuntimeException("target exception missing");
        } catch (InvocationTargetException expected) {
            System.out.println("target=" + expected.getTargetException().getClass().getName());
        }
    }
}
