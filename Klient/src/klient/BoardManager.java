/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/Classes/Class.java to edit this template
 */
package klient;

import java.awt.Color;
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
                OutputStream socketOutputStream = socket.getOutputStream();
                InputStream socketInputStream = socket.getInputStream();
                byte[] response = socketInputStream.readNBytes(4);
                ByteBuffer buf = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).put(response);
                buf.rewind();
                int resp = buf.getInt();
                buf.rewind();
                switch (resp) {
                    case 0:
                        byte[] arr = socketInputStream.readNBytes(4 * 64);
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
                        break;
                    case 3:
                        JOptionPane.showMessageDialog(null, "It' your your turn.");
                        break;
                    case 4:
                        JOptionPane.showMessageDialog(null, "Game has not started yet.");
                        break;
                    case 5:
                        JOptionPane.showMessageDialog(null, "It's not a valid move.");
                        break;
                    case 6:
                        JOptionPane.showMessageDialog(null, "You cannot take a taken field.");
                        break;
                    case 8:
                        JOptionPane.showMessageDialog(null, "Player 1 won.");
                        break;
                    case 9:
                        JOptionPane.showMessageDialog(null, "Player 2 won.");
                        break;
                }

            } catch (IOException err) {
                JOptionPane.showMessageDialog(null, err);
                break;
            }
        }
    }

}
