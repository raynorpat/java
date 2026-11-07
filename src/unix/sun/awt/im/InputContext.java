/* Reconstructed from Solaris JDK 1.1.8/16 InputContext.class.
 * Original artifact copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * The generated peer archive retains the supplied LICENSE and COPYRIGHT.
 * Adaptation: dispatch the adapter factory through MToolkit, since JDK 1.1.5's
 * SunToolkit does not declare getInputMethodAdapter. No baseline class changes.
 */
package sun.awt.im;

import java.awt.*;
import sun.awt.motif.MToolkit;

public class InputContext {
    private InputMethod inputMethod;
    private boolean inputMethodCreationFailed;
    private Component currentClientComponent;

    protected InputContext() {}
    public static InputContext getInstance() { return new InputMethodContext(); }
    public synchronized void dispatchEvent(AWTEvent event) {
        InputMethod method = getInputMethod();
        switch (event.getID()) {
        case 103:
            dispose();
            currentClientComponent = null;
            return;
        case 1004:
            activate((Component)event.getSource());
            if (method != null) method.dispatchEvent(event);
            return;
        case 1005:
            if (method != null) method.dispatchEvent(event);
            deactivate((Component)event.getSource());
            return;
        default:
            if (method != null) method.dispatchEvent(event);
        }
    }
    synchronized void activate(Component client) {
        currentClientComponent = client;
        if (inputMethod != null) {
            if (inputMethod instanceof InputMethodAdapter)
                ((InputMethodAdapter)inputMethod).setClientComponent(client);
            inputMethod.activate();
        }
    }
    synchronized void deactivate(Component client) {
        if (inputMethod != null) inputMethod.deactivate();
        currentClientComponent = null;
    }
    Component getClientComponent() { return currentClientComponent; }
    public void dispose() {
        if (inputMethod != null) {
            inputMethod.dispose();
            inputMethod = null;
        }
    }
    InputMethod getInputMethod() {
        if (inputMethod != null) return inputMethod;
        if (inputMethodCreationFailed) return null;
        try {
            Toolkit toolkit = Toolkit.getDefaultToolkit();
            inputMethod = ((MToolkit)toolkit).getInputMethodAdapter();
        } catch (Exception error) {
            inputMethodCreationFailed = true;
        }
        if (inputMethod != null) inputMethod.setInputContext((InputMethodContext)this);
        return inputMethod;
    }
}
