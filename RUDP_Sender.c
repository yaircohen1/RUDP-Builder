#include "RUDP_API.h"

#define MAX_BUFFER_SIZE 2048  

char *util_generate_random_data(unsigned int size) {
    char *buffer = NULL;
    // Argument check.
    if (size == 0)
        return NULL;
    buffer = (char *)calloc(size, sizeof(char));
    // Error checking.
    if (buffer == NULL)
        return NULL;
    // Randomize the seed of the random number generator.
    srand(time(NULL));
    for (unsigned int i = 0; i < size; i++)
        *(buffer + i) = ((unsigned int)rand() % 256);
    return buffer;
}
   
int main(int argc, char *argv[]) {

    // *** Pre-Parts : Get from the user the command from terminal ***

    // Expecting 6 arguments in total (excluding the program name)
    if (argc != 5) {       // ./RUDP_Sender -ip 127.0.0.1 -p 12345 
        fprintf(stderr, "Please provide the correct usage for the program: %s -ip <IP> -p <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Extract command-line arguments
    const char *ip_address = NULL;
    int port = 0;
   
    // Process command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-ip") == 0 && i + 1 < argc) {
            ip_address = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[i + 1]); // Convert a string representing an integer (ASCII string) to an integer value.
            i++;
        } 
    }

   // Check if required arguments are provided
    if (ip_address == NULL || port == 0) {
        fprintf(stderr, "Invalid arguments. Please provide the correct usage.\n");
        exit(EXIT_FAILURE);
    }

    
    // *** Part A: Create file and read it ***

    // Create File 
    const char *file_path = "random_data.txt"; // Adjust file path as needed
    unsigned int min_size = 2*1024*1024; // At least file with 2MB size
    unsigned int size;
    
    // Seed the random number generator with the current time
    srand(time(NULL));
    // Generate a random size greater than or equal to the minimum size
    size = min_size + (rand() % (5 * 1024 * 1024)); // Generate between 2MB to 7MB to make sure the file is at least 2MB
    
    // Step 1: Generate random data
    char *data = util_generate_random_data(size);
    
    // Step 2: Check if random data generation failed
    if (data == NULL) {
        perror("Random data generation failed");
        exit(EXIT_FAILURE);
    }

    // Step 3: Create and write data to file
    FILE *file = fopen(file_path, "wb"); // wb - write binary
    if (!file) {
        perror("File creation failed");
        free(data);
        exit(EXIT_FAILURE);
    }
    fwrite(data, 1, size, file);
    fclose(file);
    
    // Read the file
    // Step 1: open data from file for reading
    fopen(file_path, "rb"); // rb - read binary
    if (!file) {
        perror("File open for reading failed");
        free(data);
        exit(EXIT_FAILURE);
    }

    // Step 2: Read the entire file
    if (fread(data, 1, size, file) != size) {
    perror("Error reading file");
    fclose(file);
    free(data);
    exit(EXIT_FAILURE);
    }

    // *** Part B: Create UDP socket between Sender - Receiver ***
    
    printf("Starting Sender...\n");

    //creat socket
    int _sockfd = rudp_socket();
    if (_sockfd == -1){
        free(data);
        return -1;
    }

    // Define a timeout for Sender to receive ACK
    // According to the article you published,the vaild range for Timeout is 0 to 65536 (in ms), so I chose 10000ms randomly
    struct timeval timeout = {0,15530};

    //create receiver address struct
    struct sockaddr_in server_address; // Struct sockaddr_in is defined in the <netinet/in.h> header file.
    memset(&server_address, 0, sizeof(server_address));
    
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port); // htons() function ensures that the port number is properly formatted for network communication, regardless of the byte order of the host machine.
    int rval = inet_pton(AF_INET, ip_address, &server_address.sin_addr); // inet_pton() is a function that converts an IPv4 address in string format to a binary format 
    if (rval <= 0){
        close(_sockfd);
        free(data);
		printf("inet_pton() failed");
		return -1;
	}

    // The variable to store the server's address length.
    socklen_t server_len = sizeof(server_address);

    // Since in UDP there is no connection, there isn't a guarantee that the server is up and running.
    // Therefore, we set a timeout for the recvfrom() call using the setsockopt function.
    // If the server (RUDP_Receiver) does not respond within the timeout, the client (RUDP_Sender) will drop.
    if (setsockopt(_sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == -1) {
        perror("setsockopt() failed");
        close(_sockfd);
        free(data);
        return -1;
    }

    // The variable to store the server's address, that responded to the message.
    // it is required by the recvfrom function (that included in rudp_handshake function)
    // Note that the target server might be different from the server that responded to the message.
    struct sockaddr_in respond_server;

    // The variable to store the respond server's address length.
    socklen_t respond_server_len = sizeof(respond_server);
    memset(&respond_server, 0, sizeof(respond_server));

    // Ensure theres a handshake between Sender and Receiver
    int handshake = handshake_connect(_sockfd, (struct sockaddr*)&server_address, server_len, (struct sockaddr*)&respond_server, &respond_server_len);
    if (handshake != 1) {
        perror("Handshake failed");
        close(_sockfd);
        free(data);
        return -1;
    }

    printf("Receiver connected, beginning to send file...\n");

    // *** Part C + D: Send the file via the RUDP protocol + User decision ***
    
    // Send the size of the file to the receiver
    ssize_t size_sent = sendto(_sockfd, &size, sizeof(size),0,(const struct sockaddr *)&server_address, server_len);
    if (size_sent < 0) {
        perror("Error sending file size");
        close(_sockfd);
        free(data);
        exit(EXIT_FAILURE);
    }


    int send_again = 1; // Flag to control the loop
     while (send_again>0) {
        // Send the file in chuncks to minimize packet loss and bad TCP
        printf("Send the file...\n");
        unsigned int sent_total = 0;
        while (sent_total<size) {
            unsigned int remaining = size - sent_total;
            unsigned int chunk_size = remaining < MAX_BUFFER_SIZE ? remaining : MAX_BUFFER_SIZE;
            // ssize_t vab for negtive num for erorrs
            ssize_t sent = rudp_send(_sockfd, data+sent_total, chunk_size, (struct sockaddr*)&server_address, server_len, (struct sockaddr*)&respond_server, &respond_server_len);
            if (sent < 0) {
                perror("Error sending random data");
                close(_sockfd);
                free(data);
                exit(EXIT_FAILURE);
            }
            sent_total += sent;
        }
        printf("Got ACK from Receiver.\n");
        printf("a file with %u bytes has been sent successfully\n", sent_total);

        // Ask user for decision
        printf("Do you want to send the file again? (yes/no): ");
        
        // Get only valid answers - yes or no 
        int valid_char = 0;
        while (valid_char==0) {
            char decision[4];
            scanf("%s",decision);
            if(strcmp(decision, "no")==0 || strcmp(decision, "yes")==0) {
                ssize_t send_decision = sendto(_sockfd, decision, strlen(decision),0, (struct sockaddr*)&server_address, server_len);
                if(send_decision < 0){
                    perror("Error sending decision");
                    close(_sockfd);
                    free(data);
                    exit(EXIT_FAILURE);
                }
                if (strcmp(decision, "no")==0) {
                    send_again = 0; // If decision is not "yes", exit loop
                }
                valid_char=1;
            }
            else {
                    printf("not valid answer, try again\n");      
            }
        }
    }

    // *** Part E+ F: Send exit message to the receiver + Close the RUDP connection ***
    int close_connection = rdup_close(_sockfd, (struct sockaddr*)&server_address, server_len, (struct sockaddr*)&respond_server, &respond_server_len);
    if (close_connection != 1) {
        perror("disconnect failed");
        close(_sockfd);
        free(data);
        return -1;
    }
    close(_sockfd);
    free(data);
    remove(file_path); // Remove temporary file

    // *** Part G: Exit ***
    return 0;
}
