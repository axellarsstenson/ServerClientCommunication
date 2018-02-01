# ServerClientCommunication

This program is a simulation of server client communication done locally. 

1) A makefile is included that will make the two main executable to run the program.

2) First a user must open terminal and launch server exe ./bl-server X where X will be the name of the server.

3) Then they may open a number of terminals and launch client exe ./bl-client X Y, where X is the name of the server in previous step
and Y can be any name, representing the name of the client.

4) The communication is done across FIFOs, which can be removed with make clean command

