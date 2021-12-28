package klient;

import javax.swing.JFrame;

public class Klient {
    public static void main(String[] args){
        /*ConnectionWindow window = new ConnectionWindow();
        window.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
        window.setVisible(true);
        window.getSocket();*/
        Board board = new Board();
        board.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
        board.setVisible(true);
    }
}
