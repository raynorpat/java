package com.apple.rhapsody.awt;

import java.awt.*;

public class Print115 {
    static { System.loadLibrary("print115"); }
    static native int constant(String name);
    static native void cancelPanel();
    static native boolean surfaceGraphics();
    static native void donorDispose(RPrintJob job);
    static native void track();
    static native int liveObjects();
    static native void failConstruction(boolean fail);
    static int callbacks;
    public static void modalTick() { callbacks++; Toolkit.getDefaultToolkit().sync(); System.gc(); }
    static void released() throws Exception {
        for (int wait = 0; wait < 100 && liveObjects() != 0; wait++) {
            Toolkit.getDefaultToolkit().sync(); Thread.sleep(10);
        }
        if (liveObjects() != 0) throw new Error("Print native resources leaked: " + liveObjects());
    }
    public static void main(String[] args) {
        try {
            Toolkit.getDefaultToolkit();
            if (args.length == 0) track();
            int save = constant("NSPrintSaveJob"), cancel = constant("NSPrintCancelJob");
            if (save == 0 || cancel == 0 || save == cancel || constant("NSPrintSpoolJob") == save ||
                constant("NSPortraitOrientation") != 0 || constant("NSLandscapeOrientation") != 1)
                throw new Error("Print constants mismatch");
            for (int cycle = 0; cycle < 3; cycle++) {
                String destination = "/private/tmp/phase7-print-unused.ps";
                RPrintJob job = new RPrintJob(null, "DR2 print", save, destination, cycle + 1, cycle % 2, "Letter");
                if (!job.isValid() || job.nGetDisposition() != save || !destination.equals(job.nGetDestinationName()) ||
                    job.nGetNumCopies() != cycle + 1 || job.nGetOrientation() != 0 || !"Letter".equals(job.nGetPaperName()))
                    throw new Error("Print job properties mismatch " + cycle + ": " + job.isValid() + ":" + job.nGetDisposition() + ":" + job.nGetDestinationName() + ":" + job.nGetNumCopies() + ":" + job.nGetOrientation() + ":" + job.nGetPaperName() + ":save=" + save);
                if (!surfaceGraphics()) throw new Error("Print surface mismatch");
                cancelPanel();
                if (job.nShowPrintDialog() != cancel) throw new Error("Print cancellation mismatch");
                if (args.length != 0) donorDispose(job);
                else { job.end(); job.end(); }
                if (job.isValid()) throw new Error("Print job not disposed");
                System.gc(); Toolkit.getDefaultToolkit().sync();
                if (args.length == 0) released();
            }
            if (callbacks != 3) throw new Error("Missing modal callback");
            if (args.length == 0) {
                failConstruction(true);
                RPrintJob failed = new RPrintJob(null, "Fail", save, null, 1, 0, null);
                failConstruction(false);
                if (failed.isValid()) throw new Error("Print constructor rollback failed");
                released();
                RPrintJob job = new RPrintJob(null, "Recovery", save, null, 1, 0, null);
                try { job.getGraphics(); throw new Error("Donor print job is not a drawing surface"); }
                catch (RuntimeException expected) {
                    if (expected.getMessage() == null || expected.getMessage().indexOf("awtColor") < 0) throw expected;
                }
                job.end();
                released();
            }
            System.out.println("DR2 print constants, job properties, cancellation and surfaces passed");
            System.exit(0);
        } catch (Throwable problem) { problem.printStackTrace(); System.exit(1); }
    }
}
