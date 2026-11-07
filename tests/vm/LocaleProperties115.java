import java.util.*;

public class LocaleProperties115 {
    static { System.loadLibrary("localeproperties115probe"); }
    static native Properties defaults(String locale, String zone, int status);
    static void check(Properties p, String language, String region, String encoding, String zone) {
        String[] keys={"user.language","user.region","file.encoding","user.timezone"};
        String[] values={language,region,encoding,zone};
        for(int i=0;i<keys.length;i++) {
            String actual=p.getProperty(keys[i]);
            if(values[i]==null ? actual!=null : !values[i].equals(actual))
                throw new RuntimeException(keys[i]+" expected "+values[i]+" got "+actual);
        }
    }
    public static void main(String[] args) {
        try {
            if(args.length!=0) {
                check(System.getProperties(),"ja","JP","SJIS","JST");
                System.out.println("locale property overrides passed");
            } else {
                String[] keys={"user.language","user.region","file.encoding","user.timezone"};
                for(int i=0;i<keys.length;i++) {
                    String value=System.getProperty(keys[i]);
                    if(value==null && i!=1) throw new RuntimeException("missing "+keys[i]);
                    System.out.println(keys[i]+"="+value);
                }
                System.out.println("locale="+Locale.getDefault()+" zone="+TimeZone.getDefault().getID()+" offset="+TimeZone.getDefault().getRawOffset());
                check(defaults("English_United Kingdom.1252","Eastern Standard Time",1),"en","GB","8859_1","EST");
                check(defaults("eNgLiSh_uNiTeD sTaTeS.1252","Pacific Standard Time",2),"en","US","8859_1","PST");
                check(defaults("Chinese_Taiwan.950","Tokyo Standard Time",1),"zh","TW","Big5","JST");
                check(defaults("Chinese_China.936","China Standard Time",1),"zh","CN","GBK","CTT");
                check(defaults("Japanese_Japan.932","India Standard Time",1),"ja","JP","SJIS","IST");
                check(defaults("Korean_Korea.949","Romance Standard Time",1),"ko","KO","KSC5601","ECT");
                check(defaults("Russian_Russia.1251","Unknown Standard Time",1),"ru","RU","Cp1251","GMT");
                check(defaults("English_Atlantis.1252","Eastern Standard Time",0),"en","Atlantis","8859_1","GMT");
                check(defaults("English","Eastern Standard Time",1),"en",null,"8859_1","EST");
                check(defaults("Unknown_United Kingdom.1252","Unknown Standard Time",1),"en",null,"8859_1","GMT");
                check(defaults(null,"Unknown Standard Time",-1),"en",null,"8859_1","GMT");
                System.out.println("Windows locale and timezone defaults passed");
            }
            System.exit(0);
        } catch(Throwable failure) {failure.printStackTrace();System.exit(1);}
    }
}
