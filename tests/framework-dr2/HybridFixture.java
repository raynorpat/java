class HybridBase { }
public class HybridFixture extends HybridBase {
 public int value = 31;
 public int clones;
 public int answer() { return value; }
 public Object clone() { ++clones; return this; }
}
