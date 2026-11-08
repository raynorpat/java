public class ExceptionBridge115 {
    static { System.loadLibrary("exceptionbridge115"); }
    static final RuntimeException original = new RuntimeException("original callback");
    static native void test(int mode, Throwable value);
    static void callback() { throw original; }
    public static void main(String[] args) {
        for (int cycle = 0; cycle < 20; cycle++) {
            for (int mode = 0; mode < 6; mode++) {
                if (args.length != 0) System.out.println("cycle=" + cycle + " mode=" + mode);
                try {
                    test(mode, original);
                    throw new Error("native exception lost: " + mode);
                } catch (Throwable actual) {
                    if ((mode == 0 || mode == 3) && actual != original)
                        throw new Error("throwable identity lost: " + mode);
                    if (mode == 1 && (!(actual instanceof RuntimeException) || !actual.getMessage().startsWith("Non-Java exception raised, not handled! (Original problem: ") ||
                                      actual.getMessage().indexOf("native failure") < 0))
                        throw new Error("native exception translation: " + actual);
                    if (mode == 2 && (!(actual instanceof IllegalArgumentException) ||
                                      !actual.getMessage().equals("new Java failure")))
                        throw new Error("new Java exception translation: " + actual);
                    if (mode == 4 && (!(actual instanceof java.awt.AWTError) ||
                                      !actual.getMessage().equals("Unknown throwable class: no/such/Throwable.80")))
                        throw new Error("unknown exception class: " + actual);
                    if (mode == 5 && (!(actual instanceof java.awt.AWTError) ||
                                      !actual.getMessage().equals("Missing Java exception class name while trying to throw a new Java exception")))
                        throw new Error("missing exception class: " + actual);
                }
            }
        }
        System.out.println("DR2 Java/Objective-C exceptions preserve identity across callbacks and GC");
    }
}
