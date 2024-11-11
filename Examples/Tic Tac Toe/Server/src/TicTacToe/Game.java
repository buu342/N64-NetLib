package TicTacToe;

import NetLib.NetLibPacket;
import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;

public class Game implements Runnable  {
    
    private GameState state = GameState.GAMESTATE_LOBBY;
    private Player players[];
    private BoardLarge board;
    private Player turn;
    private Queue<NetLibPacket> messages;
    
    public Game() {
    	this.messages = new ConcurrentLinkedQueue<NetLibPacket>();
        this.players = new Player[2];
        this.turn = null;
        System.out.println("Tic Tac Toe initialized");
    }
    
    public void run() {
        try {
            Thread.currentThread().setName("Game");
            
            while (true) {
                boolean player1_ready = false;
                boolean player2_ready = false;
                this.ChangeGameState(GameState.GAMESTATE_LOBBY);
                
                // Wait for the room to be full and ready
                System.out.println("Waiting for players to be ready");
                while (!player1_ready || !player2_ready) {
                    NetLibPacket pkt;
                    
                    // Ensure we have 2 players
                    if (this.PlayerCount() < 2) {
                        player1_ready = false;
                        player2_ready = false;
                        Thread.sleep(1000);
                        continue;
                    }
                    
                    // Check we have messages
                    pkt = messages.poll();
                    if (pkt == null) {
                        Thread.sleep(10);
                        continue;
                    }
                    
                    // Check everyone is ready
                    if (pkt.GetID() == PacketIDs.PACKETID_PLAYERREADY.GetInt()) {
                        if (pkt.GetSender() == 1) {
                            player1_ready = (pkt.GetData()[0] == 1);
                            this.NotifyReady(this.players[1], this.players[0], player1_ready);
                        } else {
                            player2_ready = (pkt.GetData()[0] == 1);
                            this.NotifyReady(this.players[0], this.players[1], player2_ready);
                        }
                    }
                }
                
                // Ready to start
                System.out.println("Everyone ready, game starting");
                this.ChangeGameState(GameState.GAMESTATE_READY);
                Thread.sleep(1000);
                
                // Initialize the game
                this.board = new BoardLarge();
                
                // Game loop
                System.out.println("New game started");
                this.ChangeGameState(GameState.GAMESTATE_PLAYING);
                                
                // If no previous player turn was chosen, then give the first player the first turn
                if (this.turn == null)
                    this.turn = this.players[0];
                this.ChangePlayerTurn(this.turn);
                System.out.println(this.board);
                System.out.println("Player " + turn.GetNumber() + "'s turn");
                System.out.println("");
                
                while (this.state == GameState.GAMESTATE_PLAYING) {
                    int boardx, boardy, movex, movey;
                    
                    // Receive packets from the player who's turn it is to play
                    NetLibPacket pkt = messages.poll();
                    if (pkt == null) {
                        Thread.sleep(10);
                        continue;
                    }
                    
                    // Validate the packet
                    if (pkt == null || pkt.GetID() != PacketIDs.PACKETID_PLAYERMOVE.GetInt() || pkt.GetSender() != turn.GetNumber() || pkt.GetSize() < 4)
                        continue;
                    
                    // Make the move
                    boardx = pkt.GetData()[0];
                    boardy = pkt.GetData()[1];
                    movex = pkt.GetData()[2];
                    movey = pkt.GetData()[3];
                    if (!this.board.MakeMove(turn, boardx, boardy, movex, movey))
                        continue; // If the move wasn't valid, restart the loop and try again
                    
                    // Valid move, notify other players of the move
                    this.NotifyMove(turn, 1 + boardx + boardy*3, movex, movey);
                    
                    // Notify if the board was completed
                    if (this.board.GetBoard(boardx, boardy).BoardFinished())
                    {
                        BoardSmall bs = this.board.GetBoard(boardx, boardy);
                        if (bs.GetWinner() == null)
                            this.NotifyBoardComplete(1 + boardx + boardy*3, 3); // 3 means tie
                        else
                            this.NotifyBoardComplete(1 + boardx + boardy*3, bs.GetWinner().GetNumber()); // Otherwise send the player number
                    }
                    
                    // Change player turn
                    if (this.turn == this.players[0])
                        this.ChangePlayerTurn(this.players[1]);
                    else
                        this.ChangePlayerTurn(this.players[0]);
                    
                    // Check for winners
                    if (this.board.GetWinner() != null || this.board.BoardFinished())
                        break;
                    
                    // Print the next player's turn
                    System.out.println(this.board);
                    System.out.println("Player " + turn.GetNumber() + "'s turn");
                    if (this.board.GetForcedBoardNumber() != 0)
                        System.out.println("Forced to play on board " + this.board.GetForcedBoardNumber());
                    System.out.println("");
                }
                
                // Game ended. Show the winner for 5 seconds
                System.out.println(this.board);
                System.out.println("Game Ended.");
                if (this.board.GetWinner() != null) {
                    if (this.board.GetWinner() == this.players[0])
                        this.ChangeGameState(GameState.GAMESTATE_ENDED_WINNER_1);
                    else
                        this.ChangeGameState(GameState.GAMESTATE_ENDED_WINNER_2);
                    System.out.println("Player " + board.GetWinner().GetNumber() + " wins!");
                } else if (this.PlayerCount() < 2) {
                    this.ChangeGameState(GameState.GAMESTATE_ENDED_DISCONNECT);
                    System.out.println("Player disconnected!");
                } else {
                    this.ChangeGameState(GameState.GAMESTATE_ENDED_TIE);
                    System.out.println("Tie game!");
                }
                Thread.sleep(5000);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    public synchronized Player ConnectPlayer() {
        boolean foundslot = false;
        Player ply = new Player();
        for (int i=0; i<this.players.length; i++) {
            if (this.players[i] == null) {
                this.players[i] = ply;
                ply.SetNumber(i+1);
                foundslot = true;
                break;
            }
        }
        if (foundslot)
            return ply;
        return null;
    }
    
    public void SendMessage(Player sender, NetLibPacket pkt) {
        pkt.SetSender(sender.GetNumber());
    	this.messages.add(pkt);
    	System.out.println(pkt);
    }
    
    public int PlayerCount() {
        int count = 0;
        for (int i=0; i<this.players.length; i++)
            if (this.players[i] != null)
                count++;
        return count;
    }
    
    public Player[] GetPlayers() {
    	return this.players;
    }
    
    public synchronized void DisconnectPlayer(Player ply) {
        for (int i=0; i<this.players.length; i++) {
            if (this.players[i] == ply) {
                this.players[i] = null;
                if (this.state == GameState.GAMESTATE_PLAYING)
                    this.state = GameState.GAMESTATE_ENDED_DISCONNECT;
                return;
            }
        }
    }
    
    private void NotifyReady(Player target, Player who, boolean ready) {
        NetLibPacket pkt = new NetLibPacket(PacketIDs.PACKETID_PLAYERREADY.GetInt(), new byte[]{
            (byte)who.GetNumber(), 
            (byte) (ready ? 1 : 0)
        });
        target.SendMessage(null, pkt);
        System.out.println("Player " + who.GetNumber() + " ready: " + ready);
    }
    
    private void ChangeGameState(GameState state) {
        this.state = state;
        NetLibPacket pkt = new NetLibPacket(PacketIDs.PACKETID_GAMESTATECHANGE.GetInt(), new byte[]{
            (byte)state.GetInt()
        });
        for (Player ply : this.players)
            if (ply != null)
                ply.SendMessage(null, pkt);
    }
    
    private void NotifyMove(Player who, int boardnum, int movex, int movey) {
        NetLibPacket pkt = new NetLibPacket(PacketIDs.PACKETID_PLAYERMOVE.GetInt(), new byte[]{
            (byte)who.GetNumber(),
            (byte)boardnum,
            (byte)movex,
            (byte)movey,
        });
        for (Player ply : this.players)
            if (ply != null && ply != who)
                ply.SendMessage(null, pkt);
    }
    
    private void ChangePlayerTurn(Player who) {
        NetLibPacket pkt = new NetLibPacket(PacketIDs.PACKETID_PLAYERTURN.GetInt(), new byte[]{
            (byte)who.GetNumber(),
            (byte)this.board.GetForcedBoardNumber(),
        });
        for (Player ply : this.players)
            if (ply != null)
                ply.SendMessage(null, pkt);
        this.turn = who;
    }
    
    private void NotifyBoardComplete(int boardnum, int state) {
        NetLibPacket pkt = new NetLibPacket(PacketIDs.PACKETID_BOARDCOMPLETED.GetInt(), new byte[]{
            (byte)boardnum,
            (byte)state,
        });
        for (Player ply : this.players)
            if (ply != null)
                ply.SendMessage(null, pkt);
    }
}