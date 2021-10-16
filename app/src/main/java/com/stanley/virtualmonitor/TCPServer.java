package com.stanley.virtualmonitor;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;

import android.app.Activity;
import android.content.SharedPreferences.Editor;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

public class TCPServer {

    ServerSocket serverSocket;//创建ServerSocket对象
    Socket clicksSocket;//连接通道，创建Socket对象
    String s;
    Button startButton;//发送按钮
    EditText portEditText;//端口号
    EditText receiveEditText;//接收消息框
    Button sendButton;//发送按钮
    EditText sendEditText;//发送消息框
    InputStream inputstream;//创建输入数据流
    OutputStream outputStream;//创建输出数据流

    TCPServer()
    {
        ServerSocket_thread serversocket_thread = new ServerSocket_thread();
        serversocket_thread.start();
    }

    /**
     * 服务器监听线程
     */
    class ServerSocket_thread extends Thread
    {
        public void run()//重写Thread的run方法
        {
            try
            {
                serverSocket = new ServerSocket(22222);//监听port端口，这个程序的通信端口就是port了
            }
            catch (IOException e)
            {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            while (true)
            {
                try
                {
                    //监听连接 ，如果无连接就会处于阻塞状态，一直在这等着
                    clicksSocket = serverSocket.accept();
                    inputstream = clicksSocket.getInputStream();//
                    //启动接收线程
                    Receive_Thread receive_Thread = new Receive_Thread();
                   // Send_Thread send_thread=new Send_Thread();
                    receive_Thread.start();
                 //   send_thread.start();
                }
                catch (IOException e)
                {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
        }
    }

    //region socket通信
    /**
     *
     * 接收线程
     *
     */
    class Receive_Thread extends Thread//继承Thread
    {
        public void run()//重写run方法
        {
            while (true)
            {
                try
                {
                    final byte[] buf = new byte[1024];
                    final int len = inputstream.read(buf);

//                    BufferedReader br = new BufferedReader(new InputStreamReader(inputstream));
//                    PrintWriter pw = new PrintWriter( new FileWriter("files\\abc.txt"),true);

                    Log.d("TCP", new String(buf,0,len));
                }
                catch (Exception e)
                {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
        }
    }

    /**
     *
     * 发送线程
     *
     */
    class Send_Thread extends Thread//继承Thread
    {
        public void run()//重写run方法
        {
            while (true)
            {
                try
                {
                    if(s!=null)
                    {
                        clicksSocket.getOutputStream().write(s.getBytes());
                        Log.d("Send", s);
                    }
                }
                catch (Exception e)
                {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
        }
    }


    public void SendMsg(String str)
    {
       s=str;
    }

    //endregion
}
