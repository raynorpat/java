import java.sql.*;
import sun.jdbc.odbc.JdbcOdbc;

public class JdbcOdbcData115 extends JdbcOdbc {
    JdbcOdbcData115() throws Exception { super(""); }
    static void check(boolean ok, String text) throws Exception {
        if (!ok) throw new Exception(text);
    }
    public static void main(String[] args) throws Exception {
        new JdbcOdbcData115().run(args[0]);
    }
    void run(String path) throws Exception {
        int env = SQLAllocEnv(), dbc = SQLAllocConnect(env);
        try {
            SQLDriverConnect(dbc, "Driver={Microsoft Access Driver (*.mdb)};DBQ=" + path);
        } catch (SQLWarning warning) {
            // Jet may report a volatile registry-key warning on modern Windows.
            // SQL_SUCCESS_WITH_INFO still leaves a usable connection.
        }
        int stmt = SQLAllocStmt(dbc);
        SQLExecDirect(stmt, "CREATE TABLE native_test (id INTEGER, txt VARCHAR(80), bin BINARY(4), d DATETIME, t DATETIME, ts DATETIME, f SINGLE, n DOUBLE, absent VARCHAR(10))");
        SQLPrepare(stmt, "INSERT INTO native_test VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
        check(SQLNumParams(stmt) == 9, "parameter count");
        byte[][] data = new byte[9][64];
        byte[] status = new byte[8], len = new byte[8];
        SQLBindInParameterInteger(stmt, 1, Types.INTEGER, 42, data[0]);
        SQLBindInParameterString(stmt, 2, Types.VARCHAR, 20, 0, "abcdefghijklmnop", data[1]);
        bindInParameterBinary(stmt, 3, Types.BINARY, new byte[] {0, 1, 127, -1}, 4, data[2], len, status);
        SQLBindInParameterDate(stmt, 4, Date.valueOf("1997-12-10"), data[3]);
        SQLBindInParameterTime(stmt, 5, Time.valueOf("11:12:13"), data[4]);
        SQLBindInParameterTimestamp(stmt, 6, Timestamp.valueOf("1997-12-10 11:12:13"), data[5]);
        SQLBindInParameterFloat(stmt, 7, Types.REAL, 0, 3.25f, data[6]);
        SQLBindInParameterDouble(stmt, 8, Types.DOUBLE, 0, 12.5, data[7]);
        SQLBindInParameterNull(stmt, 9, Types.VARCHAR, data[8]);

        SQLExecute(stmt); check(SQLRowCount(stmt) == 1, "native insert count");
        SQLFreeStmt(stmt, 3); // SQL_RESET_PARAMS
        SQLExecDirect(stmt, "SELECT * FROM native_test");
        check(SQLNumResultCols(stmt) == 9, "result count");
        check(SQLColAttributes(stmt, 1, 2) == Types.INTEGER, "column SQL type");
        check(SQLColAttributesString(stmt, 2, 1).equals("txt"), "column name");
        check(SQLFetch(stmt), "native fetch");
        check(SQLGetDataInteger(stmt, 1).intValue() == 42, "native integer");
        byte[] small = new byte[5];
        int full = getDataString(stmt, 2, small, status);
        check(full == 16 && status[0] == 1 && status[1] == 0, "truncation status/length");
        check(small[0] == 97 && small[3] == 100 && small[4] == 0, "truncation contents");
        check(SQLGetDataBinary(stmt, 3, data[2]) == 4 && data[2][3] == -1, "binary result");
        check(SQLGetDataStringDate(stmt, 4).equals("1997-12-10"), "date result");
        check(SQLGetDataStringTime(stmt, 5).equals("11:12:13"), "time result");
        check(SQLGetDataStringTimestamp(stmt, 6).equals("1997-12-10 11:12:13"), "timestamp result");
        check(SQLGetDataFloat(stmt, 7).floatValue() == 3.25f, "float result");
        check(SQLGetDataDouble(stmt, 8).doubleValue() == 12.5, "double result");
        getDataString(stmt, 9, small, status);
        check(status[0] == 0 && status[1] == 1, "native NULL indicator");
        check(!SQLFetch(stmt), "end of rows");
        SQLFreeStmt(stmt, 0); // SQL_CLOSE
        SQLExecDirect(stmt, "SELECT id FROM native_test");
        SQLBindColDefault(stmt, 1, data[0], len);
        check(SQLFetch(stmt) && bufferToInt(data[0]) == 42, "bound column");
        SQLFreeStmt(stmt, 2); // SQL_UNBIND
        SQLFreeStmt(stmt, 0);
        SQLExecDirect(stmt, "CREATE TABLE stream_test (payload LONGCHAR)");
        SQLPrepare(stmt, "INSERT INTO stream_test VALUES (?)");
        SQLBindInParameterAtExec(stmt, 1, Types.LONGVARCHAR, 6, data[0], len);
        check(SQLExecute(stmt), "data-at-exec request");
        check(SQLParamData(stmt) == 1, "data-at-exec parameter number");
        SQLPutData(stmt, new byte[] {98, 114, 105, 100, 103, 101}, 6);
        check(SQLParamData(stmt) == -1, "data-at-exec completion");
        SQLFreeStmt(stmt, 3);
        SQLExecDirect(stmt, "SELECT payload FROM stream_test");
        check(SQLFetch(stmt), "stream row");
        check(SQLGetDataString(stmt, 1, 20, false).equals("bridge"), "stream contents");
        SQLFreeStmt(stmt, 1); // SQL_DROP
        SQLDisconnect(dbc); SQLFreeConnect(dbc); SQLFreeEnv(env);
        System.out.println("native conversions, bound buffers, truncation and data-at-exec: ok");
    }
}
