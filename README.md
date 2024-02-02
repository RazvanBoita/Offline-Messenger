Offline Messenger application made in C, that supports multiple clients connected to a server(using TCP and select for I/O multiplexing), who can perform a set of operations like:<br />
  ->login/signup<br />
  ->send a MESSAGE<br />
  ->reply to MESSAGE<br />
  ->history with USER <br />
  ->whoson/whosoff/whoall //see who is connected/offline/everyone<br />
  ->unread //check messages you have received when being offline<br />
  ->poke someone<br />
  ->block/unblock<br />
  ->quit/disconnect/clear screen<br />
  
Used technologies:
  ->C language<br />
  ->TCP(I/O multiplexing with select() )<br />
  ->Sqlite3 for DATABASE (for storing messages, extra information about users)<br />
<br />
Guide for testing the application:
  1. Fork the entire repo<br />
  2. Make sure you have the Makefile copied, enter the terminal and hit 'make'<br />
  3. You should get 2 executables, server and client, run server in an open terminal like this: ./server 0 2728<br />
  4. Open as many terminals as you'd like and run clients like this: ./client 0 2728<br />
