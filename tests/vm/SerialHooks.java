import java.io.*;

public class SerialHooks {
    public static class External implements Externalizable {
        int value;
        public External() { value = -1; }
        public void writeExternal(ObjectOutput out) throws IOException { out.writeInt(value + 4); }
        public void readExternal(ObjectInput in) throws IOException { value = in.readInt() - 4; }
    }
    public static class WriteFailure implements Serializable {
        private void writeObject(ObjectOutputStream out) throws IOException { throw new IOException("write-hook"); }
    }
    public static class ReadFailure implements Serializable {
        private void readObject(ObjectInputStream in) throws IOException { throw new IOException("read-hook"); }
    }
    static byte[] encode(Object value) throws IOException {
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        ObjectOutputStream out = new ObjectOutputStream(bytes);
        out.writeObject(value); out.close(); return bytes.toByteArray();
    }
    static Object decode(byte[] bytes) throws Exception {
        ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(bytes));
        Object value = in.readObject(); in.close(); return value;
    }
    public static void main(String[] args) throws Exception {
        External value = new External(); value.value = 115;
        System.out.println("external=" + ((External)decode(encode(value))).value);
        try { encode(new WriteFailure()); System.out.println("write=missing"); }
        catch (IOException e) { System.out.println("write=" + e.getClass().getName() + " " + e.getMessage()); }
        try { decode(encode(new ReadFailure())); System.out.println("read=missing"); }
        catch (IOException e) { System.out.println("read=" + e.getClass().getName() + " " + e.getMessage()); }
        try { encode(new Object()); System.out.println("object=missing"); }
        catch (NotSerializableException e) { System.out.println("object=" + e.getClass().getName()); }
    }
}
