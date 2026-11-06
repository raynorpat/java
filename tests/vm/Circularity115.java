public class Circularity115 {
    static void load(String name) {
        try {
            System.out.println(name + "=" + Class.forName(name).getName());
        } catch (Throwable error) {
            System.out.println(name + "=" + error.getClass().getName());
        }
    }
    public static void main(String[] args) {
        load("Cycle115Self");
        load("Cycle115A");
        load("Cycle115B");
        load("Cycle115A");
        load("Cycle115Base");
    }
}

class Cycle115Base { }
class Cycle115A extends Cycle115Base { }
class Cycle115B extends Cycle115A { }
class Cycle115Self extends Cycle115Base { }
