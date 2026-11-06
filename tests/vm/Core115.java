import java.io.*;

public class Core115 {
    static class Loader extends ClassLoader {
        public Class loadClass(String name, boolean resolve) throws ClassNotFoundException { return findSystemClass(name); }
        void sign(Class type, Object[] values) { setSigners(type, values); }
    }
    static class Manager extends SecurityManager {
        Class loaded() { return currentLoadedClass(); }
    }
    public static void main(String[] args) throws Exception {
        Class type = Core115.class;
        System.out.println("initial=" + type.getSigners());
        Loader loader = new Loader();
        Object[] values = {"one", "two"};
        loader.sign(type, values);
        Object[] first = type.getSigners();
        first[0] = "copy";
        System.gc();
        System.out.println("copy=" + type.getSigners()[0] + " " + (first != type.getSigners()));
        values[1] = "changed";
        System.out.println("stored=" + type.getSigners()[1]);
        loader.sign(type, null);
        System.out.println("cleared=" + type.getSigners());
        System.out.println("loaded=" + new Manager().loaded());
        File file = new File("core115-sync.tmp");
        FileOutputStream out = new FileOutputStream(file);
        try { out.write(115); out.getFD().sync(); System.out.println("sync=true"); }
        finally { out.close(); file.delete(); }
        System.out.println("compile=" + Compiler.compileClass(Core115.class));
    }
}
