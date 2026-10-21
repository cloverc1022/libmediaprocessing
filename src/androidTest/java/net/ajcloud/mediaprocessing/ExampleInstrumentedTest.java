package net.ajcloud.mediaprocessing;

import android.os.Environment;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import android.util.Log;

/**
 * Instrumented test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
public class ExampleInstrumentedTest {
    @Test
    public void useAppContext() {
        String downloadPath =
                InstrumentationRegistry.getInstrumentation().getTargetContext().getFilesDir().getAbsolutePath()
                        + File.separator + Environment.DIRECTORY_DCIM + File.separator + "wansviewplus" + File.separator + "Video" + File.separator + "1161556615" + File.separator + "download" + File.separator + "17787571510001778757322000-1";
        Log.d("TS2MP4Test", "downloadPath: " + downloadPath);
        String filesPath = downloadPath + File.separator + "20260514_191231-1.ts" +
                "," +
                downloadPath + File.separator + "20260514_191411-1.ts" +
                "," +
                downloadPath + File.separator + "20260514_191512-1.ts";
        MediaProcessing.INSTANCE.TS2MP4(
                filesPath,
                0,
                0,
                8000,
                1,
                downloadPath + File.separator + "vlc-record-" + System.currentTimeMillis() + "test-rtsp.mp4"
        );
    }
}