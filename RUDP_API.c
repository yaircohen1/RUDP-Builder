#include "RUDP_API.h"

#define MAX_BUFFER_SIZE 2048
#define MAX_RETRANSMISSION_ATTEMPTS 10

// RUDP Header
typedef struct UDP_Header {
    unsigned short Length; // 2 Bytes (Byte 0, Byte 1) for length of data
    unsigned short Checksum; // 2 Bytes (Byte 2, Byte 3) for checksum
    char Flag; // 1 Byte for: SYN = 'S', ACK = 'A', Data = 'D', FIN = 'F'
    char Content [MAX_BUFFER_SIZE];
} Packet;

// Creating a RUDP socket
int rudp_socket() {
    int soc = socket(AF_INET, SOCK_DGRAM, 0); // Create a UDP socket for IPv4
    if (soc == -1) {
        perror("Socket creation failed");
        return -1;
    }
    return soc;
}

// Function to calculate checksum
unsigned short int calculate_checksum(void *data, size_t  bytes) {
    unsigned short int *data_pointer = (unsigned short int *)data;
    unsigned int total_sum = 0;
    
    // Main summing loop
    while (bytes > 1) {
        total_sum += *data_pointer++;
        bytes -= 2;
    }
    // Add left-over byte, if any
    if (bytes > 0){
        total_sum += *((unsigned char *)data_pointer);
        total_sum &= 0xFFFF; // Ensure that the sum is within 16 bits
    }
    // Fold 32-bit sum to 16 bits
    while (total_sum >> 16)
        total_sum = (total_sum & 0xFFFF) + (total_sum >> 16);

    return (~((unsigned short int)total_sum));
}

// Function to verify checksum
int verify_checksum(Packet *buffer, size_t bytes) {
   // Extract data and checksum from the packet
    unsigned short *data_pointer= (unsigned short int *)buffer->Content;
    unsigned short checksum = buffer->Checksum;
    unsigned int total_sum = checksum;

    // Calculate the checksum of the received data combined with the checksum
    // Main summing loop
    while (bytes > 1) {
        total_sum += *data_pointer++;
        bytes -= 2;
    }
    // Add left-over byte, if any
    if (bytes > 0){
        total_sum += *((unsigned char *)data_pointer);
        total_sum &= 0xFFFF; // Ensure that the sum is within 16 bits
    }
    // Fold 32-bit sum to 16 bits
    while (total_sum >> 16)
        total_sum = (total_sum & 0xFFFF) + (total_sum >> 16);
    
    // If the sum is all ones, the checksum is correct
    int result = total_sum == 0xFFFF ? 1 : -1;
    return result;
}

// *** Sender's functions: ***

// Function that checks if Sender got ACK Packet
int got_ACK(int sockfd, struct sockaddr * from, socklen_t * fromlen) {
    Packet buffer;
    memset(&buffer, 0, sizeof(Packet));

    ssize_t ACK = recvfrom(sockfd, &buffer, sizeof(Packet), 0, from, fromlen);
    
    // setsockopt(SO_RCVTIMEO): Causes the receive operation to return with an error (-1 with errno set to EAGAIN or EWOULDBLOCK)
    // if the timeout expires before data is received. Need to check for this error condition explicitly.
    if (ACK == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {         
            perror("ACK packet failed to be received: the timeout expires before data is received.");
            return -10;
        } else {
                perror("ACK packet failed to be received.");
                close(sockfd);
                return -1;
        }
    }
    if (ACK == 0) {
        perror("The peer has closed the connection");
        close(sockfd);
        return 0;
    }
    if (buffer.Flag == 'A') {
        return 1; // Successfully received ACK
    }
    return 0;
}
 
// Creating handshake between two peers (Sender send SYN message, Rec recieved the SYN message & send ACK, Sender recieve ACK)
// An image of the TCP connect() function that will ensure a handshake 
int handshake_connect(int sockfd, struct sockaddr *serv_addr, socklen_t addrlen, struct sockaddr* respond_server, socklen_t * respond_server_len) {
    printf("Sending request for RDUP connection\n");
   
    // Send SYN message - send just flag without a real data
    Packet SYN;
    memset(&SYN, 0, sizeof(SYN)); // ensure struct is clean
    SYN.Length = 0;
    SYN.Flag = 'S';
    
    int attempts = 0;
    // The function wait for an acknowledgment packet, if it didnt receive any, retransmits the packet till default max_attempts
    // This for preventing infinite loops in case of persistent failures
    while (attempts < MAX_RETRANSMISSION_ATTEMPTS) {
        // send SYN packet 
        ssize_t size_SYN = sendto(sockfd,&SYN,sizeof(Packet), 0, serv_addr,addrlen); 
        if (size_SYN<=0) {
            perror("SYN packet failed to be send");
            close(sockfd);
            return -1;
        }

        int ACK_Status = got_ACK (sockfd, respond_server, respond_server_len);
        // Check if no timeout, out of the loop
        if (ACK_Status != -10) {
            if (ACK_Status == 1) 
                printf("Got ACK from Receiver.\n");
            return ACK_Status;
        } 
        attempts++;
        printf("Retransmission attempt %d\n", attempts);
    }

    // If maximum retransmission attempts reached
    printf("Maximum retransmission attempts reached. Handshake failed.\n");
    return -1;
}

