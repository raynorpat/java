import java.io.*;
import java.util.*;
import java.util.zip.*;

/* Define every peer; resolve component/button metadata without starting MToolkit. */
public class PeerLink115 extends ClassLoader {
    ZipFile archive;
    Hashtable loaded = new Hashtable();
    PeerLink115(String path) throws IOException { archive = new ZipFile(path); }
    protected synchronized Class loadClass(String name, boolean resolve) throws ClassNotFoundException {
        Class type = (Class)loaded.get(name);
        if (type == null) {
            ZipEntry entry = archive.getEntry(name.replace('.', '/') + ".class");
            if (entry == null) return findSystemClass(name);
            try {
                byte[] bytes = new byte[(int)entry.getSize()];
                DataInputStream input = new DataInputStream(archive.getInputStream(entry));
                input.readFully(bytes); input.close();
                type = defineClass(name, bytes, 0, bytes.length);
                loaded.put(name, type);
            } catch (IOException error) { throw new ClassNotFoundException(name + ": " + error); }
        }
        if (resolve) resolveClass(type);
        return type;
    }
    public static void main(String[] args) throws Exception {
        PeerLink115 loader = new PeerLink115(args[0]);
        int count = 0;
        for (Enumeration entries = loader.archive.entries(); entries.hasMoreElements();) {
            String name = ((ZipEntry)entries.nextElement()).getName();
            if (!name.endsWith(".class")) continue;
            if (name.startsWith("java/")) throw new RuntimeException("baseline replacement: " + name);
            loader.loadClass(name.substring(0, name.length()-6).replace('/', '.'), false);
            count++;
        }
        Class button = loader.loadClass("sun.awt.motif.MButtonPeer", true);
        Class component = loader.loadClass("sun.awt.motif.MComponentPeer", true);
        if (!component.getDeclaredField("target").getType().getName().equals("java.awt.Component"))
            throw new RuntimeException("peer target descriptor");
        if (!java.awt.peer.ButtonPeer.class.isAssignableFrom(button))
            throw new RuntimeException("button interface");
        if (button.getMethod("setLabel", new Class[] {String.class}).getReturnType() != Void.TYPE)
            throw new RuntimeException("button label signature");
        loader.archive.close();
        if (count != 76) throw new RuntimeException("peer count " + count);
        System.out.println("defined peers=76 resolved button target and interface verified");
        System.exit(0);
    }
}
