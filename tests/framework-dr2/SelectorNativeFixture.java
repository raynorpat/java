import com.apple.alpha.core.ObjectiveCSelector;
public class SelectorNativeFixture {
    public static String probe(Object target, String name, int mode) {
        try {
            Object argument = null;
            switch (mode) {
            case 1: argument = new Integer(17); break;
            case 2: argument = new Long(115); break;
            case 3: argument = new Float(1.5f); break;
            case 4: argument = new Double(1.25); break;
            case 5: argument = "hello"; break;
            case 6: argument = new ObjectiveCSelector("description"); break;
            }
            Object result = new ObjectiveCSelector(name).invoke(target,
                mode == 0 ? null : new Object[] { argument });
            return result == null ? "null" : result.toString();
        } catch (Throwable error) {
            return error.getClass().getName() + ":" + error.getMessage();
        }
    }
    public static boolean implemented() {
        return new ObjectiveCSelector("description").implementedByClass(Object.class);
    }
}
