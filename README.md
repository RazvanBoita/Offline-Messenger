Offline Messenger application made in C, that supports multiple clients connected to a server(using TCP and select for I/O multiplexing), who can perform a set of operations like:
  ->login/signup
  ->send a MESSAGE
  ->reply to MESSAGE
  ->history with USER 
  ->whoson/whosoff/whoall //see who is connected/offline/everyone
  ->unread //check messages you have received when being offline
  ->poke someone
  ->block/unblock
  ->quit/disconnect/clear screen
  
Used technologies:
  ->C language
  ->TCP(I/O multiplexing with select() )
  ->Sqlite3 for DATABASE (for storing messages, extra information about users)

Guide for testing the application:
  1. Fork the entire repo
  2. Make sure you have the Makefile copied, enter the terminal and hit 'make'
  3. You should get 2 executables, server and client, run server in an open terminal like this: ./server 0 2728
  4. Open as many terminals as you'd like and run clients like this: ./client 0 2728
