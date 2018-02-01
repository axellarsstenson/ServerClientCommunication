
#include "blather.h"

/* 
* @brief main server functionality, most of calls are implemented in server.c
* 
* REPEAT:
* check all sources
* handle a join request if one is ready
* for each client
* if the client is ready handle data from it
* 
*/


// Declare as global so can use in signal handlers
server_t server;

// Signal handler for graceful shutdown and alarm tick
void handle_signals(int sig_num) {
	if (sig_num == SIGALRM) {
		server_tick(&server);
		printf("TESTING - server is ticking\n");
		//alarm(1);
		}
	else if (sig_num == SIGTERM || sig_num == SIGINT) {	
		server_shutdown(&server);
		printf("--- Server shutting down ---\n");
		}
	}  // handle_signals()

int main(int argc, char *argv[]) {

	// Check to make sure the correct number of args were passed.
	if (argc != 2) {
		printf("Wrong number of args passed");
		return 1;
		}
	
	// Set up signal handling for TERM and INT
	struct sigaction my_sa = {};										// portable signal handling setup with sigaction()
	my_sa.sa_handler = handle_signals;                                 	// run function handle_signals
	sigaction(SIGTERM, &my_sa, 0);                                  	// register SIGTERM with given action
	sigaction(SIGINT, &my_sa, 0);                	                  	// register SIGTERM with given action

	// Registering signal handler and calling alarm()
	sigaction(SIGALRM, &my_sa, 0);
	//alarm(1);

	// Start server
	server_start(&server, argv[1], O_RDWR);

	int i = 0;
	while (1) {
		
		// Check and update flags
		server_check_sources(&server);
		
		// Check and handle joins
		server_join_ready(&server);
		if(server.join_ready != 0) {
			server_handle_join(&server);
			}
			
		i += 1;
		
		// Check and handle client messages	
		for (int i = 0; i < server.n_clients; i++) {
			int ready_flag = server_client_ready(&server, i);
			if(ready_flag != 0) {
				server_handle_client(&server, i);
				}
			}  // end for(...)
		}  // end while(...)
	}  // end main()