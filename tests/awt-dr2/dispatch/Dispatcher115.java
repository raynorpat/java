package com.apple.rhapsody.awt;
import java.awt.AWTEvent;
import java.awt.event.ActionEvent;
import java.util.Vector;

public class Dispatcher115 {
    static { System.loadLibrary("dispatcher115"); }
    static Thread appkit;
    static Object source = new Object();
    static Vector workers = new Vector();
    static int calls;
    static int menus;
    static Throwable failure;
    static native void initialize();
    static native boolean dispatch(AWTEvent event);
    static native void menu(RMenuItemPeer peer);
    static ActionEvent event(int depth) {
        return new ActionEvent(source, ActionEvent.ACTION_PERFORMED, "dispatch", depth);
    }
    public static boolean callback(AWTEvent value) {
        try {
            if (Thread.currentThread() == appkit) throw new Error("callback on AppKit thread");
            if (!workers.contains(Thread.currentThread())) workers.addElement(Thread.currentThread());
            if (value.getSource() != source) throw new Error("event identity lost");
            int depth = ((ActionEvent)value).getModifiers();
            calls++;
            System.gc();
            if (depth > 0 && dispatch(event(depth - 1)) != ((depth - 1) % 2 == 0))
                throw new Error("nested dispatch result lost");
            return depth % 2 == 0;
        } catch (Throwable problem) { failure = problem; return false; }
    }
    public static void main(String[] args) {
        appkit = Thread.currentThread();
        initialize();
        for (int cycle = 0; cycle < 20; cycle++) {
            if (!dispatch(event(2))) throw new Error("outer dispatch failed: " + failure);
            menu(new RMenuItemPeer());
        }
        if (failure != null || calls != 80 || menus != 20 || workers.size() != 3)
            throw new Error("callbacks=" + calls + " workers=" + workers.size() + " failure=" + failure);
        System.out.println("DR2 Java dispatch re-enters AppKit during callbacks and GC");
        System.exit(0);
    }
    public static void menuCallback() {
        try {
            if (Thread.currentThread() == appkit) throw new Error("menu callback on AppKit thread");
            menus++;
            System.gc();
            if (!dispatch(event(0))) throw new Error("menu callback reentry failed");
        } catch (Throwable problem) { failure = problem; }
    }
}
