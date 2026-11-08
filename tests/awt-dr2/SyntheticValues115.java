public class SyntheticValues115 {
    static { System.loadLibrary("syntheticvalues115"); }
    static native int[] convert(boolean key, int id, int code, char character, int modifiers);
    public static void main(String[] args) {
        int count = 0;
        for (int id = 400; id <= 402; id++) {
            int[] event = convert(true, id, 65, 'a', 15);
            if (id == 400) { if (event != null) throw new Error("typed event was not omitted"); }
            else check(event, id - 391, 0x1e0000, 0, 0, 'a', 0);
            count++;
        }
        int[] codes = {16, 17, 18, 20, 156, 157};
        for (int i = 0; i < codes.length; i++) {
            check(convert(true, 401, codes[i], (char)0, 1), 12, 0x20000, 0, 0, 0, 0);
            count++;
        }
        check(convert(true, 401, 38, (char)0, 0), 10, 0, 0, 0, 63232, 0); count++;
        check(convert(true, 401, 65, (char)0, 0), 10, 0, 0, 0, 0, 0); count++;
        check(convert(true, 401, 10, '\n', 0), 10, 0, 0, 0, 10, 0); count++;
        for (int id = 500; id <= 506; id++) {
            int[] event = convert(false, id, 0, (char)0, 4);
            if (id == 501 || id == 502 || id == 506)
                check(event, id == 501 ? 1 : id == 502 ? 2 : 6, 0x100000, 3, 29, 0, 3);
            else if (event != null) throw new Error("mouse event was not omitted: " + id);
            count++;
        }
        // Native exceptions are checked by the fixture, not allowed to escape the VM.
        if (convert(true, 999, 65, 'a', 0)[0] != -1 || convert(false, 999, 0, (char)0, 0)[0] != -1)
            throw new Error("invalid event type accepted");
        count += 2;
        System.out.println("DR2 synthetic factories match all " + count + " event cases");
    }
    static void check(int[] event, int type, int flags, int x, int y, int character, int clicks) {
        int[] expected = {type, flags, x, y, character, 37, clicks, 1, 5};
        if (event == null || event.length != expected.length) throw new Error("missing synthetic event");
        for (int i = 0; i < expected.length; i++)
            if (event[i] != expected[i]) throw new Error("synthetic field " + i + ": " + event[i] + " != " + expected[i]);
    }
}
