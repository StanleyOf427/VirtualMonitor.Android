/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.

 */

package com.stanley.virtualmonitor;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.content.pm.PackageManager;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.app.Activity;
import androidx.annotation.NonNull;

import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.Manifest;
import android.os.Bundle;
import android.media.MediaPlayer;
import android.os.Handler;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.RelativeLayout;
import android.widget.Toast;;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import butterknife.BindView;
import butterknife.ButterKnife;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Array;
import java.util.Arrays;
import java.util.Timer;
import java.util.TimerTask;
import java.nio.ByteBuffer;

public class MainActivity extends Activity implements SurfaceHolder.Callback {
//    @BindView(R.id.video_view)
//    MyVideoView videoView;
//    @BindView(R.id.lay_finish_bg)
    RelativeLayout layFinishBg;

    private int i=0;
    private int key = 0;
    private Handler handler = new Handler();
    private String TAG = "key";
//    private static UDPServer udpserver;
//    private SimpleServer simpleServer;
    private static int numdata=0;
    private long lastClickTime = 0;

    private final Timer timer = new Timer();
    private TimerTask task;

    private int portRemoteNum=11111;
    private int portLocalNum=22222;
//    private DatagramSocket socketUDP;
    private Thread thread = null;
//    WebSocketServer server;
    private String addressIP="127.0.0.1";
    private static String revData;
    private boolean flag = false;
private TCPServer _tcpserver;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        ButterKnife.bind(this);
//        tmpTest();

        _tcpserver=new TCPServer();

//
//        videoView.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
//            @Override
//            public void onCompletion(MediaPlayer mp) {
//                key = 1;
//                layFinishBg.setVisibility(View.VISIBLE);
//            }
//        });
//
//        videoView.setOnErrorListener(new MediaPlayer.OnErrorListener() {
//            @Override
//            public boolean onError(MediaPlayer mp, int what, int extra) {
//                Toast.makeText(MainActivity.this, "????????????", Toast.LENGTH_SHORT).show();
//                return false;
//            }
//        });
//
//        task = new TimerTask() {
//            @Override
//            public void run() {
//                Message message = new Message();
//                message.what = 1;
////                datasenderhandler.sendMessage(message);
//            }
//        };
//        timer.schedule(task, 2000, 2000);

        mediaCodecBufferInfo = new MediaCodec.BufferInfo();


