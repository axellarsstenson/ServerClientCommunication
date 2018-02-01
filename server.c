
#include "blather.h"


client_t *server_get_client(server_t *server, int idx) {
	/*
	* @brief Gets a pointer to the client_t struct at the given index. If the
	* index is beyond n_clients, the behavior of the function is
	* unspecified and may cause a program crash.
	*/
	if (idx >= server->n_clients) {
		perror("The index specified is out of bound in server_get_client()");
		exit(1);
		}
	return &server->client[idx];
	}  // server_get_client()




void server_start(server_t *server, char *server_name, int perms) {
	/* 
	* @brief Initializes and starts the server with the given name. A join fifo
	* called "server_name.fifo" should be created. Removes any existing
	* file of that name prior to creation. Opens the FIFO and stores its
	* file descriptor in join_fd._
	*
	* ADVANCED: create the log file "server_name.log" and write the
	* initial empty who_t contents to its beginning. Ensure that the
	* log_fd is position for appending to the end of the file. Create the
	* POSIX semaphore "/server_name.sem" and initialize it to 1 to
	* control access to the who_t portion of the log.
  	*/
  	
  	setvbuf(stdout, NULL, _IONBF, 0); 
  	printf("Starting server %s\n", server_name);
  	
	// Prepare server fifo name and s
	char finalized_server_name[MAXPATH] = {'\0'};
	char log_name[MAXPATH] = {'\0'};
	char fifo_suffix[6] = ".fifo\0";
	char log_suffix[5] = ".dat\0";
	strncpy(log_name, server_name, (MAXNAME - 4));
	strncpy(finalized_server_name, server_name, (MAXNAME - 5));
	strcat(log_name, log_suffix);
	strcat(finalized_server_name, fifo_suffix);
	
	strncpy(server->server_name, finalized_server_name, MAXNAME);		// Put name in server struct
	
	// Open join fifo and check for errors
	int fiferr;
	if((fiferr = mkfifo(finalized_server_name, S_IRUSR | S_IWUSR)) < 0) {
		printf("error in mkfifo%i\n", fiferr);
		}
		
	// Program waits here for a Client to join.
	server->join_fd = open(server->server_name, perms);
	if(server->join_fd < 0){
		printf("Error on opening server fifo in server_start.\n");
		}
	server->join_ready = 0;												// Not ready yet
	server->n_clients = 0;												// No clients yet
	server->time_sec = 0;												// No time has elapsed
	
	// Open log file and check for errors
//	if ((server->log_fd = open(log_name, perms)) < 0){
//		printf("Error opening .dat file in server_start.\n");
//		}
	
	// @todo write empty client[] list to beginning of server.dat
	// @todo, make sure the sem is named correctly and log_fd is positioned correctly
	
	//	printf("before sem_init\n");
	//	sem_init(server->log_sem, 0, 1);
	//	printf("after sem_init\n");

	return;
	}  // server_start()




void server_shutdown(server_t *server) {
	/*
	* @brief Shut down the server. Close the join FIFO and unlink (remove) it so
	* that no further clients can join. Send a BL_SHUTDOWN message to all
	* clients and proceed to remove all clients in any order.
	*
	* ADVANCED: Close the log file. Close the log semaphore and unlink
	* it.
	*/
	printf("Server %s shutting down.\n", server->server_name);

	// Setup shutdown message and broadcast it		
	mesg_t shutdown_mesg;
	shutdown_mesg.kind = BL_SHUTDOWN;	
	server_broadcast(server, &shutdown_mesg);

	// Destroy server fifo and error handling
	if (remove(server->server_name) < 0){
		printf("Problem removing %s fifo in server_shutdown", server->server_name);
		}
		
	// @todo remove .log file	
		
	// Destroy server semaphore and error handling
	//	if (sem_destroy(server->log_sem) < 0){
	//		perror("Problem destroying POSIX semaphore in server_shutdown");
	//		}
	return;
	}  // server_shutdown()





int server_add_client(server_t *server, join_t *join) {
	/*
	* @brief Adds a client to the server according to the parameter join which
	* should have fileds such as name filed in.  The client data is
	* copied into the client[] array and file descriptors are opened for
	* its to-server and to-client FIFOs. Initializes the data_ready field
	* for the client to 0. Returns 0 on success and non-zero if the
	* server as no space for clients (n_clients == MAXCLIENTS).
	*/
	// Check if MAXCLIENTS has been reached
	if (server->n_clients == MAXCLIENTS) {
		return -1;
		}
		
	else {
		// Create client_t to add to client[]
		client_t add_client;
		// Add name
		strncpy(add_client.name, join->name, MAXNAME);
		// Add to_client name
		strncpy(add_client.to_client_fname, join->to_client_fname, MAXPATH);
		// Add to_server name
		strncpy(add_client.to_server_fname, join->to_server_fname, MAXPATH);
		// Data isn't ready yet
		add_client.data_ready = 0;
	
		add_client.last_contact_time = 0;			// @todo - implement this
	
		if ((add_client.to_client_fd = open(join->to_client_fname, O_RDWR)) < 0) {
			perror("Error on opening to_client_fname in server_add_client");
			return 1;
			}  
		if ((add_client.to_server_fd = open(join->to_server_fname, O_RDWR)) < 0) {
			perror("Error on opening to_server_fname in server_add_client");
			return 1;          
			} 		
			
		server->client[server->n_clients] = add_client;
		server->n_clients += 1;
		} // end else
		return 0;
	}  // server_add_client()





