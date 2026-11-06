import java.util.Properties;

public class Props115 {
    public static void main(String[] args) {
        System.out.println("version=" + System.getProperty("java.version"));
        System.out.println("class.version=" + System.getProperty("java.class.version"));
        System.out.println("missing=" + System.getProperty("vm.fixture.absent", "fallback"));
        Properties saved = System.getProperties();
        Properties copy = new Properties(saved);
        copy.put("vm.fixture.value", "115");
        System.setProperties(copy);
        System.out.println("set=" + System.getProperty("vm.fixture.value"));
        System.setProperties(saved);
        System.out.println("restored=" + (System.getProperty("vm.fixture.value") == null));
    }
}
