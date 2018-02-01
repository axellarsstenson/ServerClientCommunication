
#include "blather.h"

simpio_t simpio_actual;
simpio_t *simpio = &simpio_actual;

pthread_t user_thread;          			// thread managing user input
pthread_t background_thread;				// thread managing background processes

int join_fd = 0;
int to_client_fd = 0;
int to_server_fd = 0;
char name[MAXNAME] = {'\0'};


// Declare client specs as global variables so they can be used in signal handlers.
void handle_signals(int sig_num) {
	printf("Signal\n");													// @todo - make graceful shutdown
}


void *user_worker(void *arg) {
	simpio_noncanonical_terminal_mode(); 
	while(!simpio->end_of_input) {
		simpio_reset(simpio);
    	iprintf(simpio, "");                                          // print prompt
    while(!simpio->line_ready && !simpio->end_of_input) {          // read until line is complete
    	simpio_get_char(simpio);
    	}
    if(simpio->line_ready){
		mesg_t mesg;
		mesg.kind = BL_MESG;
		strcpy(mesg.name, name);
		strcpy(mesg.body, simpio->buf);
		write(to_server_fd, &mesg, sizeof(mesg_t));	
    	iprintf(simpio, "%s", simpio->buf);
    	}
  	}
	mesg_t mesg;
	mesg.kind = BL_DEPARTED;
	strcpy(mesg.name, name);
	write(to_server_fd, &mesg, sizeof(mesg_t));	
	pthread_cancel(background_thread); 						// kill the background thread
	return NULL;
	}  // user_worker()

void *background_worker(void *arg){
	int shutdown = 0;
	while (shutdown == 0){
		sleep(1);
		mesg_t read_mesg;
		read(to_client_fd, &read_mesg, sizeof(mesg_t));
				
		if (read_mesg.kind == BL_MESG) {
			iprintf(simpio, "[%s] : %s\n", read_mesg.name, read_mesg.body);
			}
		else if (read_mesg.kind == BL_JOINED) {
			iprintf(simpio, "-- %s JOINED --\n", read_mesg.name);
			}
		else if (read_mesg.kind == BL_DEPARTED) {
			iprintf(simpio, "-- %s DEPARTED --\n", read_mesg.name);
			}
		else if (read_mesg.kind == BL_DISCONNECTED) {
			iprintf(simpio, "-- %s DISCONNECTED --\n", read_mesg.name);
			}
		else if (read_mesg.kind == BL_PING) {
			mesg_t ping_mesg;
			ping_mesg.kind = BL_PING;
			write(to_server_fd, &ping_mesg, sizeof(mesg_t));	
			}
		else if (read_mesg.kind == BL_SHUTDOWN) {
			iprintf(simpio, "!!! server is shutting down !!!\n");
			shutdown = 1;
			}
		}  // end of while(...)
		
	pthread_cancel(user_thread); 
	return NULL;
	}  // background_worker()

int main(int argc, char *argv[]) {
	// Set up signal handlers
	setvbuf(stdout, NULL, _IONBF, 0); 
	struct sigaction my_sa = {};                                       // portable signal handling setup with sigaction()
	my_sa.sa_handler = handle_signals;                                 // run function handle_signals
	sigaction(SIGCONT, &my_sa, NULL);                                  // register SIGCONT with given action


	// Error handling for incorrect length of argv[]
	if(argc != 3) {
		printf("Client was not successful, please enter correct number of args.\n");
		exit(1);
	  }

	// Formatting fifo name for server/joining fifo
	char fifo_name[MAXPATH] = {'\0'};
	char fifo_extension[6] = ".fifo\0";
	strcpy(fifo_name, argv[1]);
	strcat(fifo_name, fifo_extension);
	
	// Formatting of name.
	strcpy(name, argv[2]);

	// Create fifo_name = .server.fifo
	char to_client_fifo_suffix[MAXPATH] = {'\0'};
	char to_server_fifo_suffix[MAXPATH] = {'\0'};
	strcat(to_client_fifo_suffix, ".client.fifo");
	strcat(to_server_fifo_suffix, ".server.fifo");

	// Formatting the names of client/server communication fifos, adding pid
	pid_t client_pid = getpid();
	char this_to_client_fifo[MAXPATH] = {'\0'};
	char this_to_server_fifo[MAXPATH] = {'\0'};
	char pid[MAXPATH];
	snprintf(pid, 10, "%d",(int)client_pid);
	strcat(this_to_client_fifo, pid); 
	strcat(this_to_server_fifo, pid);
	
	// Add .sever.fifo and .client.fifo to end
	strcat(this_to_client_fifo, to_client_fifo_suffix);
	strcat(this_to_server_fifo, to_server_fifo_suffix);

	// Make fifos
	mkfifo(this_to_client_fifo, S_IRUSR | S_IWUSR);
	mkfifo(this_to_server_fifo, S_IRUSR | S_IWUSR);
		
	// Open fifos
	join_fd 		= open(fifo_name, O_RDWR);
	to_client_fd    = open(this_to_client_fifo, O_RDWR);     	// open read/write in case server hasn't started  
	to_server_fd 	= open(this_to_server_fifo, O_RDWR);  		// open read or write only may cause hangs if the


	join_t join_mesg;
	
	strcpy(join_mesg.name, name);										// name of the client joining the server
	strcpy(join_mesg.to_client_fname, this_to_client_fifo); 			// name of file server writes to to send to client
	strcpy(join_mesg.to_server_fname, this_to_server_fifo);				// name of file client writes to to send to server

	// Write join_t to server FIFO
	write(join_fd, &join_mesg, sizeof(join_t));
	 
	// Setup for simpio
	char prompt[MAXNAME];
	snprintf(prompt, MAXNAME, "%s>> ", name); // create a prompt string
	simpio_set_prompt(simpio, prompt);         // set the prompt
	simpio_reset(simpio);                      // initialize io
										      // set the terminal into a compatible mode

	// Start threads for user and background.
	pthread_create(&background_thread, NULL, background_worker, NULL);	// start background thread
	pthread_create(&user_thread,   NULL, user_worker,   NULL);     		// start user thread to read input
	
	// Wait for threads to return
	pthread_join(user_thread, NULL);
	pthread_join(background_thread, NULL);
	
	// Restore standard input.
	simpio_reset_terminal_mode();
	printf("\n");                 // newline just to make returning to the terminal prettier

	return 1;
}