int server_remove_client(server_t *server, int idx) {
	/*
	* @brief Remove the given client likely due to its having departed or
	* disconnected. Close fifos associated with the client and remove
	* them.  Shift the remaining clients to lower indices of the client[]
	* array and decrease n_clients.
	*/
	// Save removed client for broadcast message
	client_t * removed_client = server_get_client(server, idx);
	
	printf("Client %s left the server.\n", removed_client->name);
	
	// Remove client communication fifos
	remove(removed_client->to_client_fname);
	remove(removed_client->to_server_fname);
  	printf("Removed fifos for %s\n", removed_client->name);
		
	// Remove client from client[]	
	for (int i = idx; i < server->n_clients; i++) {
		server->client[i] = server->client[i+1];
		}
	server->n_clients -= 1;
	
	// Prepare message for broadcasting that client[idx] departed
	mesg_t depart_mesg;
	depart_mesg.kind = BL_DEPARTED;
	strncpy(depart_mesg.name, removed_client->name, MAXNAME);
	server_broadcast(server, &depart_mesg);	
	return 0;
	}  // server_remove_client()




int server_broadcast(server_t *server, mesg_t *mesg) {
	/* 
	* @brief Send the given message to all clients connected to the server by
	* writing it to the file descriptors associated with them.
	*
	* ADVANCED: Log the broadcast message unless it is a PING which
	* should not be written to the log.
	*/
	mesg_t to_pass;
	to_pass.kind = mesg->kind;
	strcpy(to_pass.name, mesg->name);
	strcpy(to_pass.body, mesg->body);
	for (int i = 0; i < server->n_clients; i++) {
		write(server->client[i].to_client_fd, &to_pass, sizeof(mesg_t));
		}
		
	//if (mesg->kind != BL_PING) {
	//	write(server->log_fd, mesg, sizeof(mesg));
	//	}
	return 1;
	} // sever_broadcast()
	


void server_check_sources(server_t *server) {
	/*
	* @brief Checks all sources of data for the server to determine if any are
	* ready for reading. Sets the servers join_ready flag and the
	* data_ready flags of each client if data is ready for them.
	* Makes use of the select() system call to efficiently determine
	* which sources are ready.
	*/
	// Populate read_set
	fd_set read_set; 
	FD_ZERO(&read_set);
	int n_fds = 1;
	int max_fd = server->join_fd;
	FD_SET(server->join_fd, &read_set);
	for (int i = 0; i < server->n_clients; i++) {
		FD_SET(server->client[i].to_server_fd, &read_set);
		if (server->client[i].to_server_fd > max_fd){
			max_fd = server->client[i].to_server_fd;
			}
		n_fds++;
		}

	// Set up timeval
	struct timeval timev;
	timev.tv_sec = 1;
	timev.tv_usec = 0;
	
	if ((select(max_fd+1, &read_set, NULL, NULL, &timev)) < 0) {
		perror("Error in select: \n");
		}
	
	// if join is ready
	if (FD_ISSET(server->join_fd, &read_set))  {
		server->join_ready = 1;
		}
	
	// if any clients are ready
	for (int i = 0; i < server->n_clients; i++) {
		if (FD_ISSET(server->client[i].to_server_fd, &read_set)) {
			server->client[i].data_ready = 1;
			}  // end if(...)
		} // end for(...)	
	}  // server_check_sources()



int server_join_ready(server_t *server) {
	/* 
	* @brief Return the join_ready flag from the server which indicates whether
	* a call to server_handle_join() is safe.
	*/
	return server->join_ready;
	}  // server_join_ready()



int server_handle_join(server_t *server) {
	/*
	* @brief Call this function only if server_join_ready() returns true. Read a
	* join request and add the new client to the server. After finishing,
	* set the servers join_ready flag to 0.
	*/
	// Read from fifo to new_client
	join_t new_client;
	read(server->join_fd, &new_client, sizeof(join_t));
	
	// Call add_client and adjust join ready	
	server_add_client(server, &new_client);
	server->join_ready = 0;
	
	// Broadcast join to all clients
	mesg_t join_mesg;
	join_mesg.kind = BL_JOINED;
	strncpy(join_mesg.name, new_client.name, MAXNAME);
	server_broadcast(server, &join_mesg);
	return 0;
	}  // server_handle_join()

	
