public class ManagedLoader115 extends ClassLoader {
    public ManagedLoader115() { super(); }
    public Class loadClass(String name, boolean resolve) throws ClassNotFoundException {
        return findSystemClass(name);
    }
}
