/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/GUIForms/JFrame.java to edit this template
 */
package klient;

import java.awt.Color;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.math.BigInteger;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import javax.swing.JButton;
import javax.swing.JOptionPane;

/**
 *
 * @author adria
 */
public class Board extends javax.swing.JFrame {

    private class Button extends JButton{
        public Button(int i, int j){
            this.i = i;
            this.j = j;
        }
        public int i;
        public int j;
    }
    private final Button[][] buttons;
    private final Socket socket;

    public Board(Socket socket) {
        initComponents();
        this.socket = socket;
        buttons = new Button[8][8];
        for (int i = 0; i < 8; i++){
            for (int j = 0; j < 8; j++){
                final int ti = i;
                final int tj = j;
                buttons[i][j] = new Button(i, j);
                buttons[i][j].setBounds(i * 60, j * 60, 60, 60);
                buttons[i][j].addActionListener(new ActionListener(){
                    @Override
                    public void actionPerformed(ActionEvent e){
                        //TODO: Send clicked button coordinates
                        try
                        {
                            System.out.print(ti);
                            System.out.println(tj);
                            OutputStream socketOutputStream = socket.getOutputStream();
                            byte[] msg = new byte[8];
                            byte[] tmp = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(ti).array();
                            System.arraycopy(tmp, 0, msg, 0, 4);
                            tmp = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(tj).array();
                            System.arraycopy(tmp, 0, msg, 4, 4);
                            socketOutputStream.write(msg);
                            InputStream socketInputStream = socket.getInputStream();
                            byte[] response = socketInputStream.readNBytes(4);
                            ByteBuffer buf = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).put(response);
                            buf.rewind();
                            int resp = buf.getInt();
                            buf.rewind();
                            switch (resp){
                                case 0:
                                    byte[] arr = socketInputStream.readNBytes(4 * 64);
                                    for (int li = 0; li < 8; li++){
                                        for (int lj = 0; lj < 8; lj++){
                                            byte[] t = new byte[4];
                                            System.arraycopy(arr, (li * 8 + lj) * 4, t, 0, 4);
                                            buf = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).put(t);
                                            buf.rewind();
                                            int val = buf.getInt();
                                            buf.rewind();
                                            System.out.print(val);
                                            System.out.print(" ");
                                            switch (val){
                                                case 0:
                                                    buttons[lj][li].setBackground(Color.BLUE);
                                                    break;
                                                case 1:
                                                    buttons[lj][li].setBackground(Color.BLACK);
                                                    break;
                                                case 2:
                                                    buttons[lj][li].setBackground(Color.WHITE);
                                                    break;
                                            }
                                        }
                                        System.out.println("");
                                    }
                                    break;
                            }
                        }
                        catch (IOException err)
                        {
                            JOptionPane.showMessageDialog(null, err);
                        }
                        catch (NullPointerException err)
                        {
                            JOptionPane.showMessageDialog(null, err);
                        }
                    }
                });
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

        jLabel1 = new javax.swing.JLabel();

        setDefaultCloseOperation(javax.swing.WindowConstants.EXIT_ON_CLOSE);

        jLabel1.setText("Gra");

        javax.swing.GroupLayout layout = new javax.swing.GroupLayout(getContentPane());
        getContentPane().setLayout(layout);
        layout.setHorizontalGroup(
            layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGroup(javax.swing.GroupLayout.Alignment.TRAILING, layout.createSequentialGroup()
                .addContainerGap(745, Short.MAX_VALUE)
                .addComponent(jLabel1)
                .addGap(38, 38, 38))
        );
        layout.setVerticalGroup(
            layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGroup(layout.createSequentialGroup()
                .addContainerGap()
                .addComponent(jLabel1)
                .addContainerGap(475, Short.MAX_VALUE))
        );

        pack();
    }// </editor-fold>//GEN-END:initComponents

    // Variables declaration - do not modify//GEN-BEGIN:variables
    private javax.swing.JLabel jLabel1;
    // End of variables declaration//GEN-END:variables
}
