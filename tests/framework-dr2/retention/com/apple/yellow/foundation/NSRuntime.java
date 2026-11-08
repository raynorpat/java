package com.apple.yellow.foundation;

import java.util.Hashtable;
import java.util.Enumeration;

/* Isolated dependency fixture, not a replacement for the shipped NSRuntime.
 * The two private methods reproduce its donor retention-table bytecode. */
public class NSRuntime {
    /* NativeObject class initialization seam; do not load the donor bridge. */
    public static String[] librarySearchPaths() { return new String[0]; }
    private static Hashtable _retainedObjects = new Hashtable();
    private static void retainObject(int key, Object object) {
        _retainedObjects.put(new Integer(key), object);
    }
    private static void releaseObject(int key) {
        _retainedObjects.remove(new Integer(key));
    }
    public static Object make(int value) { return new Integer(value); }
    public static int count() { return _retainedObjects.size(); }
    public static int collectAndSum() {
        System.gc();
        int sum = 0;
        Enumeration values = _retainedObjects.elements();
        while (values.hasMoreElements()) sum += ((Integer)values.nextElement()).intValue();
        return sum;
    }
    public static void releaseAll() {
        _retainedObjects.clear();
        System.gc();
    }
}
