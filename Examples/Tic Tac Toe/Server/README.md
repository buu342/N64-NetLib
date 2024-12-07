# Ultimate Tic Tac Toe - Server

This folder contains the server implementation of the Ultimate TicTacToe game. A basic fake client is also provided for debugging in place of using actual N64s. 

### Compiling the Server
Open the project in [Eclipse](https://www.eclipse.org).

### Using the fake client
The fake client makes the assumption that the server is available at `127.0.0.1:6460`. You can change this by modifying the `HOST` and `PORT` constants.

This client works by you manually inputting what packets you want to send. Simply input the packet type (as a number), and then the raw bytes you want to send (separated by spaces).