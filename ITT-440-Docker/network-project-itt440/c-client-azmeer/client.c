#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

int main(int argc, char *argv[]) {
    // CRITICAL: Disable output buffering for immediate logs
    setbuf(stdout, NULL); 

    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[1024];

    // --- Configuration from Environment Variables ---
    char *host_name = getenv("TARGET_HOST"); // Should be 'c-server'
    portno = atoi(getenv("TARGET_PORT")); // Should be 5001

    if (host_name == NULL || portno == 0) {
        fprintf(stderr,"[C Client] ERROR: TARGET_HOST or TARGET_PORT not set.\n");
        exit(1);
    }

    int MAX_RETRIES = 5;
    int RETRY_DELAY = 5; // seconds
    int attempt = 0;

    // 1. Get server address information
    server = gethostbyname(host_name);
    if (server == NULL) {
        fprintf(stderr,"[C Client] ERROR, no such host: %s\n", host_name);
        exit(1);
    }

    // 2. Setup server address structure (static)
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    // --- Connection and Retry Loop ---
    for (attempt = 0; attempt < MAX_RETRIES; attempt++) {
        // Create socket inside the loop (must be created before each connect attempt)
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("[C Client] ERROR opening socket");
            exit(1);
        }

        fprintf(stdout, "[C Client] Attempt %d/%d: Connecting to %s:%d...\n", attempt + 1, MAX_RETRIES, host_name, portno);
        
        // Attempt to Connect
        if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) >= 0) {
            fprintf(stdout, "[C Client] Connection successful.\n");
            goto successful_connection; // Jump out of the loop on success
        } else {
            // Connection failed
            perror("[C Client] ERROR connecting");
            close(sockfd); // Close the failed socket before retrying
            
            if (attempt < MAX_RETRIES - 1) {
                fprintf(stdout, "[C Client] Retrying in %d seconds...\n", RETRY_DELAY);
                sleep(RETRY_DELAY);
            } else {
                fprintf(stderr, "[C Client] Failed to connect after all retries. Exiting.\n");
                exit(1); // Final exit with error
            }
        }
    }
    
successful_connection: ; // Label for the goto statement

    // 5. Send message
    bzero(buffer, 1024);
    char *message = "REQUEST_LATEST_POINTS";
    int n = write(sockfd, message, strlen(message));
    if (n < 0) {
        perror("[C Client] ERROR writing to socket");
        close(sockfd);
        exit(1);
    }
    fprintf(stdout, "[C Client] Sent: %s\n", message);

    // 6. Read response
    bzero(buffer, 1024);
    // Read the response from the server (which is now formatted like the Python server's response)
    n = read(sockfd, buffer, 1023);
    if (n < 0) {
        perror("[C Client] ERROR reading from socket");
        close(sockfd);
        exit(1);
    }
    
    fprintf(stdout, "\n--- SERVER RESPONSE ---\n");
    // Print the received data, which includes the actual points and timestamp
    fprintf(stdout, "%s", buffer); 
    fprintf(stdout, "-----------------------\n");

    // 7. Close socket
    close(sockfd);
    return 0; // Exit successfully
}