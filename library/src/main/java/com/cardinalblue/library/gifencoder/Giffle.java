package com.cardinalblue.library.gifencoder;

import android.graphics.Bitmap;

/**
 * Created by prada on 15/2/12.
 */
public class Giffle {
    static {
        System.loadLibrary("gifflen");
    }

    /**
     * Init the gif file
     * @param gifName name
     * @param w width
     * @param h height
     * @param numColors colors
     * @param quality
     * @param frameDelay times
     * @return
     */
    private native int Init(String gifName, int w, int h, int numColors, int quality, int frameDelay);
    public native void Close();
    public native int AddFrame(int[] pixels);

    public int AddFrame(Bitmap bitmap) {
        int[] pixels = new int[bitmap.getWidth() * bitmap.getHeight()];
        bitmap.getPixels(pixels, 0, bitmap.getWidth(), 0, 0, bitmap.getWidth(), bitmap.getHeight());
        return AddFrame(pixels);
    }
    public native void GenPalette(int len, int[] pixels);

    public static class GiffleBuilder {
        private String file;
        private int delay;
        private int w;
        private int h;
        private int numColor = 256;
        private int qaulity = 100;

        public GiffleBuilder file(String file) {
            this.file = file;
            return this;
        }

        public GiffleBuilder delay(int delayMillSecond) {
            this.delay = delayMillSecond / 10;
            return this;
        }

        public GiffleBuilder size(int w, int h) {
            this.w = w;
            this.h = h;
            return this;
        }

        public GiffleBuilder numColor(int numColor) {
            this.numColor = numColor;
            return this;
        }

        public GiffleBuilder qaulity(int qaulity) {
            this.qaulity = qaulity;
            return this;
        }

        public Giffle build() {
            Giffle giffle = new Giffle();
            giffle.Init(file, w, h, numColor, qaulity, delay);
            return giffle;
        }
    }
}
