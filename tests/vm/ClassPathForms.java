public class ClassPathForms {
    public static void main(String[] args) throws Exception {
        System.out.println("self=" + Class.forName("ClassPathForms").getName());
        System.out.println("library=" + Class.forName("java.util.Vector").getName());
        try {
            Class.forName("AbsentClasspathFixture115");
            throw new RuntimeException("missing class loaded");
        } catch (ClassNotFoundException expected) {
            System.out.println("missing=" + expected.getClass().getName());
        }
    }
}
