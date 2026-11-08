public class ClassAbiFixture {
 public int value; private String secret; public static final long constant=5;
 public ClassAbiFixture() {} private ClassAbiFixture(int value) {this.value=value;}
 public int number() {return 17;} private String hidden(double d) {return secret;}
}
