package com.stanley.virtualmonitor;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

public class Sender {
    public static void main(String[] args) {
        try {
            // 创建发送方的套接字，IP默认为本地，端口号随机
            DatagramSocket sendSocket = new DatagramSocket();

            // 确定要发送的消息：
            String mes = "你好！接收方！";

            // 由于数据报的数据是以字符数组传的形式存储的，所以传转数据
            byte[] buf = mes.getBytes();

            // 确定发送方的IP地址及端口号，地址为本地机器地址
            int port = 8888;
            InetAddress ip = InetAddress.getLocalHost();

            // 创建发送类型的数据报：
            DatagramPacket sendPacket = new DatagramPacket(buf, buf.length, ip,
                    port);

            // 通过套接字发送数据：
            sendSocket.send(sendPacket);

            // 确定接受反馈数据的缓冲存储器，即存储数据的字节数组
            byte[] getBuf = new byte[1024];

            // 创建接受类型的数据报
            DatagramPacket getPacket = new DatagramPacket(getBuf, getBuf.length);

            // 通过套接字接受数据
            sendSocket.receive(getPacket);

            // 解析反馈的消息，并打印
            String backMes = new String(getBuf, 0, getPacket.getLength());
            System.out.println("接受方返回的消息：" + backMes);

            // 关闭套接字
            sendSocket.close();
        } catch (Exception e) {
            e.printStackTrace();
        }

    }
}