import java.io.*;
import java.net.*;
import java.util.Properties;

public class ResourceLibrary115 {
    public static void main(String[] args) throws Exception {
        URL u = ClassLoader.getSystemResource("vm115.properties");
        System.out.println("protocol=" + u.getProtocol());
        for (int i=0; i<2; i++) {
            InputStream in = u.openStream();
            Properties p = new Properties(); p.load(in); in.close();
            System.out.println("resource=" + p.getProperty("answer"));
        }
        System.out.println("missing=" + (ClassLoader.getSystemResource("absent-115-resource") == null));
        try { new URL("systemresource:/ZIP999999/+/absent").openStream(); }
        catch (IOException e) { System.out.println("invalid=" + e.getClass().getName()); }
        URL c = ClassLoader.getSystemResource("java/lang/Object.class");
        try { c.openStream(); }
        catch (SecurityException e) { System.out.println("class=" + e.getMessage()); }
    }
}
