public class InnerAndOld {
    int value = 10;
    class Inner { int value() { return InnerAndOld.this.value + 1; } }
    int local(final int value) {
        class Local { int value() { return value + InnerAndOld.this.value; } }
        return new Local().value();
    }
    public static void main(String[] args) throws Exception {
        final InnerAndOld owner = new InnerAndOld();
        System.out.println("inner=" + owner.new Inner().value());
        System.out.println("local=" + owner.local(5));
        Runnable task = new Runnable() {
            public void run() { System.out.println("anonymous=" + owner.value); }
        };
        task.run();
        if (args.length != 0) {
            Class old = Class.forName(args[0]);
            System.out.println("old=" + old.getName() + " " + old.getDeclaredMethods().length);
        }
    }
}
