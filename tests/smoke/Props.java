import java.io.File;

public class Props {
    public static void main(String[] args) {
        String home = System.getProperty("java.home");
        System.out.println("version=" + System.getProperty("java.version"));
        System.out.println("vendor=" + System.getProperty("java.vendor"));
        System.out.println("separator=" + File.separator);
        System.out.println("home is dir=" + new File(home).isDirectory());
        System.out.println("child is dir=" + new File(home, ".").isDirectory());
        System.out.println("classpath set=" + (System.getProperty("java.class.path").length() > 0));
    }
}
