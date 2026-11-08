/* Isolated dispatcher test callback, never packaged into the peer archive. */
package com.apple.rhapsody.awt;
import java.awt.AWTEvent;
public class RToolkit {
    public static boolean dispatchEvent(AWTEvent event) {
        return Dispatcher115.callback(event);
    }
}
