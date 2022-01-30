/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/GUIForms/JFrame.java to edit this template
 */
package client;

import java.awt.Color;
import java.awt.event.ActionEvent;
import java.awt.event.WindowEvent;
import java.io.IOException;
import java.io.OutputStream;
import java.net.Socket;
import java.net.SocketException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JOptionPane;

/**
 *
 * @author adria
 */
public class Board extends javax.swing.JFrame {

    public class Button extends JButton{
        public Button(int i, int j){
            this.i = i;
            this.j = j;
        }
        public int i;
        public int j;
    }
    public final Button[][] buttons;
    private final Socket socket;
    private final Object lock = new Object();
    
    public Object getLock(){
        return lock;
    }

    public Board(Socket socket) {
        initComponents();
        this.socket = socket;
        buttons = new Button[8][8];
        for (int i = 0; i < 8; i++){
            for (int j = 0; j < 8; j++){
                final int ti = i;
                final int tj = j;
                buttons[i][j] = new Button(i, j);
                buttons[i][j].setBounds(i * 60, j * 60, 55, 55);
                final JFrame tmp = this;
                buttons[i][j].addActionListener((ActionEvent e) -> {
                    synchronized(lock){
                        try {
                            OutputStream socketOutputStream = socket.getOutputStream();
                            byte[] msg = new byte[12];
                            byte[] tmp1 = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(14).array();
                            System.arraycopy(tmp1, 0, msg, 0, 4);
                            tmp1 = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(ti).array();
                            System.arraycopy(tmp1, 0, msg, 4, 4);
                            tmp1 = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(tj).array();
                            System.arraycopy(tmp1, 0, msg, 8, 4);
                            socketOutputStream.write(msg);
                        }catch (NullPointerException err){
                            JOptionPane.showMessageDialog(null, err);
                        }catch (SocketException err){
                            JOptionPane.showMessageDialog(null, "Disconnected from server.");
                            dispatchEvent(new WindowEvent(tmp, WindowEvent.WINDOW_CLOSING));
                        }catch (IOException err){
                            JOptionPane.showMessageDialog(null, err);
                        }
                    }});
                this.add(buttons[i][j]);
            }
        }
        buttons[3][3].setBackground(Color.black);
        buttons[4][4].setBackground(Color.black);
        buttons[3][4].setBackground(Color.white);
        buttons[4][3].setBackground(Color.white);
    }
    
    public Socket getSocket(){
        return socket;
    }

    /**
     * This method is called from within the constructor to initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is always
     * regenerated by the Form Editor.
     */
    @SuppressWarnings("unchecked")
    // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
    private void initComponents() {

        setDefaultCloseOperation(javax.swing.WindowConstants.EXIT_ON_CLOSE);

        javax.swing.GroupLayout layout = new javax.swing.GroupLayout(getContentPane());
        getContentPane().setLayout(layout);
        layout.setHorizontalGroup(
            layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGap(0, 480, Short.MAX_VALUE)
        );
        layout.setVerticalGroup(
            layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGap(0, 480, Short.MAX_VALUE)
        );

        pack();
    }// </editor-fold>//GEN-END:initComponents

    // Variables declaration - do not modify//GEN-BEGIN:variables
    // End of variables declaration//GEN-END:variables
}
