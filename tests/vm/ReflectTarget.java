public class ReflectTarget {
    private int hidden;
    public String label;
    public ReflectTarget() { hidden = 0; }
    public ReflectTarget(int value) { hidden = value; }
    public int add(int value) { return hidden + value; }
}
