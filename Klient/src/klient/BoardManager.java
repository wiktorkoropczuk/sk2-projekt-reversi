/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/Classes/Class.java to edit this template
 */
package klient;

import java.io.IOException;
import java.io.InputStream;
import java.net.Socket;
import java.util.Arrays;
import javax.swing.JOptionPane;

public class BoardManager implements Runnable{

    private Board board;
    
    public BoardManager(Board board){
        this.board = board;
    }
    
    @Override
    public void run() {
        for (;;){
            Socket socket = board.getSocket();
            try
            {
                InputStream socketInputStream = socket.getInputStream();
                byte[] r = socketInputStream.readNBytes(4);
                Integer val = new Integer(Arrays.toString(r));
                switch (val)
                {
                    case 0:
                        //Game stopped
                        break;
                    case 1:
                        //Read button and its new value
                        break;
                }
            }
            catch (IOException err)
            {
                JOptionPane.showMessageDialog(null, err);
            }
        }
    }
    
}
