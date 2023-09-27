#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "chatroom.h"
#include <poll.h>


//*******************
#include <pthread.h>
//*******************

#define MAX 1024 // max buffer size
#define PORT 6789 // server port number
#define MAX_USERS 50 // max number of users
static unsigned int users_count = 0; // number of registered users

static user_info_t *listOfUsers[MAX_USERS] = {0}; // list of users

//*******************
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
//*******************


/* Add user to userList */
void user_add(user_info_t *user);
/* Get user name from userList */
char * get_username(int sockfd);
/* Get user sockfd by name */
int get_sockfd(char *name);

/* Add user to userList */
void user_add(user_info_t *user){

	pthread_mutex_lock(&clients_mutex); //*******************

	if(users_count ==  MAX_USERS){
		printf("sorry the system is full, please try again later\n");
		return;
	}
	/***************************/
	/* add the user to the list */
	/**************************/
	else
	{
		listOfUsers[users_count] = user;
		users_count++;
	}

	pthread_mutex_unlock(&clients_mutex); //*******************

}

/* Determine whether the user has been registered  */
int isNewUser(char* name) {
	int i;
	int flag = -1;
	/*******************************************/
	/* Compare the name with existing usernames */
	/*******************************************/
	for (int i = 0; i < users_count; i++)
	{
		if (strcmp(name, listOfUsers[i]->username) == 0)
		{
			// Already Registered
			flag = 0;
		}
	}

	return flag; // -1 if it is a new user
}

/* Get user name from userList */
char * get_username(int ss){
	int i;
	static char uname[C_NAME_LEN];
	/*******************************************/
	/* Get the user name by the user's sock fd */
	/*******************************************/
	for(int i = 0; i < users_count; i++)
	{
		if (ss == listOfUsers[i]->sockfd)
		{
			strcpy(uname, listOfUsers[i]->username);
		}
	}
	
	// printf("get user name: %s\n", uname);
	return uname;
}

/* Get user sockfd by name */
int get_sockfd(char *name){
	int i;
	int sock = -1;
	/*******************************************/
	/* Get the user sockfd by the user name */
	/*******************************************/
	for(int i = 0; i < users_count; i++)
	{
		if (strcmp(name, listOfUsers[i]->username) == 0)
		{
			sock = listOfUsers[i]->sockfd;
		}
	}

	return sock;
}

// The following two functions are defined for poll()
// Add a new file descriptor to the set
void add_to_pfds(struct pollfd* pfds[], int newfd, int* fd_count, int* fd_size)
{
	// If we don't have room, add more space in the pfds array
	if (*fd_count == *fd_size) {
		*fd_size *= 2; // Double it

		*pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
	}

	(*pfds)[*fd_count].fd = newfd;
	(*pfds)[*fd_count].events = POLLIN; // Check ready-to-read

	(*fd_count)++;
}
// Remove an index from the set
void del_from_pfds(struct pollfd pfds[], int i, int* fd_count)
{
	// Copy the one from the end over this one
	pfds[i] = pfds[*fd_count - 1];

	(*fd_count)--;
}


//HELPER FUNCTION 
void strcat_c (char *str, char c)
  {
    for (;*str;str++); // note the terminating semicolon here. 
    *str++ = c; 
    *str++ = 0;
  }



