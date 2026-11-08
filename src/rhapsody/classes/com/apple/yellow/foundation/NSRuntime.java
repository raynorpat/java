/* Reconstructed from DR2 NSRuntime bytecode. Bootstrap uses the private VM
 * library search path instead of the donor's fixed /usr/lib/java path. */
package com.apple.yellow.foundation;
import java.io.File;
import java.util.Vector;
import java.util.Hashtable;

public class NSRuntime {
    private static Vector _loadedLibraries = new Vector(1);
    private static String[] _searchPaths;
    private static Hashtable _retainedObjects = new Hashtable();
    private static final native String[] initialSearchPaths();
    private static final native void initializeObjcLibrary(String name, String path);
    private NSRuntime() {}
    private static String libraryPrefix() {
        String os = System.getProperty("os.name");
        return os.equals("Rhapsody") || os.equals("Solaris") || os.equals("HP-UX") ? "lib" : null;
    }
    private static String librarySuffix() {
        String os = System.getProperty("os.name");
        if (os.equals("Rhapsody")) return ".dylib";
        if (os.startsWith("Windows")) return ".dll";
        if (os.equals("Solaris")) return ".so";
        if (os.equals("HP-UX")) return ".sl";
        return null;
    }
    private static String libraryBaseName(String name) {
        String prefix = libraryPrefix(), suffix = librarySuffix();
        int start = name.lastIndexOf('/') + 1, end = name.length();
        if (prefix != null && name.regionMatches(start, prefix, 0, prefix.length())) start += prefix.length();
        if (suffix != null && name.endsWith(suffix)) end -= suffix.length();
        return name.substring(start, end);
    }
    private static boolean isAbsolutePath(String path, String os) {
        return path.charAt(0) == '/' || (os.startsWith("Windows") && path.charAt(1) == ':');
    }
    private static boolean shouldTryToLoad(String path, String os) {
        return !isAbsolutePath(path, os) || new File(path).exists();
    }
    private static String loadLibraryFromPath(String path, String name) {
        String os = System.getProperty("os.name");
        if (path == null) path = "";
        try {
            String candidate = path + name;
            if (shouldTryToLoad(candidate, os)) { System.load(candidate); return candidate; }
        } catch (UnsatisfiedLinkError error) {}
        String suffix = librarySuffix(), prefix = libraryPrefix();
        if (prefix == null) prefix = "";
        if (suffix != null && !path.endsWith(suffix)) {
            try {
                String candidate = path + prefix + name + suffix;
                if (shouldTryToLoad(candidate, os)) { System.load(candidate); return candidate; }
            } catch (UnsatisfiedLinkError error) {}
        }
        return null;
    }
    private static String searchForLibrary(String name) {
        if (System.getProperty("os.name").equals("HP-UX")) return name;
        if (loadLibraryFromPath(null, name) != null) return name;
        if (isAbsolutePath(name, System.getProperty("os.name"))) throw new UnsatisfiedLinkError(name);
        String[] paths = librarySearchPaths();
        for (int i = 0; i < paths.length; ++i) {
            String result = loadLibraryFromPath(paths[i], name);
            if (result != null) return result;
        }
        throw new UnsatisfiedLinkError(name);
    }
    public static void loadLibrary(String name) {
        try {
            String base = libraryBaseName(name);
            synchronized (_loadedLibraries) {
                if (!_loadedLibraries.contains(base)) {
                    _loadedLibraries.addElement(base);
                    name = searchForLibrary(name);
                    initializeObjcLibrary(base, name);
                }
            }
        } catch (Error error) {
            System.err.println("NSRuntime.loadLibrary() error.");
            error.printStackTrace(System.err);
            throw error;
        }
    }
    private static void initializeSearchPaths() {
        if (_searchPaths == null) _searchPaths = initialSearchPaths();
    }
    public static String[] librarySearchPaths() { initializeSearchPaths(); return _searchPaths; }
    public static void setLibrarySearchPaths(String[] paths) { if (paths != null) _searchPaths = paths; }
    public static void addPathToLibrarySearchPaths(String path) {
        if (path == null) return;
        initializeSearchPaths();
        if (path.charAt(path.length() - 1) != '/') path += "/";
        String[] paths = new String[_searchPaths.length + 1];
        paths[0] = path;
        System.arraycopy(_searchPaths, 0, paths, 1, _searchPaths.length);
        _searchPaths = paths;
    }
    public static void removePathFromLibrarySearchPaths(String path) {
        if (path == null) return;
        initializeSearchPaths();
        for (int i = 0; i < _searchPaths.length; ++i) {
            if (path.equals(_searchPaths[i])) {
                String[] paths = new String[_searchPaths.length - 1];
                System.arraycopy(_searchPaths, 0, paths, 0, i);
                System.arraycopy(_searchPaths, i + 1, paths, i, paths.length - i);
                _searchPaths = paths;
                break;
            }
        }
    }
    private static void retainObject(int key, Object value) { _retainedObjects.put(new Integer(key), value); }
    private static void releaseObject(int key) { _retainedObjects.remove(new Integer(key)); }
    static {
        String bridge = System.getProperty("com.apple.yellow.foundation.ObjCJava");
        if (bridge == null) bridge = System.getProperty("java.home") + "/lib/libObjCJava.dylib";
        if (!new File(bridge).isAbsolute()) throw new UnsatisfiedLinkError("ObjCJava path must be absolute");
        System.load(bridge);
        _loadedLibraries.addElement("ObjCJava");
        initializeObjcLibrary("ObjCJava", bridge);
    }
}
