/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/Classes/Class.java to edit this template
 */
package klient;

import java.awt.Color;
import java.awt.event.WindowEvent;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;
import javax.swing.JOptionPane;

public class BoardManager implements Runnable {

    private Board board;

    public BoardManager(Board board) {
        this.board = board;
    }

    @Override
    public void run() {
        for (;;) {
            Socket socket = board.getSocket();
            try {
                InputStream socketInputStream = socket.getInputStream();
                byte[] response = socketInputStream.readNBytes(4);
                ByteBuffer buf = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).put(response);
                buf.rewind();
                int resp = buf.getInt();
                buf.rewind();
                byte[] arr;
                switch (resp) {
                    case 0:
                        arr = socketInputStream.readNBytes(4 * 64);
                        for (int li = 0; li < 8; li++) {
                            for (int lj = 0; lj < 8; lj++) {
                                byte[] t = new byte[4];
                                System.arraycopy(arr, (li * 8 + lj) * 4, t, 0, 4);
                                buf = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).put(t);
                                buf.rewind();
                                int val = buf.getInt();
                                buf.rewind();
                                switch (val) {
                                    case 0:
                                        board.buttons[li][lj].setOpaque(true);
                                        break;
                                    case 1:
                                        board.buttons[li][lj].setBackground(Color.BLACK);
                                        break;
                                    case 2:
                                        board.buttons[li][lj].setBackground(Color.WHITE);
                                        break;
                                }
                            }
                        }
                        board.getContentPane().setBackground(Color.RED);
                        break;
                    case 3:
                        JOptionPane.showMessageDialog(board, "It's your your turn.");
                        break;
                    case 4:
                        JOptionPane.showMessageDialog(board, "Game has not started yet.");
                        break;
                    case 5:
                        JOptionPane.showMessageDialog(board, "It's not a valid move.");
                        break;
                    case 6:
                        JOptionPane.showMessageDialog(board, "You cannot take a taken field.");
                        break;
                    case 7:
                        arr = socketInputStream.readNBytes(4 * 64);
                        for (int li = 0; li < 8; li++) {
                            for (int lj = 0; lj < 8; lj++) {
                                byte[] t = new byte[4];
                                System.arraycopy(arr, (li * 8 + lj) * 4, t, 0, 4);
                                buf = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).put(t);
                                buf.rewind();
                                int val = buf.getInt();
                                buf.rewind();
                                switch (val) {
                                    case 0:
                                        board.buttons[li][lj].setOpaque(true);
                                        break;
                                    case 1:
                                        board.buttons[li][lj].setBackground(Color.BLACK);
                                        break;
                                    case 2:
                                        board.buttons[li][lj].setBackground(Color.WHITE);
                                        break;
                                }
                            }
                        }
                        board.getContentPane().setBackground(Color.GREEN);
                        break;
                    case 8:
                        JOptionPane.showMessageDialog(board, "Player 1 won.");
                        board.dispatchEvent(new WindowEvent(board, WindowEvent.WINDOW_CLOSING));
                        break;
                    case 9:
                        JOptionPane.showMessageDialog(board, "Player 2 won.");
                        board.dispatchEvent(new WindowEvent(board, WindowEvent.WINDOW_CLOSING));
                        break;
                    case 10:
                        System.out.println("Received ping");
                        byte[] msg = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(11).array();
                        socket.getOutputStream().write(msg);
                        System.out.println("Sent ack");
                        break;
                    case 12:
                        JOptionPane.showMessageDialog(board, "Player 1 timeout.");
                        board.dispatchEvent(new WindowEvent(board, WindowEvent.WINDOW_CLOSING));
                        break;
                    case 13:
                        JOptionPane.showMessageDialog(board, "Player 2 timeout.");
                        board.dispatchEvent(new WindowEvent(board, WindowEvent.WINDOW_CLOSING));
                        break;
                    case 15:
                        JOptionPane.showMessageDialog(board, "Draw.");
                        board.dispatchEvent(new WindowEvent(board, WindowEvent.WINDOW_CLOSING));
                        break;
                    case 16:
                        board.getContentPane().setBackground(Color.GREEN);
                        break;
                    case 17:
                        board.getContentPane().setBackground(Color.RED);
                        break;
                }

            } catch (IOException err) {
                JOptionPane.showMessageDialog(null, err);
                break;
            }
        }
    }

}
