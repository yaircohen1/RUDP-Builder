#pragma once

#include <stdlib.h>

struct _fileList;
typedef struct _fileList fileList;

/*
 * Allocates a new empty fileList.
 * It's the user responsibility to free it with fileList_free.
 */
fileList *fileList_alloc();

/*
 * Frees the memory and resources allocated to fileList.
 * If StrList==NULL does nothing (same as free).
 */
void fileList_free(fileList *fileList);

/*
 * Returns the number of elements (files) in the fileList.
 */
size_t fileList_size(const fileList *fileList);

/*
 * Inserts an element(new received file) in the end of the fileList with time and size 
 */
 void fileList_insertLast(fileList *fileList, double get_time, int file_size);

/*
 * Function to calculate speed in megabytes per second
 */
double calculateSpeed(int fileSizeBytes, double timeMilliseconds);

/*
 * Prints the fileList statistics for each file (time + speed) 
 */
void fileList_print(const fileList *fileList);

/*
 * Prints the fileList average time by combine each time file received and divide by the amount of files 
 */
void fileList_AverageT_print(const fileList *fileList);

/*
 * Prints the fileList average speed by combine each file speed and divide by the amount of files 
 */
void fileList_AverageBT_print(const fileList *fileList);