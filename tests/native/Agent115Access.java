package sun.tools.debug;
/* Establish the real package caller for package-private metadata constructors. */
public class Agent115Access {
    public static Object[] fields(Object agent, Class clazz) {
        return ((Agent)agent).getFields(clazz);
    }
    public static Object[] methods(Object agent, Class clazz) {
        return ((Agent)agent).getMethods(clazz);
    }
    public static String trace(Object agent) {
        return ((Agent)agent).exceptionStackTrace(new RuntimeException("agent probe"));
    }
}
