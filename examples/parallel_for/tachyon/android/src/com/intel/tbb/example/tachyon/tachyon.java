/*
    Copyright 2005-2016 Intel Corporation.  All Rights Reserved.

    The source code contained or described herein and all documents related
    to the source code ("Material") are owned by Intel Corporation or its
    suppliers or licensors.  Title to the Material remains with Intel
    Corporation or its suppliers and licensors.  The Material is protected
    by worldwide copyright laws and treaty provisions.  No part of the
    Material may be used, copied, reproduced, modified, published, uploaded,
    posted, transmitted, distributed, or disclosed in any way without
    Intel's prior express written permission.

    No license under any patent, copyright, trade secret or other
    intellectual property right is granted to or conferred upon you by
    disclosure or delivery of the Materials, either expressly, by
    implication, inducement, estoppel or otherwise.  Any license under such
    intellectual property rights must be express and approved by Intel in
    writing.
*/

/*
 The original source for this example is
 Copyright (c) 1994-2008 John E. Stone
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 3. The name of the author may not be used to endorse or promote products
 derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 SUCH DAMAGE.
*/

package com.intel.tbb.example.tachyon;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Timer;
import java.util.TimerTask;

import android.app.ActionBar;
import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.content.Context;
import android.content.res.AssetManager;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Gravity;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;

public class tachyon extends Activity {
    private tachyonView myView;
    private int W;
    private int H;
    private String fileOnSDcard; 
    private float currentYMenuPosition=(float) 1e5;
    private float previousYMenuPosition=0;
    public int number_of_threads=0;
    public static TextView txtThreadNumber;
    public static TextView txtElapsedTime;
    
    private static native void setPaused(boolean paused);

    @SuppressWarnings("deprecation")
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        try {
            fileOnSDcard = Environment.getExternalStorageDirectory()
                    .getPath() + "/tachyon/data.dat";
            Log.i("tachyon", "Data file name is " + fileOnSDcard);
            File dataFile = new File(fileOnSDcard);
            if (dataFile.exists()) {
                dataFile.delete();
            }
            if (!dataFile.exists()) {
                AssetManager assetManager = getAssets();
                InputStream inputFile = assetManager.open("data.dat");
                dataFile.getParentFile().mkdirs();
                dataFile.createNewFile();
                OutputStream outputFile = new FileOutputStream(fileOnSDcard);
                byte[] buffer = new byte[10000];
                int bytesRead;
                while ((bytesRead = inputFile.read(buffer)) != -1)
                    outputFile.write(buffer, 0, bytesRead);
                inputFile.close();
                inputFile = null;
                outputFile.flush();
                outputFile.close();
                outputFile = null;
            }
            DisplayMetrics displayMetrics = new DisplayMetrics();
            getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
            ActionBar actionBar = getActionBar();
            actionBar.hide();

            H = displayMetrics.heightPixels;
            W = displayMetrics.widthPixels;
            Log.i("tachyon", "displayMetrics.heightPixels: " + H);
            Log.i("tachyon", "displayMetrics.widthPixels: " + W);

            //uncomment to override scene size 
            int sceneWidth = 400;
            float ratio = W>H?(float)(W)/H:(float)(H)/W;
            W = sceneWidth;
            H = (int) (W/ratio);

            Log.i("tachyon", "Scene size is " + W + "*" + H );

        } catch (Exception e) {
            Log.e("tachyon", "Exception in file copy: " + e.getMessage());
        }
        myView = new tachyonView(this, W, H, fileOnSDcard);
        setContentView(myView);
        
