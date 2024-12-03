package com.opentraa.traa;

public class NativeLib {

    // Used to load the 'traa' library on application startup.
    static {
        System.loadLibrary("traa");
    }

    /**
     * A native method that is implemented by the 'traa' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}