int server_client_ready(server_t *server, int idx) {
	/*
	* @brief Return the data_ready field of the given client which indicates
	* whether the client has data ready to be read from it.
	*/
	if (idx >= server->n_clients) {
		perror("Index passed to server_client_ready is out of bounds\n");
		}
	else {
		return server->client[idx].data_ready;
		}
	printf("reached end of server_client_ready in error\n");
	return 0;
	}  // server_client_ready()



int server_handle_client(server_t *server, int idx) {
	/*
	* @brief Process a message from the specified client. This function should
	* only be called if server_client_ready() returns true. Read a
	* message from to_server_fd and analyze the message kind. Departure
	* and Message types should be broadcast to all other clients.  Ping
	* responses should only change the last_contact_time below. Behavior
	* for other message types is not specified. Clear the client's
	* data_ready flag so it has value 0.
	*
	* ADVANCED: Update the last_contact_time of the client to the current
	* server time_sec.
	*/
	
	mesg_t new_mesg;
	read(server->client[idx].to_server_fd, &new_mesg, sizeof(mesg_t));

	if(new_mesg.kind == BL_MESG) {
		server_broadcast(server, &new_mesg);
		}
	
	else if(new_mesg.kind == BL_DEPARTED) {
		server_remove_client(server, idx);
		}
	
	else if(new_mesg.kind == BL_DISCONNECTED) {
		server_remove_client(server, idx);
		}
	
	else if(new_mesg.kind != BL_PING) {
		perror("Unknown message in server_handle_client\n");
		}
	
	// Update last_contact_time and data_ready flag
	server->client[idx].last_contact_time = server->time_sec; 
	server->client[idx].data_ready = 0;
	return 1;
	}  // server_handle_client()



void server_tick(server_t *server) {
	/*
	* @brief ADVANCED: Increment the time for the server
	*/
	server->time_sec += 1;
	}  // server_tick()



void server_ping_clients(server_t *server) {
	/*
	* @brief ADVANCED: Ping all clients in the server by broadcasting a ping.
	*/
	mesg_t ping_mesg;
	ping_mesg.kind = BL_PING;
	server_broadcast(server, &ping_mesg);
	}  // server_ping_clients()	



void server_remove_disconnected(server_t *server, int disconnect_secs) {
	/*
	* @brief ADVANCED: Check all clients to see if they have contacted the
	* server recently. Any client with a last_contact_time field equal to
	* or greater than the parameter disconnect_secs should be
	* removed. Broadcast that the client was disconnected to remaining
	* clients.  Process clients from lowest to highest and take care of
	* loop indexing as clients may be removed during the loop
	* necessitating index adjustments.
	*/
	int dyn_n_clients = server->n_clients;

	for (int i = 0; i < dyn_n_clients; i++) {
		if (server->client[i].last_contact_time >= disconnect_secs) {
			server_remove_client(server, i);
			dyn_n_clients -= 1;
			}
		}  // end for(...)
	}  // server_remove_disconnected()



void server_write_who(server_t *server) { return; }
	/*
	* ADVANCED: Write the current set of clients logged into the server
	* to the BEGINNING the log_fd. Ensure that the write is protected by
	* locking the semaphore associated with the log file. Since it may
	* take some time to complete this operation (acquire semaphore then
	* write) it should likely be done in its own thread to preven the
	* main server operations from stalling.  For threaded I/O, consider
	* using the pwrite() function to write to a specific location in an
	* open file descriptor which will not alter the position of log_fd so
	* that appends continue to write to the end of the file.
	*/ /*
	
	pthread_t who_t_thread;
    pthread_create(&who_t_thread, NULL, server_write_who_helper(server), NULL);
	pthread_join(&who_t_thread, NULL);
	}  // server_write_who()
*/

	

void* server_write_who_helper(server_t *server) { return NULL; }
/*
	who_t new_who_t;
	new_who_t.n_clients = server->n_clients;
	for(int i = 0; i < server->n_clients; i++) {
		strncpy(new_who_t.names[i], server->client[i]->name, MAXNAME);
		}

	if(sem_wait(server->log_sem) < 0) {
		perror("Error on sem_wait() in server_write_who_helper()\n");
		}
	pwrite(server->log_fd, new_who_t,  0, 0)
	if(sem_post(server->log_sem) < 0) {
		perror("Error on sem_post() in server_write_who_helper()\n");
		}
	}  // server_write_who_helper()
*/

void server_log_message(server_t *server, mesg_t *mesg) {
	/*
	* @brief ADVANCED: Write the given message to the end of log file associated
	* with the server.
	*/
	if (lseek(server->log_fd, 0, SEEK_END) < 0) {
		perror("Error on lseek in server_log_message()\n");
		}
	if (write(server->log_fd, mesg, sizeof(mesg))) {
		perror("Error on write in server_log_message()\n");
		}
	}  // server_log_message()
	











