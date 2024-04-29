#include <stdio.h>
#include <stdlib.h>
#include "LinkedList.h"


// Node & List Data Structures
typedef struct _node {
  double _time;
  int _file_size;
  struct _node *_next;
} Node;

typedef struct _fileList {
  Node *head;
  size_t size;
} fileList;

Node *createNode(double get_time, int file_size, Node *next) {
    Node *newNode = (Node*)malloc(sizeof(Node)); // Allocation of memory space
    if (newNode == NULL) {
       return NULL;
    }
    newNode->_time = get_time;
    newNode->_file_size = file_size;
    newNode->_next = next;
    return newNode;
}

void Node_free(Node *node) { 
  if (node!=NULL){
    free(node); 
  }
}

void fileList_free(fileList *fileList) {
  if (fileList == NULL) {
        return;
  }
  Node *current = fileList->head;
  while (current!=NULL) {
    Node *next = current ->_next;
    Node_free(current);
    current=next;
  }
  free(fileList);
}

fileList *fileList_alloc() {
  fileList *p = (fileList *)malloc(sizeof(fileList));
  p->head = NULL;
  p->size = 0;
  return p;
}

size_t fileList_size(const fileList *fileList) {
   return fileList->size;
 }

 void fileList_insertLast(fileList *fileList, double get_time, int file_size) {
  Node *curr = fileList->head; 
  Node *tmp = createNode(get_time,file_size,NULL);
  if (tmp == NULL) {
      return; // Memory allocation failed
  }
  if (curr==NULL) {
    fileList->head = tmp;
  }
  else {
    // find the last node
    while (curr->_next != NULL){
      curr = curr->_next;
     }
    curr->_next = tmp; // curr is the last node we found and add tmp after curr 
  }
   ++(fileList->size);
}

// Function to calculate speed in megabytes per second
double calculateSpeed(int fileSizeBytes, double timeMilliseconds) {
    double fileSizeMB = (double)fileSizeBytes / (1024 * 1024); // Convert bytes to megabytes
    double timeSeconds = timeMilliseconds / 1000; // Convert milliseconds to seconds
    return fileSizeMB / timeSeconds;
}

void fileList_print(const fileList *fileList) {
    Node *p = fileList->head;
    for(int index = 1; p!=NULL; index++) {    
    printf("- Run #%d Data: Time =%.2fms; Speed=%.2fMB/s\n", index, p->_time, calculateSpeed(p->_file_size, p->_time));
    p = p->_next;
  }
}

void fileList_AverageT_print(const fileList *fileList) {
    double time_combined = 0;
    Node *p = fileList->head;
    while (p){
        time_combined+=p->_time;
        p=p->_next;
    }
    printf("- Average time: %.2fms\n", time_combined/(double)fileList->size);
}

void fileList_AverageBT_print(const fileList *fileList) {
    double speed_combined = 0;
    Node *p = fileList->head;
    while (p){
        speed_combined+=calculateSpeed(p->_file_size, p->_time);
        p=p->_next;
    }
    printf("- Average bandwidth: %.2fMB/s\n", speed_combined/(double)fileList->size);
}