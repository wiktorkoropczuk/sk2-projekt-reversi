package client;

import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import javax.swing.JFrame;

public class Client{
    public static final Object lock = new Object();
    
    public static void main(String[] args){
        
        ConnectionWindow window = new ConnectionWindow();
        window.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
        window.addWindowListener(new WindowAdapter(){
            @Override
            public void windowClosing(WindowEvent ev){
                synchronized(lock){
                    window.setVisible(false);
                    lock.notify();
                }
            }
        });
        window.setVisible(true);
        while (window.isVisible()){
            synchronized(lock){
                try{
                    lock.wait();
                }
                catch (InterruptedException e){
                    
                }
            }
        }
        Thread thread = null;
        if (window.getSocket() != null){
            Board board = new Board(window.getSocket());
            board.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
            board.setVisible(true);
            thread = new Thread(new BoardManager(board));
            thread.start();
        }
        try{
            thread.join();
        }catch (InterruptedException | NullPointerException err){
            
        }
        
    }

}
