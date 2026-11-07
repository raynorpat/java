import java.io.*;
import java.util.*;

public class Resources {
    static class Loader extends ClassLoader {
        public InputStream getResourceAsStream(String name) {
            return getSystemResourceAsStream(name);
        }
        public synchronized Class loadClass(String name, boolean resolve) throws ClassNotFoundException {
            Class type = findLoadedClass(name);
            if (type == null) {
                if (name.equals("ResourcePayload")) {
                    try {
                        InputStream in = getResourceAsStream("ResourcePayload.bin");
                        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
                        int value;
                        while ((value = in.read()) != -1) bytes.write(value);
                        in.close();
                        byte[] data = bytes.toByteArray();
                        type = defineClass(name, data, 0, data.length);
                    } catch (IOException error) { throw new ClassNotFoundException(error.toString()); }
                } else type = findSystemClass(name);
            }
            if (resolve) resolveClass(type);
            return type;
        }
    }
    static void read(String label, InputStream in) throws Exception {
        if (in == null) throw new RuntimeException("missing resource: " + label);
        Properties props = new Properties(); props.load(in); in.close();
        System.out.println(label + "=" + props.getProperty("answer") + " " + (int)props.getProperty("letter").charAt(0));
    }
    public static void main(String[] args) throws Exception {
        read("system", ClassLoader.getSystemResourceAsStream("vm115.properties"));
        Loader loader = new Loader();
        read("custom", loader.getResourceAsStream("vm115.properties"));
        Class lazy = new Loader().loadClass("ResourcePayload", false);
        System.out.println("lazy=" + lazy.newInstance().toString());
        Class payload = loader.loadClass("ResourcePayload", true);
        System.out.println("defined=" + (payload.getClassLoader() == loader));
        System.out.println("payload=" + payload.newInstance().toString());
        System.out.println("missing=" + (loader.getResourceAsStream("absent-vm115.properties") == null));
    }
}
