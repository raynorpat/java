import java.io.*;

public class Unicode {
    static void bytes(String name, byte[] value) {
        System.out.print(name + "=" + value.length);
        for (int i = 0; i < value.length; i++) System.out.print(" " + (value[i] & 255));
        System.out.println();
    }
    public static void main(String[] args) throws Exception {
        String value = "\u00e9\u4e2d";
        System.out.println("length=" + value.length());
        System.out.println("chars=" + (int)value.charAt(0) + " " + (int)value.charAt(1));
        System.out.println("upper=" + (int)value.toUpperCase().charAt(0));
        bytes("UTF8", value.getBytes("UTF8"));
        bytes("UnicodeBig", value.getBytes("UnicodeBig"));
        System.out.println("roundtrip=" + value.equals(new String(value.getBytes("UTF8"), "UTF8")));
        ByteArrayOutputStream data = new ByteArrayOutputStream();
        PrintStream saved = System.out;
        System.setOut(new PrintStream(data));
        byte[] encoded = value.getBytes("UTF8");
        System.out.write(encoded, 0, encoded.length);
        System.out.flush();
        System.setOut(saved);
        bytes("stdout", data.toByteArray());
        File file = new File("unicode115-\u00e9.tmp");
        if (file.exists()) throw new IOException("fixture file already exists");
        try {
            FileOutputStream out = new FileOutputStream(file);
            out.write(encoded); out.close();
            FileInputStream in = new FileInputStream(file);
            ByteArrayOutputStream read = new ByteArrayOutputStream();
            int b;
            while ((b = in.read()) != -1) read.write(b);
            in.close();
            System.out.println("filename=" + file.exists() + " roundtrip="
                + value.equals(new String(read.toByteArray(), "UTF8")));
        } finally {
            if (!file.delete()) throw new IOException("fixture file cleanup failed");
        }
    }
}
