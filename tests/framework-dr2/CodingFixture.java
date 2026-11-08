import java.io.Serializable;
public class CodingFixture implements Serializable {
    int number = 115;
    String text = "bridge coding";
    Object self = this;
    public static Object make() { return new CodingFixture(); }
    public static Object invalid() { return new Object(); }
    public static boolean check(Object value) {
        CodingFixture f = (CodingFixture)value;
        return f.number == 115 && f.text.equals("bridge coding") && f.self == f;
    }
}
