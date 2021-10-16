package com.stanley.virtualmonitor;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Debug;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;

public class UDPServer {

    //收发用同一个socket	，只在连接时实例化一次
    private DatagramSocket socketUDP = null;
    private Thread thread = null;
    private int portRemoteNum=22222;
    private int portLocalNum=11110;
    private String addressIP="127.0.0.1";
    private static String revData;
    private boolean flag = false;


    UDPServer() {
        try {
            socketUDP = new DatagramSocket(portLocalNum);
//            thread = new Thread(revMsg);
//            thread.start();

//            byte data[] = new byte[1024];
//            DatagramPacket packetR = new DatagramPacket(data, data.length);
//            try {
//                socketUDP.receive(packetR);
//                revData = new String(packetR.getData(),
//                        packetR.getOffset(), packetR.getLength());
//            } catch (IOException e) {
//                // TODO Auto-generated catch block
//                e.printStackTrace();
//            }

            try {
                if(addressIP!=null)
                {
                    InetAddress serverAddress = InetAddress.getByName(addressIP);
                    DatagramPacket  packetS = new DatagramPacket("测试文本".getBytes(),
                            "测试文本".getBytes().length, serverAddress, portRemoteNum);
                    //从本地端口给指定IP的远程端口发数据包
                    socketUDP.send(packetS);
                }
            } catch (Exception e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            Log.d("UDPServer", "UDPServer: 已连接");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

public void SendUDPPackage()
{
    if(socketUDP!=null)
    {
        try {
            if(addressIP!=null)
            {
                InetAddress serverAddress = InetAddress.getByName(addressIP);
                DatagramPacket  packetS = new DatagramPacket("测试文本".getBytes(),
                        "测试文本".getBytes().length, serverAddress, portRemoteNum);
                //从本地端口给指定IP的远程端口发数据包
                socketUDP.send(packetS);
            }
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        Log.d("UDPServer", "UDPServer:发送数据");
    }

}

    public void SendUDPPackage(String s)
    {
        if(socketUDP!=null)
        {
            try {
                if(addressIP!=null)
                {
                    InetAddress serverAddress = InetAddress.getByName(addressIP);
                    DatagramPacket packetS = new DatagramPacket(("测试文本"+s).getBytes(),
                            ("测试文本"+s).getBytes().length, serverAddress, portRemoteNum);
                    //从本地端口给指定IP的远程端口发数据包
                    socketUDP.send(packetS);
                }
            } catch (Exception e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            Log.d("UDPServer", "UDPServer:发送数据"+s);
        }

    }


    public void CloseLink() {
        flag = false;//结束线程
        socketUDP.close();
    }

//    @SuppressLint("HandlerLeak")
//    private static Handler handler = new Handler() {
//        @Override
//        public void handleMessage(Message msg) {
//            if (msg != null&&revData!=null) {
//                StringBuilder sb = new StringBuilder();
//                sb.append("\n");
//                sb.append(revData);
//                Log.d("收到数据", (sb.toString().trim()));//测试
//                sb.delete(0, sb.length());
//                sb = null;
//            }
//        }
//    };

//    private Runnable revMsg = new Runnable() {
//        @Override
//        public void run() {
//            while (flag) {
//                byte data[] = new byte[1024];
//            DatagramPacket packetR = new DatagramPacket(data, data.length);
//            try {
//                socketUDP.receive(packetR);
//                revData = new String(packetR.getData(),
//                        packetR.getOffset(), packetR.getLength());
//                handler.sendEmptyMessage(0);
//            } catch (IOException e) {
//                // TODO Auto-generated catch block
//                e.printStackTrace();
//            }
//            }
//        }
//    };
//
//    void sendData(String msg) {
//        try {
//            if(addressIP!=null)
//            {
//                InetAddress serverAddress = InetAddress.getByName(addressIP);
//                DatagramPacket packetS = new DatagramPacket(msg.getBytes(),
//                        msg.getBytes().length, serverAddress, portRemoteNum);
//                //从本地端口给指定IP的远程端口发数据包
//                socketUDP.send(packetS);
//            }
//        } catch (Exception e) {
//            // TODO Auto-generated catch block
//            e.printStackTrace();
//        }
//    }

}