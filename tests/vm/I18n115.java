import java.text.*;
import java.util.*;

public class I18n115 {
    static void print(String text) {
        StringBuffer ascii = new StringBuffer();
        for (int i = 0; i < text.length(); i++) {
            char c = text.charAt(i);
            if (c < 128) ascii.append(c);
            else ascii.append("[" + (int)c + "]");
        }
        System.out.println(ascii.toString());
    }
    public static void main(String[] args) throws Exception {
        Locale[] locales = { Locale.US, Locale.FRANCE, Locale.GERMANY };
        for (int i = 0; i < locales.length; i++) {
            NumberFormat numbers = NumberFormat.getNumberInstance(locales[i]);
            print(locales[i] + " number=" + numbers.format(12345.75));
            System.out.println("parse=" + numbers.parse(numbers.format(12345.75)));
            DateFormat dates = DateFormat.getDateInstance(DateFormat.LONG, locales[i]);
            dates.setTimeZone(TimeZone.getTimeZone("GMT"));
            System.out.println("date=" + dates.format(new Date(0)));
        }
        DecimalFormat decimal = new DecimalFormat("000.00");
        System.out.println("decimal=" + decimal.format(12.5));
        MessageFormat message = new MessageFormat("{0}: {1,number,integer} files");
        System.out.println(message.format(new Object[] { "Total", new Integer(7) }));
        Collator collator = Collator.getInstance(Locale.US);
        collator.setStrength(Collator.PRIMARY);
        System.out.println("collation=" + collator.compare("abc", "ABC") + "," +
            collator.compare("abc", "abd"));
        BreakIterator words = BreakIterator.getWordInstance(Locale.US);
        words.setText("One two.");
        for (int pos = words.first(); pos != BreakIterator.DONE; pos = words.next())
            System.out.println("word=" + pos);
        BreakIterator sentences = BreakIterator.getSentenceInstance(Locale.US);
        sentences.setText("One. Two!");
        for (int pos = sentences.first(); pos != BreakIterator.DONE; pos = sentences.next())
            System.out.println("sentence=" + pos);
    }
}
