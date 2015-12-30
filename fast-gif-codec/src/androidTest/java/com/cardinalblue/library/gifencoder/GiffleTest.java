package com.cardinalblue.library.gifencoder;

import android.test.AndroidTestCase;

import java.io.File;
import java.io.IOException;

/**
 * Created by prada on 15/5/24.
 */
public class GiffleTest extends AndroidTestCase {
    public void testBuilderWithNormalCase() {
        Giffle.GiffleBuilder b = new Giffle.GiffleBuilder();
        try {
            File f = File.createTempFile("giffle", "gif");
            Giffle encoder = b.delay(10).size(10, 10).file(f).build();
            assertNotNull(encoder);
        } catch (IOException e) {
            fail(e.getMessage());
        }
    }

    public void testBuilderWithIAE() {
        Giffle.GiffleBuilder b = new Giffle.GiffleBuilder();
        try {
            Giffle encoder = b.build();
            fail("should throw IAE");
        } catch (IllegalArgumentException e) {
        } catch (Throwable t) {
            fail("should not throw another exception like : " + t);
        }
    }
}