        //????????????
      checkPermissions();
    }

    private void checkPermissions() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE}, 10086);
        } else {
            //
        }
    }
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        switch (requestCode) {
            case 10086:
                if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                   //
                } else {
                    checkPermissions();
                }
                break;
            default:
                super.onRequestPermissionsResult(requestCode, permissions, grantResults);
                break;
        }
    }


    //region Test
    SurfaceView surfaceView;
    private SurfaceHolder holder;
    private AsyncDecoder decoder;
    public void StartPlay()
    {
        surfaceView = (SurfaceView) findViewById(R.id.testview);
        holder = surfaceView.getHolder();
        //?????? Holder ????????? ???????????? ?????????????????????????????????
        holder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        holder.addCallback(this);

        decoder=new AsyncDecoder();
        decoder.createDecoder();
        decoder.decode(surfaceView);
        new Thread(new Runnable() {
            @Override
            public void run() {
                int i = 80,j=80;
                int k=0;

                //#region sps pps
                byte[] header_sps = {
                        0,
                        0, 0, 1,
                        7,
                        39,77,0,42 ,
                        -107,-80,30,0 ,
                        -119,-7,112,17 ,
                        0,0,3,0,
                        1,0,0,3 ,
                        0,120,-32,64 ,
                        0,7,39,15 ,
                        0,0,28,-100,
                        57,-67,-17,-125 ,
                        -76,56,101,-64
                };
                byte[] header_pps = {
                        0,
                        0 ,0, 1,
                        8,
                        40, -18, 60, -128,0};

                decoder.putData(header_sps);
                decoder.putData(header_pps);
                //#endrdgion
                while (i+3 < data.length)//?????????????????????
                {
                    if((data[i]&0xFF)==0&&(data[i+1]&0xFF)==0&&(data[i+2]&0xFF)==0&&(data[i+3]&0xFF)==1)
                    {
                        j=i;i+=4;
                        while(i+3<data.length)
                        {
                            if((data[i]&0xFF)==0&&(data[i+1]&0xFF)==0&&(data[i+2]&0xFF)==0&&(data[i+3]&0xFF)==1)
                            {
//                                decoder.putData(Arrays.copyOfRange(data,j+4,i-1));
//                                if((i-j)<1000){ i++;continue;}
                                if(((data[j+4]&0xFF)& 0x1F)==1||((data[j+4]&0xFF)& 0x1F)==5)
                                {
                                    decoder.putData(Arrays.copyOfRange(data,j,i));
                                    Log.d("?????????"+(++k)+"???", "???"+(i-j)+"????????????"+j+"???"+(i-1));
                                }
                                break;
                            }
                            i++;
                            if (k>=10) break;
                        }
                    }
                }
            }
        }).start();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    MediaCodec mediaCodec = null;
    ByteBuffer inputBuffer;
    MediaCodec.BufferInfo mediaCodecBufferInfo ;

    private void InitDecoder()
{
    surfaceView = (SurfaceView) findViewById(R.id.testview);
    holder = surfaceView.getHolder();
    //?????? Holder ????????? ???????????? ?????????????????????????????????
    holder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
    holder.addCallback(this);

    //???????????????????????????????????????????????????AVC???????????????
    try {
        mediaCodec = MediaCodec.createDecoderByType(MediaFormat.MIMETYPE_VIDEO_AVC);
    } catch (IOException e) {
        Log.d("CreatDecoder","error!");
    }

    //????????????????????????????????????????????????
    //????????????????????????
    MediaFormat mediaFormat = MediaFormat.createVideoFormat("video/avc", 128, 128);

    //????????????????????????????????????????????????????????????????????????????????????
    mediaCodec.configure(mediaFormat, surfaceView.getHolder().getSurface(), null, 0);
    mediaCodec.start();
}

//    ??????????????????????????????????????????????????????
    public void StartDecode(byte[] buf) {
        try {
            int inputBufferIndex = mediaCodec.dequeueInputBuffer(0);
            if (inputBufferIndex >= 0) {
                inputBuffer = mediaCodec.getInputBuffer(inputBufferIndex);

                inputBuffer.put(buf, 0, buf.length);
                mediaCodec.queueInputBuffer(inputBufferIndex, 0, buf.length, System.nanoTime() , 0);
            }
        } catch (IllegalStateException e) {
            e.printStackTrace();
        }
    }

//    ???????????????surface????????????????????????????????????????????????????????????????????????????????????????????????
    public void RenderView()
    {
        int outputBufferIndex = mediaCodec.dequeueOutputBuffer(mediaCodecBufferInfo, 0);
        if (outputBufferIndex >= 0) {
            mediaCodec.releaseOutputBuffer(outputBufferIndex, true);
        }
        else if (outputBufferIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {


        }
    }




//private void tmpTest()
//{
//    try {
//        if(addressIP!=null)
//        {
//            socketUDP = new DatagramSocket(portLocalNum);
//            InetAddress serverAddress = InetAddress.getByName(addressIP);
//            DatagramPacket packetS = new DatagramPacket("????????????".getBytes(),
//                    "????????????".getBytes().length, serverAddress, portRemoteNum);
//            //????????????????????????IP???????????????????????????
//            socketUDP.send(packetS);
//        }
//    } catch (Exception e) {
//        e.printStackTrace();
//    }
//    Log.d("UDPServer", "UDPServer: ?????????");
//}

//    public void SendUDPPackage(String s)
//    {
//        if(socketUDP!=null)
//        {
//            try {
//                if(addressIP!=null)
//                {
//                    InetAddress serverAddress = InetAddress.getByName(addressIP);
//                    DatagramPacket packetS = new DatagramPacket(("????????????"+s).getBytes(),
//                            ("????????????"+s).getBytes().length, serverAddress, portRemoteNum);
//                    //????????????????????????IP???????????????????????????
//                    socketUDP.send(packetS);
//                }
//            } catch (Exception e) {
//                e.printStackTrace();
//            }
//            Log.d("UDPServer", "UDPServer:????????????"+s);
//        }
//    }

//    private void initVideo() {
//        //????????????
//        videoView.setVideoURI(Uri.parse("android.resource://" + getPackageName() + "/raw/test"));
//
//        //????????????
//        final Uri uri = Uri.parse("http://gslb.miaopai.com/stream/ed5HCfnhovu3tyIQAiv60Q__.mp4");
//        videoView.setVideoURI(uri);
//        videoView.requestFocus();
//
//        videoView.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {
//            @Override
//            public void onPrepared(MediaPlayer mp) {
//                videoView.start();
//            }
//        });
//    }

    byte[] data;
    String logout;
    private int AnalyseData()
    {
        AssetManager manager = this.getAssets();
        try{
            InputStream in = manager.open("test.mp4");//????????????
            this.data = new byte[42 * 1024 * 1024];
            in.read(this.data,0,in.available());
        }
        catch (Exception e)
        {
            e.printStackTrace();
            Log.d(TAG, "file opened failed!");
            return -1;
        }

        Log.d(TAG, "file opened succeed! analysing??????");

        //????????????,????????????00
        int i0 = 80, i1, i2, i3;
        int length, count = 0;
        int[] header=new int[14];
        byte[] b=new byte[15];
        String isoString;//??=??new??String(bytes,??"ISO-8859-1");

        try
        {
//            for(int i=0;i0<160;i0++,i++)
//            {
//                b[3]=32;
//                if(i0%8==0) Log.d(TAG,"");
//
//                b[0]=data[i0++];
//                b[1]=32;
//                b[2]=data[i0++];
//                b[4]=data[i0++];
//                b[5]=32;
//                b[6]=data[i0++];
//                b[7]=32;
//                b[8]=data[i0++];
//                b[9]=32;
//                b[10]=data[i0++];
//                b[11]=32;
//                b[12]=data[i0++];
//                b[13]=32;
//                b[14]=data[i0];
//
//                Log.d(TAG, new String(b,"ISO-8859-1"));
//            }
//            i0=80;
            while (i0 < data.length)//?????????????????????
            {
                i1 = i0 + 1; i2 = i0 + 2; i3 = i0 + 3;
                length = ((data[i0]&0xFF) << 24) + ((data[i1]&0xFF) << 16) + ((data[i2]&0xFF) << 8) + (data[i3]&0xFF);

                if (length == 0) {
                    Log.d(TAG,( "\n" + "??????0,????????????,??????NALU??????:" + count +
                        "\nI????????????" + header[5] +"??????I?????? "+(header[1]+header[2]+header[3]+header[4])+
                        ", SPS???" + header[7]+", "+", PPS??? "+header[8]+"???SEI??? "+header[6]+"???????????? "+header[0]));
                break;
                }
                else {
                    data[i0]=0;
                    data[i1]=0;
                    data[i2]=0;
                    data[i3]=1;

//                    Log.d(TAG,((data[i0]&0xFF)+" "+(data[i0+1]&0xFF)+" "+(data[i0+2]&0xFF)+" "+(data[i0+3]&0xFF)+"    "+"length:"+length+","));
//                    logout="";
//                    for(int k=4;k<length+4;k++)
//                    {
//                        if(k%4==0)
//                            logout+=(data[i0+k]&0xFF)+"   ";
//                        else
//                            logout+=(data[i0+k]&0xFF)+"_";
//                    }
//                    Log.d(TAG,logout);

                    i0 += length+4; count++;
                    if (((data[i3 + 1]&0xFF) & 0x1F) >= 1 && ((data[i3 + 1] &0xFF)& 0x1F) <= 13)
                    {
                        header[(data[i3 + 1]&0xFF) & 0x1F]++;
                        switch ((data[i3 + 1]&0xFF) & 0x1F)
                        {
                            case 1:data[i3 + 1]=1;break;
                            case 5:data[i3 + 1]=5;break;
                            case 6:data[i3 + 1]=6;break;
                            case 9:data[i3 + 1]=9;break;
                        }
                    }
                    else
                        header[0]++;
                }
            }

//            for(i0=0;i0<data.length-80;i0++)
//            {
//                data[i0]=data[i0+80];
//                data[i0]= (byte) (data[i0]&0xFF);
//            }
            Log.d(TAG,"analyse finished!");
        }
        catch (Exception e)
        {
            e.printStackTrace();
            Log.d(TAG, "file analyse failed!");
            return -1;
        }
        return 0;
    }
    //endregion

    public boolean onKeyDown(int keyCode, KeyEvent event) {
        String path = "/storage/emulated/0/DCIM/20210604-1504-21 (copy).mp4";
//                String path = "http://storage.googleapis.com/exoplayer-test-media-1/mkv/android-screens-lavf-56.36.100-aac-avc-main-1280x720.mkv";

        switch (keyCode) {

            case KeyEvent.KEYCODE_ENTER:

                break;//?????????enter
            case KeyEvent.KEYCODE_DPAD_CENTER:
//                Log.d(TAG, "center--->");
//                if(_tcpserver!=null)
//                    _tcpserver.SendMsg(String.valueOf(i++));
//                InitDecoder();
//                AnalyseData();
//                new Thread(new Runnable() {
//                    @Override
//                    public void run() {
//                        StartDecode(data);
//                    }
//                }).start();
//                new Thread(new Runnable() {
//                    @Override
//                    public void run() {
//                        RenderView();
//                    }
//                }).start();
//                StartPlay();


                    surfaceView = (SurfaceView) findViewById(R.id.testview);
                    holder = surfaceView.getHolder();
                JNIUtils.CorePlayer(holder.getSurface(),path,60);

                break;
            case KeyEvent.KEYCODE_BACK:    //?????????
                Log.d(TAG,"back--->");

             /*   if (lastClickTime <= 0) {
                    Toast.makeText(this, "?????????????????????????????????", Toast.LENGTH_SHORT).show();
                    lastClickTime = System.currentTimeMillis();
                }
                else {
                    long currentClickTime = System.currentTimeMillis();
                    if (currentClickTime - lastClickTime < 2000) {
                        task.cancel();
                        finish();
                    } else {
                        Toast.makeText(this, "?????????????????????????????????", Toast.LENGTH_SHORT).show();
                        lastClickTime = System.currentTimeMillis();
                    }
                }*/
//
                surfaceView = (SurfaceView) findViewById(R.id.testview);
                holder = surfaceView.getHolder();
                JNIUtils.CorePlayer(holder.getSurface(),path,60);
                break;

            case KeyEvent.KEYCODE_SETTINGS: //?????????
                Log.d(TAG, "setting--->");
                break;

            case KeyEvent.KEYCODE_DPAD_DOWN:   //?????????

                /*    ?????????????????????????????????????????????????????????????????????????????? ???????????????????????????
                 *    exp:KeyEvent.ACTION_UP
                 */
                if (event.getAction() == KeyEvent.ACTION_DOWN) {
                    Log.d(TAG, "down--->");
                    //
//                    byte[] header_sps = {0, 0, 0, 1, 103, 66, 0 , 41, -115, -115, 64, 80 , 30 , -48 , 15 ,8,-124, 83, -128};
//                    byte[] header_pps = {0,0 ,0, 1, 104, -54, 67, -56};
//                    byte[] header_sps = {0, 0, 0, 1,
//                            0x27,0x4d,0,0x2a,
//                            0x95,0xb0,0x1e,0,
//                            0x89,0xf9,0x70,0x11 ,
//                            0,0,0x03,0,
//                            0x01,0,0,0x03 ,
//                            0,0x78,0xe0,0x40 ,
//                            0,0x07,0x27,0x0e ,
//                            0,0,0x1c,0x9c,
//                            0x39,0xbd,0xef,0x83 ,
//                            0xb4,0x38,0x65,0xc0
//                    };
//                    byte[] header_pps = {0,0 ,0, 1,
//                    0x28, 0xee, 0x3c, 0x80,00};

//                    byte[] header_sps = {0, 0, 0, 1,
//                           103,
//                            39,77,0,0x2a ,
//                            -107,-80,0x1e,0 ,
//                            -119,-7,0x70,0x11 ,
//                            0,0,0x03,0,
//                            0x01,0,0,0x03 ,
//                            0,0x78,-32,0x40 ,
//                            0,0x07,0x27,0x0e ,
//                            0,0,0x1c,-100,
//                            0x39,-67,-17,-125 ,
//                            -76,0x38,0x65,-64
//                    };
//                    byte[] header_pps = {0,0 ,0, 1,
//                            0x28,
//                            0x28, -18, 0x3c, -128,00};
//
//                    for(int k=0;k<45;k++)
//                        Log.d(TAG, (k+1)+"  "+Integer.toHexString((header_sps[k] & 0xFF) + 0x100).substring(1)+
//                                "  "+Integer.toBinaryString((header_sps[k] & 0xFF) + 0x100).substring(1));
//                    Log.d(TAG, "/n");
//                    for(int k=0;k<10;k++)
//                        Log.d(TAG, (k+1)+"  "+Integer.toHexString((header_pps[k] & 0xFF) + 0x100).substring(1)+
//                                "  "+Integer.toBinaryString((header_pps[k] & 0xFF) + 0x100).substring(1));
                }
                break;

            case KeyEvent.KEYCODE_DPAD_UP:   //?????????
                Log.d(TAG, "up--->");
//                Log.d(TAG,JNIUtils.stringFromJNI());

                //region using ffmpeg
//                String path = "/sdcard/Android/data/com.stanley.virtualmonitor/20210604-1504-21 (copy).mp4";
//                surfaceView = (SurfaceView) findViewById(R.id.testview);
//                holder = surfaceView.getHolder();
//                JNIUtils.testDecode(path,holder.getSurface());
                //endregion

                break;

            case KeyEvent.KEYCODE_DPAD_LEFT: //?????????
                Log.d(TAG, "left--->");
//                if (videoView.getCurrentPosition() > 4) {
//                    videoView.seekTo(videoView.getCurrentPosition() - 5 * 1000);
//                }
                break;

            case KeyEvent.KEYCODE_DPAD_RIGHT:  //?????????
                Log.d(TAG, "right--->");
//                videoView.seekTo(videoView.getCurrentPosition() + 5 * 1000);
                break;

            case KeyEvent.KEYCODE_VOLUME_UP:   //???????????????
                Log.d(TAG, "voice up--->");

//????????????
//                AnalyseData();
//                StartPlay();

                //region ???????????????
//                Log.d(TAG,JNIUtils.stringFromJNI());

//                    String path = "/sdcard/Android/data/com.stanley.virtualmonitor/20210604-1504-21 (copy).mp4";
//                    surfaceView = (SurfaceView) findViewById(R.id.testview);
//                    holder = surfaceView.getHolder();
//                    JNIUtils.testDecode(path,holder.getSurface());



//                new Thread(new Runnable() {
//                    @Override
//                    public void run() {
//                        StartDecode(data);
//                    }
//                }).start();
//                new Thread(new Runnable() {
//                    @Override
//                    public void run() {
//                        RenderView();
//                    }
//                }).start();
                //endregion

                break;

            case KeyEvent.KEYCODE_VOLUME_DOWN: //???????????????
                Log.d(TAG, "voice down--->");

                break;
            default:
                break;
       }
       return super.onKeyDown(keyCode, event);
    }
//    @SuppressLint("HandlerLeak")
//    private static Handler  datasenderhandler = new Handler() {
//        @Override
//        public void handleMessage(Message msg) {
//            if(udpserver!=null)
////                udpserver.sendData("test data"+numdata++);
//            super.handleMessage(msg);
//        }
//    };

//    public void onClick(View v) {
//        Log.d(TAG, "click--->");
////                Log.d(TAG,JNIUtils.stringFromJNI());
//
//        String path = "/storage/emulated/0/Download/20210604-1504-21 (copy).mp4";
//        surfaceView = (SurfaceView) findViewById(R.id.testview);
//        holder = surfaceView.getHolder();
//        JNIUtils.testDecode(path,holder.getSurface());
//
//
//             }



    //region surfaceview
//    public class SurfaceViewDemo2 extends SurfaceView implements SurfaceHolder.Callback{
//        private SurfaceHolder mSurfaceHolder;
//        private Canvas mCanvas;
//        private Paint paint;
//        private MediaPlayer mediaPlayer;
//
//        public SurfaceViewDemo2(Context context) {
//            this(context,null,0);
//        }
//
//        public SurfaceViewDemo2(Context context, AttributeSet attrs) {
//            this(context, attrs,0);
//        }
//
//        public SurfaceViewDemo2(Context context, AttributeSet attrs, int defStyleAttr) {
//            super(context, attrs, defStyleAttr);
//            init();
//        }
//
//        private void init() {
//            mSurfaceHolder = getHolder();
//            mSurfaceHolder.addCallback(this);
//            setFocusable(true);
//            setFocusableInTouchMode(true);
//            this.setKeepScreenOn(true);
//            setZOrderOnTop(true);
//            paint = new Paint(Paint.ANTI_ALIAS_FLAG);
//            paint.setColor(Color.RED);
//            paint.setStrokeWidth(5);
//            paint.setStyle(Paint.Style.STROKE);
//        }
//
//        @Override
//        public void surfaceCreated(SurfaceHolder holder) {
//            System.out.println("=========surfaceCreated========");
//            new Thread(new Runnable() {
//                @Override
//                public void run() {
//                    play();
//                }
//            }).start();
//        }
//
//        @Override
//        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
//            System.out.println("=========surfaceChanged========");
//        }
//
//        @Override
//        public void surfaceDestroyed(SurfaceHolder holder) {
//            System.out.println("=========surfaceDestroyed========");
//            if (mediaPlayer != null ){
//                stop();
//            }
//        }
//
//
//        protected void stop() {
//            if (mediaPlayer != null && mediaPlayer.isPlaying()) {
//                mediaPlayer.stop();
//                mediaPlayer.release();
//                mediaPlayer = null;
//            }
//        }
//
//        protected void play() {
//            String path = "/sdcard/DCIM/Camera/VID_20190110_102218.mp4";
//            File file = new File(path);
//            if (!file.exists()) {
//                return;
//            }
//            try {
//                mediaPlayer = new MediaPlayer();
//                // ????????????????????????
//                mediaPlayer.setDataSource(file.getAbsolutePath());
//                // ?????????????????????SurfaceHolder
//                mediaPlayer.setDisplay(getHolder());
//
//                mediaPlayer.prepareAsync();
//                mediaPlayer.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {
//
//                    @Override
//                    public void onPrepared(MediaPlayer mp) {
//                        mediaPlayer.start();
//                    }
//                });
//                mediaPlayer.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
//
//                    @Override
//                    public void onCompletion(MediaPlayer mp) {
//                        replay();
//                    }
//                });
//
//                mediaPlayer.setOnErrorListener(new MediaPlayer.OnErrorListener() {
//
//                    @Override
//                    public boolean onError(MediaPlayer mp, int what, int extra) {
//                        play();
//                        return false;
//                    }
//                });
//            } catch (Exception e) {
//                e.printStackTrace();
//            }
//        }
//
//        protected void replay() {
//            if (mediaPlayer!=null){
//                mediaPlayer.start();
//            }else{
//                play();
//            }
//        }
//
//        protected void pause() {
//            if (mediaPlayer != null && mediaPlayer.isPlaying()) {
//                mediaPlayer.pause();
//            }else{
//                mediaPlayer.start();
//            }
//        }
//    }

    //endregion
}


