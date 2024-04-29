#include "RUDP_API.h"
#include "LinkedList.h"

#define MAX_BUFFER_SIZE 2048 

//  a function to calculate milliseconds
double get_time_in_milliseconds(struct timeval start, struct timeval end) {
    return (double)(end.tv_sec - start.tv_sec) * 1000.0 + (double)(end.tv_usec - start.tv_usec) / 1000.0;
}

int main(int argc, char *argv[]) {
    
    // *** Pre-Parts : Get from the user the command from terminal ***

    if (argc != 3) {        // ./RUDP_Receiver -p 12345
        fprintf(stderr, "Please provide the correct usage for the program: %s -ip IP -p PORT -algo ALGO <FILE_PATH>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Extract command-line arguments
    int port = 0;

    // Process command-line arguments
    for (int i = 1; i < argc; i++) {
         if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[i + 1]); // Convert a string representing an integer (ASCII string) to an integer value.
            i++;
        } 
    }

    // Check if required arguments are provided
    if (port == 0) {
        fprintf(stderr, "Invalid arguments. Please provide the correct usage.\n");
        exit(EXIT_FAILURE);
    }

    // *** Part A: Create a UDP connection between the Receiver and the Sender ***
    printf("Starting Receiver...\n");

    //Create socket
    int listeningSocket = rudp_socket(); // Create a UDP socket for IPv4

    // The variable to store the socket option for reusing the server's address.
    int opt = 1;

    // Set the socket option to reuse the server's address.
    // This is useful to avoid the "Address already in use" error message when restarting the server.
    if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("Error setting sockopt");
        close(listeningSocket);
        return 1;
    }

    // Define Receiver's values
    struct sockaddr_in server_address; // Struct sockaddr_in is defined in the <netinet/in.h> header file.
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY; // The server will listen on all available network interfaces.
    server_address.sin_port = htons(port); // htons() function ensures that the port number is properly formatted for network communication, regardless of the byte order of the host machine.

    // Bind the socket to the specified address and port
    if (bind(listeningSocket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Bind failed");
        close(listeningSocket);
        exit(EXIT_FAILURE);
    }

    // *** Part B: Get a connection from the sender ***

    // Define Sender
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);
    memset(&client_address, 0, sizeof(client_address));

    // Accept a connection
    printf("Waiting for RUDP connection...\n");
    ssize_t recvSYN = rudp_receive(listeningSocket, (struct sockaddr *)&client_address, &client_address_len);
    if (recvSYN<=0) {
        exit(EXIT_FAILURE);
    }
    
    // *** Part C: Receive the file, measure the time it took to save ***

    struct timeval start_time, end_time;
    fileList* files = fileList_alloc(); // Start a new null list of files 
    
    // Receive the size of the file from the sender
    unsigned int file_size;
    ssize_t size_received = recvfrom(listeningSocket, &file_size, sizeof(file_size), 0, (struct sockaddr *)&client_address, &client_address_len);
    if (size_received <= 0) {
        perror("Error receiving file size");
        close(listeningSocket);
        exit(EXIT_FAILURE);
    }

    // Receive the data of the file from the sender 
    while (1)
    {
        int total_received = 0;
        while(total_received<file_size) {
            gettimeofday(&start_time,NULL);
            ssize_t bytes_received = rudp_receive(listeningSocket, (struct sockaddr *)&client_address, &client_address_len);
            if (bytes_received <= 0) { 
                exit(EXIT_FAILURE);
            }
            total_received+=bytes_received;
        }
    
        gettimeofday(&end_time, NULL); // Set the end time before measuring elapsed time
        double elapsed_time = get_time_in_milliseconds(start_time, end_time);
        fileList_insertLast (files,elapsed_time,file_size); // Create new file 
        printf("File transfer completed (%d bytes).\n",total_received);
        printf("ACK sent.\n");
        printf("Waiting for Sender response...\n");

        // ** Part D: Wait for Sender Response **
        char decision[4];
        ssize_t decision_rec = recvfrom(listeningSocket, decision, sizeof(decision), 0, (struct sockaddr *)&client_address, &client_address_len);
        if (decision_rec <= 0) {
            perror("Error receiving decision");
            close(listeningSocket);
            exit(EXIT_FAILURE);
        }
        // if user don't want to send the file again, the next time in the loop won't measure time and will get exit message
        if(decision[0]=='n') {
            memset(decision, 0, strlen(decision));
            break;
        }
    }   
    
    // Get Exit message from Sender
    ssize_t Exit = rudp_receive(listeningSocket, (struct sockaddr *)&client_address, &client_address_len);
        if (Exit <= 0) {
            perror("Error receiving Exit message");
            exit(EXIT_FAILURE);
        }

    // ** Part E: Print out statistics **
    
    printf("----------------------------------\n");
    printf("- * Statistics * -\n");
    fileList_print(files); //fucntion that prints time and speed for each file

    // ** Part F: Calculate the average time and the total average bandwith **
    
    fileList_AverageT_print(files);
    fileList_AverageBT_print(files);
    printf("----------------------------------\n");
    close(listeningSocket);
    fileList_free(files);
    
    // ** Part G: Exit **

    printf("Receicer end.\n");
    return 0;
 }



