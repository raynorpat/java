import java.io.*;

public class Serial {
    public static void main(String[] args) throws Exception {
        SerialNode first = new SerialNode(7);
        SerialNode second = new SerialNode(11);
        first.next = second; second.next = first;
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        ObjectOutputStream output = new ObjectOutputStream(bytes);
        output.writeObject(new Object[] {first, second, first}); output.close();
        ObjectInputStream input = new ObjectInputStream(new ByteArrayInputStream(bytes.toByteArray()));
        Object[] graph = (Object[])input.readObject(); input.close();
        SerialNode a = (SerialNode)graph[0], b = (SerialNode)graph[1];
        System.out.println("values=" + a.value + " " + b.value);
        System.out.println("cycle=" + (a.next == b && b.next == a));
        System.out.println("shared=" + (graph[0] == graph[2]));
        System.out.println("hook=" + a.marker + " " + b.marker);
        Class[] types = {SerialNode.class, String.class, java.util.Vector.class};
        for (int i = 0; i < types.length; i++)
            System.out.println(types[i].getName() + "=" + ObjectStreamClass.lookup(types[i]).getSerialVersionUID());
    }
}
class SerialNode implements Serializable {
    private static final long serialVersionUID = 115L;
    int value;
    SerialNode next;
    transient int marker;
    SerialNode(int value) { this.value = value; }
    private void writeObject(ObjectOutputStream out) throws IOException {
        out.defaultWriteObject(); out.writeInt(value + 100);
    }
    private void readObject(ObjectInputStream in) throws IOException, ClassNotFoundException {
        in.defaultReadObject(); marker = in.readInt();
    }
}
