import java.io.*;

public class Canonical115 {
    public static void main(String[] args) throws Exception {
        String base = new File(".").getCanonicalPath();
        String child = new File("./missing115/../future115.txt").getCanonicalPath();
        System.out.println("dots=" + child.equals(base + File.separator + "future115.txt"));
        File file = new File("Canon115Case.tmp");
        FileOutputStream out = new FileOutputStream(file); out.close();
        try {
            System.out.println("existing=" + file.getCanonicalPath().equals(base + File.separator + "Canon115Case.tmp"));
            System.out.println("absolute=" + new File(base).getCanonicalPath().equals(base));
        } finally { file.delete(); }
    }
}