int main()
{
	int listener;     // listening socket descriptor
	int newfd;        // newly accept()ed socket descriptor
	int addr_size;     // length of client addr
	struct sockaddr_in server_addr, client_addr;
	
	char buffer[MAX]; // buffer for client data
	int nbytes;
	int fd_count = 0;
	int fd_size = 5;
	struct pollfd* pfds = malloc(sizeof * pfds * fd_size);
	
	int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, u, rv;

    
	/**********************************************************/
	/*create the listener socket and bind it with server_addr*/
	/**********************************************************/
	listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener == -1) {
		printf("Socket creation failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully created..\n");
	
	bzero(&server_addr, sizeof(server_addr));
	
	// asign IP, PORT
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);
	
	// Binding newly created socket to given IP and verification
	if ((bind(listener, (struct sockaddr*)&server_addr, sizeof(server_addr))) != 0) {
		printf("Socket bind failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully binded..\n");

	// Now server is ready to listen and verification
	if ((listen(listener, 5)) != 0) {
		printf("Listen failed...\n");
		exit(3);
	}
	else
		printf("Server listening..\n");
		
	// Add the listener to set
	pfds[0].fd = listener;
	pfds[0].events = POLLIN; // Report ready to read on incoming connection
	fd_count = 1; // For the listener
	
	// main loop
	for(;;) {
		/***************************************/
		/* use poll function */
		/**************************************/
		int poll_count = poll(pfds, fd_count, -1);
		if (poll_count == -1)
		{
			perror("poll");
			exit(1);
		}

		// run through the existing connections looking for data to read
        	for(i = 0; i < fd_count; i++) 
			{
            	  if (pfds[i].revents & POLLIN) { // we got one!!
                    if (pfds[i].fd == listener) 
					{
                      /**************************/
					  /* we are the listener and we need to handle new connections from clients */
					  /****************************/
						addr_size = sizeof(client_addr);
						newfd = accept(listener, (struct sockaddr*)& client_addr, &addr_size);
						printf("DEBUG - newfd : %d\n", newfd);
						// send welcome message
						if (newfd == -1)
						{
							perror("accept");
						}
						else
						{
							add_to_pfds(&pfds, newfd, &fd_count, &fd_size);
							printf("pollserver: new connection from %s on socket %d\n",inet_ntoa(client_addr.sin_addr),newfd);
						}
						bzero(buffer, sizeof(buffer));
						strcpy(buffer, "Welcome to the chat room!\nPlease enter a nickname.\n");

						if (send(newfd, buffer, sizeof(buffer), 0) == -1)
						{
							perror("send");
						}
					}
                    else //Not the listener, just a regular client
					{ 
                        // handle data from a client
						bzero(buffer, sizeof(buffer));
                        if ((nbytes = recv(pfds[i].fd, buffer, sizeof(buffer), 0)) <= 0) {
                          // got error or connection closed by client
                          if (nbytes == 0) {
                            // connection closed
                            printf("pollserver: socket %d hung up\n", pfds[i].fd);
                          } else {
                            perror("recv");
                          }
						  close(pfds[i].fd); // Bye!
						  del_from_pfds(pfds, i, &fd_count);
                        } 
						else 
						{
                            // we got some data from a client
							if (strncmp(buffer, "REGISTER", 8)==0){
								printf("Got register/login message\n");
								/********************************/
								/* Get the user name and add the user to the userlist*/
								/**********************************/

								char name[C_NAME_LEN];
								strncpy(name, &buffer[8], C_NAME_LEN);
								name[strcspn(name, "\n")] = 0;
								
								printf("DEBUG Name : %s\n", name);


								if (isNewUser(name) == -1) {
									/********************************/
									/* it is a new user and we need to handle the registration*/
									/**********************************/
									printf("DEBUG - This is a new user\n");
									

									user_info_t* newUser = malloc(sizeof(user_info_t));
									newUser->sockfd = newfd;
									printf("%d\n", newUser->sockfd);
									strcpy(newUser->username, name);
									newUser->state = 1;
									user_add(newUser);

									/********************************/
									/* create message box (e.g., a text file) for the new user */
									/**********************************/
									char filename[C_NAME_LEN];
									strcpy(filename, name);
									FILE* fp;
									strcat(filename, ".txt");
									fp = fopen(filename, "a");
									fclose(fp);

									// broadcast the welcome message (send to everyone except the listener)
									bzero(buffer, sizeof(buffer));
									strcpy(buffer, "Welcome ");
									strcat(buffer, name);
									strcat(buffer, " to join the chat room!\n");

									/*****************************/
									/* Broadcast the welcome message*/
									/*****************************/
									
									pthread_mutex_lock(&clients_mutex);
									for (int i = 0; i < MAX_USERS; i++)
									{
										if (listOfUsers[i])
										{
											
											if (listOfUsers[i]->sockfd == listener)
											{
												continue;
											}
											printf("DEBUG - Sockfd : %d\n" ,listOfUsers[i]->sockfd);
											if (listOfUsers[i]->state == 1)
											{
												send(listOfUsers[i]->sockfd, buffer, strlen(buffer), 0);
											}
										}
									}
									pthread_mutex_unlock(&clients_mutex);

									/*****************************/
									/* send registration success message to the new user*/
									/*****************************/
									bzero(buffer, sizeof(buffer));
									strcpy(buffer, "A new account has been created\n");
									send(newUser->sockfd, buffer, strlen(buffer), 0);
									printf("DEBUG - Registration Message Sent\n");

								}
								else {
									/********************************/
									/* it's an existing user and we need to handle the login. Note the state of user,*/
									/**********************************/
									printf("DEBUG - This is an existing user\n");

									// int flag = 0;

									for(int i = 0; i < users_count; i++)
									{
										if (strcmp(listOfUsers[i]->username, name) == 0)
										{
											printf("DEBUG - Login Successful\n");
											printf("DEBUG - Changed state to online\n");
											listOfUsers[i]->state = 1;
											listOfUsers[i]->sockfd = newfd;
										}
									}

									/********************************/
									/* send the offline messages to the user and empty the message box*/
									/**********************************/
									printf("DEBUG - Reached here\n");
									printf("DEBUG - name %s\n", name);
									char filename[C_NAME_LEN];
									strcpy(filename, name);
									strcat(filename, ".txt");
									printf("DEBUG - filename %s\n", filename);
									
									FILE* fp;
									fp = fopen(filename, "r"); //reset the pointer
																	
									char c;
									bzero(buffer, sizeof(buffer));
									strcpy(buffer, "Welcome back! The message box contains:\n");

									while ((c = getc(fp)) != EOF) 
									{
										strcat_c(buffer, c);
									}
									fclose(fp);
									printf("DEBUG - buffer : %s", buffer);
									fclose(fopen(filename, "w"));

									send(newfd, buffer, strlen(buffer), 0);

									// broadcast the welcome message (send to everyone except the listener)
									bzero(buffer, sizeof(buffer));
									strcat(buffer, name);
									strcat(buffer, " is online!\n");
									printf("DEBUG - welcome back message : %s", buffer);
									printf("DEBUG - users_count : %d\n", users_count);

									/*****************************/
									/* Broadcast the welcome message*/
									/*****************************/
									for (int i = 0; i < users_count; i++)
									{
										if (listOfUsers[i]->sockfd == listener || listOfUsers[i]->sockfd == newfd)
										{
											continue;
										}
										printf("%d\n", listOfUsers[i]->sockfd);
										send(listOfUsers[i]->sockfd, buffer, strlen(buffer), 0);
									}
								}
							}
							else if (strncmp(buffer, "EXIT", 4)==0){
								printf("Got exit message. Removing user from system\n");
								// send leave message to the other members
                                bzero(buffer, sizeof(buffer));
								char uname[C_NAME_LEN];
								strcpy(uname, get_username(pfds[i].fd));
								printf("DEBUG - username : %s\n", uname);
								strcpy(buffer, get_username(pfds[i].fd));
								strcat(buffer, " has left the chatroom\n");
								/*********************************/
								/* Broadcast the leave message to the other users in the group*/
								/**********************************/
								pthread_mutex_lock(&clients_mutex);
								for (int i = 0; i < MAX_USERS; i++)
								{
									if (listOfUsers[i])
									{
										
										if (listOfUsers[i]->sockfd == listener)
										{
											continue;
										}
										printf("DEBUG - Sockfd : %d\n" ,listOfUsers[i]->sockfd);
										send(listOfUsers[i]->sockfd, buffer, strlen(buffer), 0);
									}
								}
								pthread_mutex_unlock(&clients_mutex);
								/*********************************/
								/* Change the state of this user to offline*/
								/**********************************/
								for (int i = 0; i < users_count; i++)
								{
									if (strcmp(listOfUsers[i]->username, uname) == 0)
									{
										printf("DEBUG - username : %s\n", listOfUsers[i]->username);
										printf("DEBUG - Changed state to offline\n");
										listOfUsers[i]->state = 0;
									}
								}
								
								//close the socket and remove the socket from pfds[]
								close(pfds[i].fd);
								del_from_pfds(pfds, i, &fd_count);
							}
							else if (strncmp(buffer, "WHO", 3)==0){
								// concatenate all the user names except the sender into a char array
								printf("Got WHO message from client.\n");
								int sendsock = pfds[i].fd;
								char name[C_NAME_LEN];
								strcpy(name, get_username(sendsock));
								char ToClient[MAX];
								bzero(ToClient, sizeof(ToClient));
								/***************************************/
								/* Concatenate all the user names into the tab-separated char ToClient and send it to the requesting client*/
								/* The state of each user (online or offline)should be labelled.*/
								/***************************************/

								for (int i = 0; i < MAX_USERS; i++)
								{
									if (listOfUsers[i])
									{
										if (strcmp(listOfUsers[i]->username, name) == 0)
										{
											continue;
										}
										strcat(ToClient, listOfUsers[i]->username);
										if (listOfUsers[i]->state == 1)
										{
											strcat(ToClient, "*");
										}

										strcat(ToClient, "\t");
									}
								}

								printf("DEBUG pfds[i].fd - %d\n", pfds[i].fd);
								printf("DEBUG - ToClient : %s\n", ToClient);
								strcat(ToClient, "\n");
								send(pfds[i].fd, ToClient, strlen(ToClient), 0);
							
							}
							else if (strncmp(buffer, "#", 1)==0){
								// send direct message 
								// get send user name:
								printf("Got direct message.\n");
								// get which client sends the message
								char sendname[MAX];
								// get the destination username
								char destname[MAX];
								// get dest sock
								int destsock;
								// get the message
								char msg[MAX];
								/**************************************/
								/* Get the source name xx, the target username and its sockfd*/
								/*************************************/
								strcpy(sendname, get_username(pfds[i].fd));
								printf("DEBUG - sendname : %s\n", sendname);
								int sendsock = get_sockfd(sendname);
								for (int i = 1; i < strlen(buffer); i++)
								{
									if (buffer[i] == ':')
									{
										strncpy(destname,buffer+1,i-1);
										strncpy(msg, buffer + i + 1, MAX);
										destname[i-1] = '\0';
										break;
									}
								}

								printf("DEBUG - destname : %s\n", destname);
								printf("DEBUG - message : %s\n", msg);

								destsock = get_sockfd(destname);
								printf("DEBUG - destsock : %d\n", destsock);

								if (destsock == -1) {
									/**************************************/
									/* The target user is not found. Send "no such user..." messsge back to the source client*/
									/*************************************/

									//TODO
									printf("DEBUG - sendsock : %d\n", sendsock);
									strcpy(buffer, "no such user...\n");
									printf("DEBUG - buffer : %s", buffer);
									send(sendsock, buffer, strlen(buffer), 0);
								}
								else {
									// The target user exists.
									// concatenate the message in the form "xx to you: msg"
									char sendmsg[MAX];
									strcpy(sendmsg, sendname);
									strcat(sendmsg, " to you: ");
									strcat(sendmsg, msg);
									

									/**************************************/
									/* According to the state of target user, send the msg to online user or write the msg into offline user's message box*/
									/* For the offline case, send "...Leaving message successfully" message to the source client*/
									/*************************************/
									
									
									for (int i = 0; i < MAX_USERS; i++)
									{
										if (listOfUsers[i])
										{
											if (strcmp(listOfUsers[i]->username, destname) == 0)
											{
												if (listOfUsers[i]->state == 1)
												{
													printf("DEBUG - username : %s\n", listOfUsers[i]->username);
													printf("DEBUG - state : %d\n", listOfUsers[i]->state);
													send(destsock, sendmsg, strlen(sendmsg), 0);
												}
												else
												{
													strcpy(buffer, destname);
													strcat(buffer, " is offline. Leaving message successfully\n");
													printf("DEBUG - buffer : %s\n", buffer);
													send(sendsock, buffer, strlen(buffer), 0);
													char filename[MAX];
													strcpy(filename, destname);
													strcat(filename, ".txt");
													FILE* fp;
													fp = fopen(filename, "a");
													fprintf(fp,"%s",sendmsg);
													fclose(fp);
												}
											}
										}
									}
								}
								
								

								if (send(destsock, sendmsg, sizeof(sendmsg), 0) == -1){
									perror("send");
								}

							}
							else
							{
								printf("Got broadcast message from user\n");
								printf("DEBUG - buffer : %s", buffer);
								/*********************************************/
								/* Broadcast the message to all users except the one who sent the message*/
								/*********************************************/

								char name[MAX];
								strcpy(name, get_username(pfds[i].fd));
								strcat(name, ": ");
								strcat(name, buffer);
								int sendsock = pfds[i].fd;
								printf("DEBUG - sendsock : %d\n", sendsock);

								pthread_mutex_lock(&clients_mutex);
								for (int i = 0; i < MAX_USERS; i++)
								{
									if (listOfUsers[i])
									{
										if (listOfUsers[i]->sockfd == listener || listOfUsers[i]->sockfd == sendsock)
										{
											continue;
										}
										printf("DEBUG - Sockfd : %d\n" ,listOfUsers[i]->sockfd);
										send(listOfUsers[i]->sockfd, name, strlen(name), 0);
									}
								}
								pthread_mutex_unlock(&clients_mutex);			
							}   

                        }
                    } // end handle data from client
                  } // end got new incoming connection
                } // end looping through file descriptors
        } // end for(;;) 
		

	return 0;
}
