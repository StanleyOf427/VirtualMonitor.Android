package com.stanley.virtualmonitor;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.os.Build;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.ArrayBlockingQueue;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;

public class AsyncDecoder {

    private static final String StringTAG ="AsyncDecoder";

    private MediaFormat mediaFormat;

    private MediaCodec mediaCodec;

    private static final String TYPE_AVC ="video/avc";

    private int width=1920, height=1080;

    private int framerate =60;

//    private int bitrate =width *height *framerate *8;
    private int bitrate =25296*1024;

    private static final int MAX_QUEUE_SIZE =12;

    private ArrayBlockingQueue queue =new ArrayBlockingQueue<>(MAX_QUEUE_SIZE);

    private Surface surface;

    private int flag;

    public void createDecoder(){

        try {
            mediaCodec = MediaCodec.createDecoderByType(TYPE_AVC);

            mediaFormat = MediaFormat.createVideoFormat(TYPE_AVC, width, height);


//            byte[] header_sps = {0, 0, 0, 1,
//                    103,
//                    39,77,0,0x2a ,
//                    -107,-80,0x1e,0 ,
//                    -119,-7,0x70,0x11 ,
//                    0,0,0x03,0,
//                    0x01,0,0,0x03 ,
//                    0,0x78,-32,0x40 ,
//                    0,0x07,0x27,0x0e ,
//                    0,0,0x1c,-100,
//                    0x39,-67,-17,-125 ,
//                    -76,0x38,0x65,-64
//            };
            byte[] header_sps = {
                    0,
                    0, 0, 1,
                    103,
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
//            byte[] header_pps = {0,0 ,0, 1,
//                    104,
//                    0x28, -18, 0x3c, -128,00};
//
            byte[] header_pps = {
                    0,
                    0 ,0, 1,
                    104,
                    40, -18, 60, -128,0};
//            mediaFormat.setByteBuffer("csd-0", ByteBuffer.wrap(header_sps));//设定SPS信息
//            mediaFormat.setByteBuffer("csd-1", ByteBuffer.wrap(header_pps));//设定PPS信息



            mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, bitrate);

            mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, framerate);

            mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible);

            mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 128);

        }catch (IOException e) {
            e.printStackTrace();
        }

    }

    public void putData(byte[] data){
        if(queue.size()>=MAX_QUEUE_SIZE)
        {queue.poll();}
        queue.add(data.clone());
    }

    public void setSurface(Surface surface) {
        this.surface = surface;
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public void decode(SurfaceView surfaceView){
        mediaCodec.setCallback(new MediaCodec.Callback() {
            @Override
            public void onInputBufferAvailable(@NonNull MediaCodec codec, int index) {
                ByteBuffer decoderInputBuffer =mediaCodec.getInputBuffer(index);
                decoderInputBuffer.clear();

                byte[] input = (byte[]) queue.poll();
                if(input!=null) {

                    decoderInputBuffer.limit(input.length);

                    decoderInputBuffer.put(input, 0, input.length);

                    if(((input[4]) & 0x1F)==8||((input[4]) & 0x1F)==7)
                        flag=mediaCodec.BUFFER_FLAG_CODEC_CONFIG;
                    else
                    if(((input[4]&0xFF) & 0x1F)==5) flag=mediaCodec.BUFFER_FLAG_KEY_FRAME;
                    else
                        flag=0;
                    Log.d("flag", flag+"");
                    mediaCodec.queueInputBuffer(index, 0, input.length, computePresentationTime(index), flag);
//                    mediaCodec.queueInputBuffer(index, 0, input.length, 0, flag);

                }
            }

            @Override
            public void onOutputBufferAvailable(@NonNull MediaCodec codec, int index, @NonNull MediaCodec.BufferInfo info) {
                mediaCodec.releaseOutputBuffer(index, true);
            }

            @Override
            public void onError(@NonNull MediaCodec codec, @NonNull MediaCodec.CodecException e) {
                Log.d("OmError()", e.toString() );
                mediaCodec.reset();

            }

            @Override

            public void onOutputFormatChanged(@NonNull MediaCodec codec, @NonNull MediaFormat format) {
                Log.d("OnOutputFormatChanged()", "输出格式改变");            }
        });

//        mediaCodec.configure(mediaFormat, surface, null, 0);

        //！！！注意，这行代码需要界面绘制完成之后才可以调用！！！
        mediaCodec.configure(mediaFormat, surfaceView.getHolder().getSurface(), null, 0);
        mediaCodec.start();

    }

    private long computePresentationTime(long frameIndex){

        return 132+frameIndex*1000000/framerate;

    }

    public void stop(){

        mediaCodec.stop();

        mediaCodec.release();

        mediaFormat =null;

        mediaCodec =null;

    }

}