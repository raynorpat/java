public class EventCover115 {
    static { System.loadLibrary("eventcover115"); }
    static native int cover(Object event);
    static native Object unwrap(int handle);
    static native boolean release(int handle);
    public static void main(String[] args) {
        for (int cycle = 0; cycle < 40; cycle++) {
            Object value = new String("event " + cycle);
            int handle = cover(value);
            System.gc();
            if (unwrap(handle) != value) throw new Error("lost event identity");
            if (!release(handle)) throw new Error("event reference not cleared");
            handle = cover(new String("native root " + cycle));
            System.gc();
            if (!("native root " + cycle).equals(unwrap(handle)))
                throw new Error("native root did not survive GC");
            if (!release(handle)) throw new Error("native root not cleared");
        }
        int handle = cover(null);
        if (unwrap(handle) != null || !release(handle)) throw new Error("null event");
        System.out.println("DR2 event covers preserve Java identity across GC and clear released references");
    }
}
