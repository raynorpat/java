import java.io.*;

// File, stream, random-access and directory operations.
public class Files {
    static int fails = 0;
    static void check(String what, boolean ok) {
        if (!ok) { fails++; System.out.println("FAIL " + what); }
    }
    public static void main(String[] args) throws IOException {
        File dir = new File("files-test.tmp");
        File f = new File(dir, "data.bin");
        if (f.exists()) f.delete();

        // 1.0.2 File.delete() cannot remove directories, so the directory is reused.
        check("mkdir", dir.isDirectory() || (dir.mkdir() && dir.isDirectory()));
        FileOutputStream out = new FileOutputStream(f);
        for (int i = 0; i < 1000; i++) out.write(i & 0xff);
        out.close();
        check("length", f.length() == 1000 && f.isFile() && f.canRead() && f.canWrite());

        FileInputStream in = new FileInputStream(f);
        check("available", in.available() == 1000);
        byte[] buf = new byte[600];
        int n = in.read(buf);
        check("read", n == 600 && (buf[599] & 0xff) == (599 & 0xff));
        check("available after", in.available() == 400);
        in.close();

        RandomAccessFile raf = new RandomAccessFile(f, "rw");
        raf.seek(900);
        check("raf read", raf.read() == (900 & 0xff) && raf.getFilePointer() == 901);
        raf.seek(1000);
        raf.writeInt(0x12345678);
        raf.seek(1000);
        check("raf int", raf.readInt() == 0x12345678 && raf.length() == 1004);
        raf.close();

        String[] names = dir.list();
        check("list", names != null && names.length == 1 && names[0].equals("data.bin"));
        check("missing", !new File(dir, "nope").exists());
        File g = new File(dir, "renamed.bin");
        check("rename", f.renameTo(g) && g.exists() && !f.exists());
        check("delete", g.delete() && !g.exists());
        System.out.println("files " + (fails == 0 ? "ok" : "FAILED"));
    }
}
