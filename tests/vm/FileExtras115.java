import java.io.*;
public class FileExtras115 {
    public static void main(String[] args) throws Exception {
        File file = new File("append115.tmp");
        FileOutputStream out = new FileOutputStream(file.getPath());
        out.write(65); out.close();
        out = new FileOutputStream(file.getPath(), true);
        out.write(66); out.close();
        FileInputStream in = new FileInputStream(file);
        System.out.println("append=" + in.read() + ":" + in.read() + ":" + in.read());
        in.close();
        System.out.println("file=" + file.delete());
        File dir = new File("mkdir115.tmp");
        System.out.println("directory=" + dir.mkdir() + ":" + dir.delete() + ":" + dir.exists());
    }
}