        LinearLayout llThreadNumber = new LinearLayout(this);
        txtThreadNumber = new TextView(this);
        txtThreadNumber.setText("");
        txtThreadNumber.setTextColor(0xFF00FF00);
        txtThreadNumber.setScaleX(1);
        txtThreadNumber.setScaleY(1);
        txtThreadNumber.setPadding(10, 10, 10, 10);
        llThreadNumber.setGravity(Gravity.TOP | Gravity.CENTER);
        llThreadNumber.addView(txtThreadNumber);
        this.addContentView(llThreadNumber,
                    new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));
        LinearLayout llElapsedTime = new LinearLayout(this);
        txtElapsedTime = new TextView(this);
        txtElapsedTime.setText("");
        txtElapsedTime.setTextColor(0xFFFF0000);
        txtElapsedTime.setScaleX(2);
        txtElapsedTime.setScaleY(2);
        txtElapsedTime.setPadding(10, 10, 40, 10);
        llElapsedTime.setGravity(Gravity.TOP | Gravity.RIGHT);
        llElapsedTime.addView(txtElapsedTime);
        this.addContentView(llElapsedTime,
                    new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));
    }

    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.main_screen_menu, menu);
        return true;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
        currentYMenuPosition = event.getY();
        if(event.getAction()==MotionEvent.ACTION_UP ){
        ActionBar actionBar = getActionBar();
            if (previousYMenuPosition < currentYMenuPosition){
                actionBar.show();
            }else{
                actionBar.hide();
            }
            previousYMenuPosition = currentYMenuPosition;
            return true;
        }
        return super.onTouchEvent(event);
    }
    
    @Override
    public void onPause() {
        super.onPause();
        Log.i("tachyon", "onPause working" );
        setPaused(true);
    }
    
    @Override
    public void onResume() {
        super.onResume(); 
        Log.i("tachyon", "onResume working" );
        setPaused(false);
    }
    
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.thread0: {
                number_of_threads = 0;
                break;
            }
            case R.id.thread1: {
                number_of_threads = 1;
                break;
            }
            case R.id.thread2: {
                number_of_threads = 2;
                break;
            }
            case R.id.thread4: {
                number_of_threads = 4;
                break;
            }
            case R.id.thread8: {
                number_of_threads = 8;
                break;
            }
            case R.id.exit: {
                Log.i("tachyon", "Exiting...");
                System.exit(0);
            }
        }
        Log.i("tachyon", "Starting in " + number_of_threads + " Thread(s)");
        myView.initNative(number_of_threads);
        return true;
    }

    static {
        System.loadLibrary("gnustl_shared");
        System.loadLibrary("tbb");
        System.loadLibrary("jni-engine");
    }
}

class tachyonView extends View {
    private Bitmap myBitmap;
    private Rect targetRect;
    private TimerTask myRefreshTask;
    private static Timer myRefreshTimer;
    private int W;
    private int H;
    private String filename;
    public static String strCntDisplay;
    public static String strFpsDisplay;
    
    private static native int renderBitmap(Bitmap bitmap, int size);

    private static native void initBitmap(Bitmap bitmap, int x_size,
            int y_size, int number_of_threads, String fileOnSDcard);

    private static native void pressButton(int x, int y);

    private static native long getElapsedTime();

    public void initNative(int number_of_threads) {
        initBitmap(myBitmap, W, H, number_of_threads, filename);
    }

    public tachyonView(Context context, int widthPixels, int heightPixels, String fileOnSDcard) {
        super(context);

        //Landscape support only: H must be less than W
        //In case application is started on locked phone portrait layout comes 
        //to the constructor even landscape is set in the manifest
        W = widthPixels>heightPixels?widthPixels:heightPixels;
        H = widthPixels>heightPixels?heightPixels:widthPixels;
        filename=fileOnSDcard;
        myBitmap = Bitmap.createBitmap(W, H, Bitmap.Config.ARGB_8888);
        targetRect = new Rect();
        initBitmap(myBitmap, W, H, 0, filename);

    }

    @Override
    protected void onDraw(Canvas canvas) {
        //Write bitmap buffer
        if( renderBitmap(myBitmap, 4 * H * W) == 0 ){
            targetRect.right = canvas.getWidth();
            targetRect.bottom = canvas.getHeight();
            //Draw bitmap buffer
            canvas.drawBitmap(myBitmap, null, targetRect, null);
            tachyon parent = (tachyon)getContext();
            long elapsedTime=getElapsedTime();
            if ( parent.number_of_threads > 0 ){
                parent.getWindow().setTitle(parent.number_of_threads + " Thread(s): " + elapsedTime + " s.");
                tachyon.txtThreadNumber.setText(parent.number_of_threads + " thread(s)");
                tachyon.txtElapsedTime.setText(elapsedTime + " secs");
            }else{
                parent.getWindow().setTitle("HW concurrency: " + elapsedTime + " s.");
                tachyon.txtThreadNumber.setText("Auto HW concurrency");
                tachyon.txtElapsedTime.setText(elapsedTime + " secs");
            }
        }
        invalidate();
        return;
    }
}
