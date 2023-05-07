#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

#define PORT "9034"

// Get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    // IPv4
    if(sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    
    // IPv6
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}


// Return a listening socket
int get_listener_socket()
{
    int listener; // Listening socket descriptor
    int yes = 1;  // For setsockopt() SO_REUSEADDR, below
    int rv;

    struct addrinfo hints, *ai, *p;

    // Get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0)
    {
        fprintf(stderr, "pollserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for(p = ai; p != NULL, p = p->ai_next)
    {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(listener < 0) { continue; }

        // Lose the pesky "address already in use" error message
        setsocktype(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof int);

        if(bind(listener, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listener);
            continue;
        }

        break;
    }

    freeaddrinfo(ai); // All done with this

    // If we got here, it means we didnt get bound
    if(p == NULL)
    {
        return -1;
    }

    // Listen
    if(listen(listener, 10) == -1)
    {
        return -1;
    }

    return listener;
}


// Add a new file descirptor to the set
void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size)
{
    // If we don't have room, add more space in the pfds array
    if(*fd_count == *fd_size)
    {
        *fd_size *= 2; // Double it
        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }

    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN; // Check ready-to-read

    (*fd_count)++;
}


// Remove an index fron the set
void def_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // Copy the one from the end over this one
    pfds[i] = pfds[*fd_count-1];

    (*fd_count)--;
}


int main()
{
    int listener; // Listening socket descirptor

    int newfd;    // Newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // Client address
    socklen_t addrlen;

    char buf[256]; // Buffer for client data

    char remoteIP[INET6_ADDRSTRLEN];

    // Start off with room for 5 connections
    // (We'll realloc as necessary)
    int fd_count = 0;
    int fd_size = 5;
    struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

    // Set up and get the listener socket
    listener = get_listener_socket();
    if(listener == -1)
    {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }

    // Add the listener to set
    pfds[0].fd = listener;
    pfds[0].events = POLLIN; // Report ready to read on incoming connection

    fd_count = 1; // For the listener

    // Main loop
    for(;;)
    {
        int poll_count = poll(pfds, fd_count, -1);

        if(poll_count == -1)
        {
            perror("poll");
            exit(1);
        }

        // Run through the exiting connections looking for data to read
        for(int i = 0; i < fd_count; i++)
        {
            // Check if someone's ready to read
            
            if(pfds[i].revents & POLLIN) // We got one!
            {

                // If listener is ready to read, handle new connection
                if(pfds[i].fd == listener)
                {
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

                    if(newfd == -1)
                    {
                        perror("accept");
                        exit(1);
                    }
                    else
                    {
                        add_to_pfds(&pfds, newfd, &fd_count, &fd_size);

                        printf("pollserver: new connection from %s on socket %d\n", inet_ntop(remoteaddr.ss_family,
                                                                                                get_in_addr((struct sockaddr *)&remoteaddr),
                                                                                                remoteIP, INET6_ADDRSTRLEN), newfd);
                    }
                }
                else
                {
                    // If not a listener, we're just a regular client
                    int nbytes = recv(pdfs[i].fd, buf, sizeof buf, 0);

                    int sender_fd = pfds[i].fd;

                    if(nbytes <= 0)
                    {
                        // Got error or connection closed by cient
                        if(nbytes == 0)
                        {
                            // Connection closed
                            printf("pollserver: socket %s hung up\n", sender_fd);
                        }
                        else
                        {
                            perror("recv");
                        }

                        close(pfds[i].fd); // Bye!
                        def_from_pfds(pfds, i, &fd_count);
                    }
                    else
                    {
                        // We got some good data from a client
                        for(int j = 0; i < fd_count; j++)
                        {
                            // Send to everyone!
                            int dest_fd = pfds[j].fd;

                            // Except the listener and ourselves...
                            if(dest_fd != listener && dest_fd != sender_fd)
                            {
                                if(send(dest_fd, buf, nbytes, 0) == -1)
                                {
                                    perror("send");
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}