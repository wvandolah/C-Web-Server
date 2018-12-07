/**
 * webserver.c -- A webserver written in C
 * 
 * Test with curl (if you don't have it, install it):
 * 
 *    curl -D - http://localhost:3490/
 *    curl -D - http://localhost:3490/d20
 *    curl -D - http://localhost:3490/date
 * 
 * You can also test the above URLs in your browser! They should work!
 * 
 * Posting Data:
 * 
 *    curl -D - -X POST -H 'Content-Type: text/plain' -d 'Hello, sample data!' http://localhost:3490/save
 * 
 * (Posting data is harder to test from a browser.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/file.h>
#include <fcntl.h>
#include "net.h"
#include "file.h"
#include "mime.h"
#include "cache.h"

#define PORT "3490"  // the port users will be connecting to

#define SERVER_FILES "./serverfiles"
#define SERVER_ROOT "./serverroot"

/**
 * Send an HTTP response
 *
 * header:       "HTTP/1.1 404 NOT FOUND" or "HTTP/1.1 200 OK", etc.
 * content_type: "text/plain", etc.
 * body:         the data to send.
 * 
 * Return the value from the send() function.
 */
int send_response(int fd, char *header, char *content_type, void *body, int content_length)
{
    const int max_response_size = 65536;
    char response[max_response_size];

    // Build HTTP response and store it in response

    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////
    // found this on stack overflow https://stackoverflow.com/questions/1442116/how-to-get-the-date-and-time-values-in-a-c-program
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    int response_length = sprintf(response, 
        "%s\n"
        "Date: %s"
        "Connection: close\n"
        "Content-Length: %d\n"
        "Content-Type: %s\n"
        "\n", 
        header, asctime(tm), content_length, content_type );
    // Send it all!
    memcpy(response + response_length, body, content_length);
    // printf("test: %s\n", response);
    int rv = send(fd, response, response_length + content_length, 0);
    
    if (rv < 0) {
        perror("send");
    }


    return rv;
}


/**
 * Send a /d20 endpoint response
 */
void get_d20(int fd)
{
    // Generate a random number between 1 and 20 inclusive
    // https://stackoverflow.com/questions/822323/how-to-generate-a-random-int-in-c
    srand(time(NULL));
    int r = rand() % 20;
    printf("rand int: %d\n\n", r);
    char rToString[8];
    sprintf(rToString, "%d", r);
    printf("rand int as string: %s\n", rToString);

    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////

    // Use send_response() to send it back as text/plain data
    send_response(fd, "HTTP/1.1 200 OK", "text/plain", rToString, 3);
    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////
}

/**
 * Send a 404 response
 */
void resp_404(int fd)
{
    char filepath[4096];
    struct file_data *filedata; 
    char *mime_type;

    // Fetch the 404.html file
    snprintf(filepath, sizeof filepath, "%s/404.html", SERVER_FILES);
    filedata = file_load(filepath);

    if (filedata == NULL) {
        // TODO: make this non-fatal
        fprintf(stderr, "cannot find system 404 file\n");
        exit(3);
    }

    mime_type = mime_type_get(filepath);

    send_response(fd, "HTTP/1.1 404 NOT FOUND", mime_type, filedata->data, filedata->size);

    file_free(filedata);
}

/**
 * Read and return a file from disk or cache
 */
void get_file(int fd, struct cache *cache, char *request_path)
{
    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////
    char filepath[4096];
    struct file_data *filedata; 
    char *mime_type;
    snprintf(filepath, sizeof filepath, "%s%s", SERVER_ROOT, request_path);
    struct cache_entry *cEntry = cache_get(cache, filepath);

    if(cEntry == NULL){
        printf("Not in cache\n");
        filedata = file_load(filepath);
        if(filedata == NULL){
            resp_404(fd);
            return;
        }
        mime_type = mime_type_get(filepath);
        send_response(fd, "HTTP/1.1 200 OK", mime_type, filedata->data, filedata->size);
        cache_put(cache, filepath, mime_type, filedata->data, filedata->size);
        file_free(filedata);
    }else{
        printf("In cache\n");
        send_response(fd, "HTTP/1.1 200 OK", cEntry->content_type, cEntry->content, cEntry->content_length);
    }


}

/**
 * Search for the end of the HTTP header
 * 
 * "Newlines" in HTTP can be \r\n (carriage return followed by newline) or \n
 * (newline) or \r (carriage return).
 */
// char *find_start_of_body(char *header)
// {
//     ///////////////////
//     // IMPLEMENT ME! // (Stretch)
//     ///////////////////
// }

/**
 * Handle HTTP request and send response
 */
void handle_http_request(int fd, struct cache *cache)
{
    const int request_buffer_size = 65536; // 64K
    char request[request_buffer_size];

    // Read request
    int bytes_recvd = recv(fd, request, request_buffer_size - 1, 0);

    if (bytes_recvd < 0) {
        perror("recv");
        return;
    }

    // printf("bytes_recvd: %d\n", bytes_recvd);
    // printf("what is request: %s\n", request);
    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////
    char request_type[8]; // GET or POST
    char request_path[1024]; // /info etc.
    char request_protocol[128]; // HTTP/1.1

    sscanf(request, "%s %s %s", request_type, request_path, request_protocol);
 
    // printf("what did sscanf do: %s %s %s\n", request_type, request_path, request_protocol);
    // Read the three components of the first request line

    // If GET, handle the get endpoints
    if(strcmp(request_type, "GET") == 0){
        if(strcmp(request_path, "/d20") == 0){
            get_d20(fd);
        } else {
            get_file(fd, cache, request_path);
        }
    }
    //    Check if it's /d20 and handle that special case
    //    Otherwise serve the requested file by calling get_file()


    // (Stretch) If POST, handle the post request
}

/**
 * Main
 */
int main(void)
{
    int newfd;  // listen on sock_fd, new connection on newfd
    struct sockaddr_storage their_addr; // connector's address information
    char s[INET6_ADDRSTRLEN];

    struct cache *cache = cache_create(10, 0);

    // Get a listening socket
    int listenfd = get_listener_socket(PORT);

    if (listenfd < 0) {
        fprintf(stderr, "webserver: fatal error getting listening socket\n");
        exit(1);
    }

    printf("webserver: waiting for connections on port %s...\n", PORT);

    // This is the main loop that accepts incoming connections and
    // forks a handler process to take care of it. The main parent
    // process then goes back to waiting for new connections.

    while(1) {
        socklen_t sin_size = sizeof their_addr;

        // Parent process will block on the accept() call until someone
        // makes a new connection:
        newfd = accept(listenfd, (struct sockaddr *)&their_addr, &sin_size);
        if (newfd == -1) {
            perror("accept");
            continue;
        }
        // resp_404(newfd);
        // Print out a message that we got the connection
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);
        
        // newfd is a new socket descriptor for the new connection.
        // listenfd is still listening for new connections.

        handle_http_request(newfd, cache);

        close(newfd);
    }

    // Unreachable code

    return 0;
}

