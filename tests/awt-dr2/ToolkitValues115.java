package com.apple.rhapsody.awt;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Toolkit;

public class ToolkitValues115 {
    public static void main(String[] args) {
        try { run(); }
        catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
        System.exit(0);
    }
    static void run() {
        Toolkit toolkit = Toolkit.getDefaultToolkit();
        Dimension size = toolkit.getScreenSize();
        Color color = RToolkit.getDefaultBackgroundColor();
        if (size.width <= 0 || size.height <= 0 || color == null) throw new Error("invalid screen values");
        if (toolkit.getScreenResolution() <= 0) throw new Error("invalid resolution");
        System.out.println("screen=" + size.width + "x" + size.height);
        System.out.println("resolution=" + toolkit.getScreenResolution());
        System.out.println("background=" + color.getRed() + "," + color.getGreen() + "," + color.getBlue());
        toolkit.sync();
        toolkit.beep();
        System.exit(0);
    }
}
