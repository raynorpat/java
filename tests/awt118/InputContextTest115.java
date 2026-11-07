package sun.awt.im;

import java.awt.*;
import java.awt.event.*;

public class InputContextTest115 {
    static native void bind(InputContext context, InputMethod method, boolean failed);
    static native void verifyLayout();
    static native Component swapTarget(Object peer, Component target);
    static class Method extends InputMethodAdapter {
        String calls = "";
        public void dispatchEvent(AWTEvent event) { calls += event.getID()==1004 ? "g" : "l"; }
        public void activate() { calls += "a"; }
        public void deactivate() { calls += "d"; }
        public void endComposition() { calls += "e"; }
        public void dispose() { calls += "x"; }
        Component client() { return getClientComponent(); }
    }
    public static void main(String[] args) {
        System.load(args[0]);
        verifyLayout();
        InputContext context = InputContext.getInstance();
        Method method = new Method();
        Canvas client = new Canvas();
        bind(context, method, false);
        context.dispatchEvent(new FocusEvent(client, FocusEvent.FOCUS_GAINED));
        if (context.getClientComponent()!=client || method.client()!=client || !method.calls.equals("ag"))
            throw new RuntimeException("activation ordering");
        context.dispatchEvent(new FocusEvent(client, FocusEvent.FOCUS_LOST));
        if (context.getClientComponent()!=null || !method.calls.equals("agld"))
            throw new RuntimeException("deactivation ordering");
        context.dispatchEvent(new ComponentEvent(client, ComponentEvent.COMPONENT_HIDDEN));
        context.dispose();
        if (!method.calls.equals("agldx")) throw new RuntimeException("dispose ordering");
        bind(context, null, true);
        if (context.getInputMethod()!=null || context.getInputMethod()!=null)
            throw new RuntimeException("cached creation failure");
        System.out.println("peer field layout and input-context lifecycle passed");
        System.exit(0);
    }
}
