package com.cardinalblue.nativegifencoder;

import android.graphics.Bitmap;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.support.v7.app.ActionBarActivity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Toast;

import com.cardinalblue.library.gifencoder.Giffle;

import java.io.File;
import java.util.Random;
import java.util.concurrent.Callable;

import bolts.Continuation;
import bolts.Task;
import pl.droidsonroids.gif.GifImageView;


public class MainActivity extends ActionBarActivity {
    private Random mRandom = new Random();
    private GifImageView mPreviewView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mPreviewView = (GifImageView) findViewById(R.id.gif_preview);

        findViewById(R.id.btn_generate).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Giffle.GiffleBuilder builder = new Giffle.GiffleBuilder();
                final int width = 100;
                final int height = 100;
                int delay = 100; // 100 ms
                final File file = new File(MainActivity.this.getExternalCacheDir(), mRandom.nextInt(10000) + ".gif");
                final Giffle encoder = builder.size(width, height).delay(delay).file(file.getAbsolutePath()).build();
                Toast.makeText(MainActivity.this, "starting generate gif", Toast.LENGTH_SHORT).show();
                Task.callInBackground(new Callable<File>() {
                    @Override
                    public File call() throws Exception {
                        int numFrame = 100;
                        int[] colors = new int[numFrame];
                        for (int i = 0 ; i < numFrame ; i ++) {
                            colors[i] = Color.rgb(mRandom.nextInt(255), mRandom.nextInt(255), mRandom.nextInt(255));
                        }
                        encoder.GenPalette(colors.length, colors);
                        for (int color : colors) {
                            Bitmap bm = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
                            bm.eraseColor(color);
                            encoder.AddFrame(bm);
                        }
                        encoder.Close();
                        return file;
                    }
                }).onSuccess(new Continuation<File, Void>() {
                    @Override
                    public Void then(Task<File> task) throws Exception {
                        Toast.makeText(MainActivity.this, "generated gif", Toast.LENGTH_SHORT).show();
                        mPreviewView.setImageURI(Uri.fromFile(task.getResult()));
                        return null;
                    }
                }, Task.UI_THREAD_EXECUTOR);
            }
        });
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        return super.onOptionsItemSelected(item);
    }
}