// Sending data to the peer (after receive ACK packet)
int rudp_send(int sockfd, const void*msg, int len, struct sockaddr *serv_addr, socklen_t addrlen, struct sockaddr* respond_server, socklen_t * respond_server_len){
    // Send data message
    Packet data;
    memset(&data, 0, sizeof(data)); // ensure struct is clean
    data.Length = len;
    data.Flag = 'D';
    strcpy(data.Content, msg);
    data.Checksum = calculate_checksum(data.Content, data.Length);

    int attempts = 0;
    
    // The function wait for an acknowledgment packet, if it didnt receive any, retransmits the data till default max_attempts
    // This for preventing infinite loops in case of persistent failures
    while (attempts < MAX_RETRANSMISSION_ATTEMPTS) {
    ssize_t dataSent = sendto(sockfd, &data, sizeof(Packet), 0, (const struct sockaddr *)serv_addr,addrlen); 
        if (dataSent<=0) {
            perror("data packet failed to be send");
            close(sockfd);
            return -1;
        }
        int ACK_Status = got_ACK (sockfd, respond_server, respond_server_len);
        // Check if no timeout, out of the loop (if ACK is 1, return size of dataSent)
         if (ACK_Status == -1) {
            return -1;
        } 
        if (ACK_Status == 0) {
            return 0;
        } 
        if (ACK_Status == 1) {
            return dataSent;
        } 
        attempts++;
        printf("Retransmission attempt %d\n", attempts);
    }
    // If maximum retransmission attempts reached
    printf("Maximum retransmission attempts reached. Sending the data failed.\n");
    return -1;
}

int rdup_close(int sockfd, struct sockaddr *serv_addr, socklen_t addrlen, struct sockaddr * respond_server, socklen_t * respond_server_len) {
    printf("Sending request for exit.\n");
   
    // Send FIN message - send just flag without a real data
    Packet FIN;
    memset(&FIN, 0, sizeof(FIN)); // ensure struct is clean
    strcpy(FIN.Content,"Exit");
    FIN.Length = 0;
    FIN.Flag = 'F';
    
    int attempts = 0;
    // The function wait for an acknowledgment packet, if it didnt receive any, retransmits the packet till default max_attempts
    // This for preventing infinite loops in case of persistent failures
    while (attempts < MAX_RETRANSMISSION_ATTEMPTS) {
        // send FIN packet 
        ssize_t size_FIN = sendto(sockfd, &FIN, sizeof(Packet), 0, (const struct sockaddr *)serv_addr,addrlen); 
        if (size_FIN<=0) {
            perror("FIN packet failed to be send");
            close(sockfd);
            return -1;
        }

        int ACK_Status = got_ACK (sockfd, respond_server, respond_server_len);
        // Check if no timeout, out of the loop
        if (ACK_Status != -10) {
            if (ACK_Status == 1) {
                printf("Got ACK from Receiver.\n");
                printf("Exit.\n");
            }
            return ACK_Status;
        } 
        attempts++;
        printf("Retransmission attempt %d\n", attempts);
    }

    // If maximum retransmission attempts reached
    printf("Maximum retransmission attempts reached. disconnect failed.\n");
    return -1;
}

// *** Receiver's functions: ***

// Function that send ACK packet to Sender
int send_ACK(int sockfd, struct sockaddr * dest, socklen_t destlen) {   
    // Send ACK message - send just flag without a real data
    Packet ACK;
    memset(&ACK, 0, sizeof(ACK)); // ensure struct is clean
    ACK.Length = 0;
    ACK.Flag = 'A';
    
    ssize_t size_ACK = sendto(sockfd, &ACK, sizeof(Packet), 0, (const struct sockaddr *)dest, destlen);
    if (size_ACK<=0) {
            perror("ACK packet failed to be send");
            close(sockfd);
            return -1;
        }
    return 1;
}

// Function to receive any type of packet,
// after receive successfully, send ACK packet to the Sender 
int rudp_receive(int sockfd, struct sockaddr * Sender_adrr, socklen_t * Sender_len) {
    Packet buffer;
    memset(&buffer, 0, sizeof(Packet));

    ssize_t rec_size = recvfrom(sockfd, &buffer, sizeof(Packet), 0, Sender_adrr, Sender_len);
    if (rec_size<0) {
            perror("packet failed to be received");
            close(sockfd);
            return -1;
        }
    // Connection closed
    if (rec_size == 0) {
            printf("Connection closed by peer.\n");
            close(sockfd);       
            return 0;
    }

    // Ensure that the buffer is null-terminated, no matter what message was received.
    // This is important to avoid SEGFAULTs when printing the buffer.
    if (buffer.Content[MAX_BUFFER_SIZE - 1] != '\0')
           buffer.Content[MAX_BUFFER_SIZE- 1] = '\0';
    
    int ACK = 0;
    // Got a SYN packet (Sender wants to connect)
    if(buffer.Flag == 'S') {
        printf("Connection request received, sending ACK.\n");
        ACK = send_ACK(sockfd, Sender_adrr,*Sender_len);
        if (ACK == -1){
            perror("packet ACK failed to be Send for start connection");
            close(sockfd);
            return -1;
        }
        printf("Sender connected, beginning to receive file...\n");
        return 1;
    }
    // Got a Data packet
    if(buffer.Flag == 'D') {
        int val_checksum = verify_checksum(&buffer,buffer.Length);
        if (val_checksum == -1) {
            perror("Checksum is not valid");
            close(sockfd);
            return -1;
        } else { 
            ACK = send_ACK(sockfd, Sender_adrr,*Sender_len);
            if (ACK == -1){
                perror("packet ACK failed to be Send data");
                close(sockfd);
                return -1;
            }
            return rec_size;
        }   
    }

    // Got a FIN packet (Sender wants to close connection)
    if(buffer.Flag == 'F') {
        printf("Sender sent exit message.\n");
        ACK = send_ACK(sockfd, Sender_adrr,*Sender_len);
        if (ACK == -1){
            perror("packet ACK failed to be Send for start connection");
            close(sockfd);
            return -1;
        }
        printf("ACK sent.\n");
        memset(buffer.Content, 0, MAX_BUFFER_SIZE);
        return 2;
    }

   
    return 0;
}








