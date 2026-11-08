import java.awt.*;
import java.awt.event.*;

public class Menu115 {
    static { System.loadLibrary("menu115"); }
    static native void track();
    static native void failConstruction(int kind);
    static native int live();
    static native boolean state(MenuItem item, String label, boolean enabled, int state);
    static native boolean hierarchy(MenuBar bar, Menu file, Menu nested, Menu help);
    static native void choose(MenuItem item);
    static native void shortcut(Frame frame);
    static native void popupKeys(Frame frame);
    static native boolean active(MenuBar bar);
    static native void activate(Frame frame);
    static volatile int actions, changes;
    static volatile Throwable failure;
    static volatile boolean disposeOnAction;
    static void awaitAction(int before) throws Exception {
        for (int wait = 0; wait < 200 && actions == before && failure == null; wait++) Thread.sleep(10);
        if (failure != null) throw new Error(failure.toString());
        require(actions > before, "missing menu action");
    }
    static final RuntimeException marker = new RuntimeException("menu font failure");
    static class BadFont extends Font {
        BadFont() { super("Serif", Font.PLAIN, 12); }
        public int getSize() { throw marker; }
    }
    static void failedConstruction(Menu parent, int kind, boolean nativeFailure) throws Exception {
        MenuItem item;
        if (kind == 0) item = new MenuItem("failure");
        else if (kind == 1) item = new CheckboxMenuItem("failure");
        else item = new Menu("failure");
        int before = live();
        if (!nativeFailure) item.setFont(new BadFont());
        failConstruction(nativeFailure ? (kind == 2 ? 2 : 1) : 0);
        try { parent.add(item); throw new Error("menu construction succeeded"); }
        catch (RuntimeException problem) {
            if (!nativeFailure) require(problem == marker, "menu throwable identity");
            else require(problem.getMessage().indexOf("menu construction failure") >= 0, "native constructor exception");
        } finally { failConstruction(0); parent.remove(item); }
        require(item.getPeer() == null, "failed menu peer");
        for (int wait = 0; wait < 100 && live() != before; wait++) {
            Thread.sleep(50); Toolkit.getDefaultToolkit().sync(); System.gc();
        }
        require(live() == before, "failed menu resources retained");
    }
    static void require(boolean value, String message) { if (!value) throw new Error(message); }
    public static void main(String[] args) {
        try {
            boolean tracked = args.length != 0;
            if (tracked) { Toolkit.getDefaultToolkit(); track(); }
            final Frame frame = new Frame("Menu fixture"); frame.setBounds(350, 100, 300, 240);
            MenuBar bar = new MenuBar();
            Menu file = new Menu("File"), nested = new Menu("Nested"), help = new Menu("Help");
            final MenuItem first = new MenuItem("caf\u00e9", new MenuShortcut(65));
            final CheckboxMenuItem checked = new CheckboxMenuItem("Checked", true);
            file.add(first); file.addSeparator(); file.add(checked);
            nested.add("duplicate"); nested.add("duplicate"); file.add(nested);
            bar.add(file); bar.setHelpMenu(help); help.add("About");
            frame.setMenuBar(bar); frame.show(); Toolkit.getDefaultToolkit().sync();
            if (tracked) for (int kind = 0; kind < 3; kind++) {
                failedConstruction(file, kind, false); failedConstruction(file, kind, true);
            }
            MenuItem separator = new MenuItem("-"); separator.setFont(new BadFont());
            file.add(separator); file.remove(separator);
            first.setLabel("renamed"); first.setShortcut(new MenuShortcut(66, true));
            first.setEnabled(false); first.setEnabled(true);
            checked.setState(false); checked.setState(true);
            nested.setLabel("Submenu"); nested.setEnabled(false); nested.setEnabled(true);
            file.setFont(new Font("Dialog", Font.PLAIN, 16));
            first.setFont(new Font("Dialog", Font.BOLD, 18));
            nested.remove(0); nested.add("replacement");
            require(file.getItemCount() == 4 && nested.getItemCount() == 2, "menu items");
            require(hierarchy(bar, file, nested, help), "native hierarchy");
            require(state(first, "renamed", true, -1) && state(checked, "Checked", true, 1), "native item state");
            first.setActionCommand("menu action");
            first.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent event) {
                    try {
                        require(event.getSource() == first && event.getActionCommand().equals("menu action"), "menu action source/command");
                        Toolkit.getDefaultToolkit().sync(); System.gc();
                        if (disposeOnAction) frame.dispose();
                        actions++;
                    } catch (Throwable problem) { failure = problem; }
                }
            });
            checked.addItemListener(new ItemListener() {
                public void itemStateChanged(ItemEvent event) {
                    try {
                        require(event.getSource() == checked && !checked.getState() && event.getStateChange() == ItemEvent.DESELECTED, "checkbox menu callback");
                        Toolkit.getDefaultToolkit().sync(); System.gc(); changes++;
                    } catch (Throwable problem) { failure = problem; }
                }
            });
            int before = actions; choose(first); awaitAction(before);
            choose(checked);
            require(changes == 1 && failure == null && state(checked, "Checked", true, 0), "native checkbox menu toggle");
            first.setEnabled(false); before = actions; shortcut(frame); Thread.sleep(150);
            require(actions == before, "disabled menu shortcut");
            first.setEnabled(true); before = actions; shortcut(frame); awaitAction(before);
            frame.setMenuBar(null); frame.setMenuBar(bar);
            frame.hide(); frame.show();
            Frame other = new Frame("Other menu"); other.setBounds(680, 100, 200, 180);
            MenuBar otherBar = new MenuBar(); Menu otherMenu = new Menu("Other"); otherMenu.add("Item");
            otherBar.add(otherMenu); other.setMenuBar(otherBar); other.show(); activate(other);
            require(active(otherBar), "second window menu activation");
            activate(frame); require(active(bar), "first window menu activation");
            other.dispose(); Thread.sleep(250);
            require(active(bar), "delayed hide removed active menu");
            if (tracked) { disposeOnAction = true; before = actions; choose(first); awaitAction(before); }
            frame.dispose(); Toolkit.getDefaultToolkit().sync(); System.gc();
            if (tracked) {
                for (int wait = 0; wait < 100 && live() != 0; wait++) {
                    Thread.sleep(50); Toolkit.getDefaultToolkit().sync(); System.gc();
                }
                require(live() == 0, "native menus retained: " + live());
            }
            System.out.println("DR2 menus preserve hierarchy, labels, state and attachment");
            System.exit(0);
        } catch (Throwable problem) { problem.printStackTrace(); System.exit(1); }
    }
}
