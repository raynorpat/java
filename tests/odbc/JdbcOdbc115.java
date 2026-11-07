import java.sql.*;
import sun.jdbc.odbc.JdbcOdbc;

public class JdbcOdbc115 {
    static void check(boolean value, String message) throws Exception {
        if (!value) throw new Exception(message);
    }
    public static void main(String[] args) throws Exception {
        JdbcOdbc o = new JdbcOdbc("");
        for (int i = 0; i < 10; i++) {
            int env = o.SQLAllocEnv();
            int dbc = o.SQLAllocConnect(env);
            try {
                o.SQLDriverConnect(dbc, "DSN=Jdk115MissingDriverFixture;");
                throw new Exception("invalid DSN accepted");
            } catch (SQLException e) {
                check("IM002".equals(e.getSQLState()), "diagnostic " + e.getSQLState());
            }
            o.SQLFreeConnect(dbc);
            o.SQLFreeEnv(env);
        }
        System.out.println("lifecycle and IM002: ok");
        if (args.length == 0) return;
        Connection c = new FixtureDriver().connect("jdbc:odbc:Driver={Microsoft Access Driver (*.mdb)};DBQ=" + args[0], new java.util.Properties());
        Statement s = c.createStatement();
        s.executeUpdate("CREATE TABLE bridge_test (id INTEGER, txt VARCHAR(80), n DOUBLE, absent VARCHAR(20), stamp DATETIME)");
        c.setAutoCommit(false);
        PreparedStatement p = c.prepareStatement("INSERT INTO bridge_test VALUES (?, ?, ?, ?, ?)");
        p.setInt(1, 7); p.setString(2, "native bridge"); p.setDouble(3, 12.5);
        p.setNull(4, Types.VARCHAR); p.setTimestamp(5, Timestamp.valueOf("1997-12-10 11:12:13"));
        check(p.executeUpdate() == 1, "insert count"); c.commit();
        s.executeUpdate("INSERT INTO bridge_test (id) VALUES (99)"); c.rollback();
        ResultSet r = s.executeQuery("SELECT * FROM bridge_test ORDER BY id");
        check(r.getMetaData().getColumnCount() == 5, "metadata columns");
        check(r.next(), "missing row"); check(r.getInt(1) == 7, "integer");
        check(r.getString(2).equals("native bridge"), "string");
        check(r.getDouble(3) == 12.5, "double");
        check(r.getString(4) == null && r.wasNull(), "SQL NULL");
        check(r.getTimestamp(5).toString().startsWith("1997-12-10 11:12:13"), "timestamp");
        check(!r.next(), "rollback"); r.close();
        DatabaseMetaData m = c.getMetaData();
        r = m.getTables(null, null, "bridge_test", null);
        check(r.next(), "tables metadata"); r.close();
        r = m.getColumns(null, null, "bridge_test", "%");
        int count = 0; while (r.next()) count++;
        check(count == 5, "columns metadata"); r.close();
        p.close(); s.close(); c.rollback(); c.close();
        System.out.println("prepared SQL, conversions, metadata, NULL, commit and rollback: ok");
    }
}

/* The 1.1.5 Java connection prepends DSN= unconditionally. This test adapter
 * supplies a DSN-less connection without changing the installed DSN registry.
 * Everything after the connection string passes through the original classes.
 */
class FixtureBridge extends JdbcOdbc {
    FixtureBridge() throws Exception { super(""); }
    public void SQLDriverConnect(int dbc, String text) throws SQLException {
        super.SQLDriverConnect(dbc, text.substring(4));
    }
}
class FixtureDriver extends sun.jdbc.odbc.JdbcOdbcDriver {
    FixtureDriver() throws Exception { OdbcApi = new FixtureBridge(); }
}
