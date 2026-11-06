import java.util.Date;

// java.util.Date natives: local/GMT conversion, mktime, formatting.
public class Dates {
    static int fails = 0;
    static void check(String what, boolean ok) {
        if (!ok) { fails++; System.out.println("FAIL " + what); }
    }
    public static void main(String[] args) {
        Date d = new Date(96, 0, 15, 10, 30, 45);      // Mon 15 Jan 1996 10:30:45 local
        check("fields", d.getYear() == 96 && d.getMonth() == 0 && d.getDate() == 15
              && d.getHours() == 10 && d.getMinutes() == 30 && d.getSeconds() == 45);
        check("weekday", d.getDay() == 1);
        Date e = new Date(d.getTime());
        check("roundtrip", e.getHours() == 10 && e.getDate() == 15 && e.equals(d));
        check("gmt epoch", new Date(0).toGMTString().equals("1 Jan 1970 00:00:00 GMT"));
        check("utc", Date.UTC(96, 0, 15, 10, 30, 45) == 821701845000L);
        check("offset", d.getTime() - Date.UTC(96, 0, 15, 10, 30, 45) == d.getTimezoneOffset() * 60000L);
        check("toString", d.toString().indexOf("Jan 15 10:30:45") >= 0 && d.toString().endsWith("1996"));
        check("parse", Date.parse("Sat, 12 Aug 1995 13:30:00 GMT") == 808234200000L);
        long now = System.currentTimeMillis();
        check("now", new Date().getTime() >= now && now > 820454400000L);
        System.out.println("dates " + (fails == 0 ? "ok" : "FAILED"));
    }
}
