/*
* BuddyBuffer.c
*
*  Created on: Apr 3, 2018
*      Author: Colin Quinn
*
* ICS 462 Assignment #3
*
* This program contains the functions for a buddy buffer system and a
* test driver for the functions
*
* The BuddyBuffer space contains 10 512 word blocks, that can be divided by 2
* until a minimum size of 8 is reached.  The first word is a control word, so
* the buffer space avaialable to the user is 1 less than the block size.
*
* When blocks are returned, they are recombined into larger blocks if possible
*
* The buddy buffers are allocated using a first-fit algorithm.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <share.h>
#include <time.h>

#define MAX_COUNT 10
#define MIN_COUNT 2
#define MAX_SIZE 512
#define MIN_SIZE 8

#define SIZE_6 256
#define SIZE_5 128
#define SIZE_4 64
#define SIZE_3 32
#define SIZE_2 16



/* Data */

// Control block structure for buffers
typedef struct control_block {
	// Holds up to 8 sizes
	unsigned int size : 3;
	// buffer is available to lend out
	unsigned int available : 1;
	// buffer is the top of it's buddy buffer pair
	unsigned int top : 1;
} ControlBlock;

// Pointer for buddy buffers
static ControlBlock * buddy_buffer_pointer = NULL;

// Number of Max-sized buffers
static short int max_size_count = MAX_COUNT;



/* Private Function declarations */

// Initializes buddy buffer memory block and returns pointer to start of block
static ControlBlock * createBuddyBuffers();

// Gets compact size representation based on amount of words requested
static unsigned short int getBufferSize(
	unsigned short int actual_size, unsigned short int size, unsigned short int max
);

// Gets offset to next control word based on compact size
static int getOffset(unsigned short int size);

// Splits block into smaller blocks until block size == request size
static ControlBlock * splitBuffer(unsigned short int request_size, ControlBlock * buffer);

// Combines buffer with equal sized buddy
// Continues to combine if new buffer is equal sized to its buddy
static void merge(ControlBlock * buffer);



/* Public Function Declarations */

// Returns true if memory is OK, false if memory is tight
bool bufferPoolStatus();

// Returns a buffer of a specified size if possible
// Returns -2 for an invalid request, -1 if no memory available
void * requestBuffer(unsigned short int size);

// Gives back a buffer to the buddy buffer block
// Combines equal-sized available buddies into larger blocks
void returnBuffer(void * return_pointer);

// Prints status of buddy buffer block to file
// Prints memory status and number of blocks of each size;
int debugBuffers(FILE * file);



/* main */

// Tests buddy buffer functionality
int main(int argc, char *argv[]) {

	// File data
	const char * file_name = "HW3Out.txt";
	FILE * file;

	// Open output file
	if ((file = _fsopen(file_name, "w", _SH_DENYNO)) == NULL) {
		fprintf(stderr, "Error opening file\n");
		return 1;
	}

	fprintf(file, "Colin Quinn\nICS 462 Assignment #3\n\n");

	// Test code

	// Request 700 word buffer
	fprintf(file, "Request 512 word buffer.\n"
		"Expected values:\n"
		"\tAddress: -2\n"
		"\tMemory status: OK\n\tCount of buffer sizes:\n\t511 : 10, 255 : 0, 127 : 0, 63 : 0, 31 : 0, 15 : 0, 7 : 0\n\n"
	);
	ControlBlock * buffer = requestBuffer(512);
	fprintf(file, "\tAddress: %d\n", buffer);
	debugBuffers(file);

	// Request 4 word buffer
	fprintf(file, "Request 4 word buffer.\n"
		"Expected values:\n"
		"\tMemory status: OK\n\tCount of buffer sizes:\n\t511 : 9, 255 : 1, 127 : 1, 63 : 1, 31 : 1, 15 : 1, 7 : 1\n\n"
	);
	buffer = requestBuffer(4);
	fprintf(file, "\tAddress: %p\n", buffer);
	debugBuffers(file);

	// Return 7 word buffer
	fprintf(file, "Return 7 word buffer.\n"
		"Expected values:\n"
		"\tMemory status: OK\n\tCount of buffer sizes:\n\t511 : 10, 255 : 0, 127 : 0, 63 : 0, 31 : 0, 15 : 0, 7 : 0\n\n"
	);
	returnBuffer(buffer);
	debugBuffers(file);

	// Request 10 buffers
	fprintf(file, "Request 10 511 word buffers.\n"
		"Expected values:\n"
		"\tMemory status: tight\n\tCount of buffer sizes:\n\t511 : 0, 255 : 0, 127 : 0, 63 : 0, 31 : 0, 15 : 0, 7 : 0\n\n"
	);
	ControlBlock ** buffers = malloc(sizeof(ControlBlock *) * 10);
	int i;
	for (i = 0; i < 10; i++) {
		buffers[i] = requestBuffer(510);
		fprintf(file, "\tAddress: %p\n", buffers[i]);
	}
	debugBuffers(file);

	// Request extra buffer
	fprintf(file, "Request extra 511 word buffer.\n"
		"Expected values:\n"
		"\tAddress: -1\n"
		"\tMemory status: tight\n\tCount of buffer sizes:\n\t511 : 0, 255 : 0, 127 : 0, 63 : 0, 31 : 0, 15 : 0, 7 : 0\n\n"
	);
	buffer = requestBuffer(400);
	fprintf(file, "\tAddress: %d\n", buffer);
	debugBuffers(file);

	// Return 10 511 word buffers
	fprintf(file, "Return 10 511 word buffers.\n"
		"Expected values:\n"
		"\tMemory status: OK\n\tCount of buffer sizes:\n\t511 : 10, 255 : 0, 127 : 0, 63 : 0, 31 : 0, 15 : 0, 7 : 0\n\n"
	);
	for (i = 0; i < 10; i++) {
		returnBuffer(buffers[i]);
	}
	debugBuffers(file);
	// Request 19 255 word buffers
	fprintf(file, "Request 19 255 word buffers.\n"
		"Expected values:\n"
		"\tMemory status: tight\n\tCount of buffer sizes:\n\t511 : 0, 255 : 1, 127 : 0, 63 : 0, 31 : 0, 15 : 0, 7 : 0\n\n"
	);
	buffers = realloc(buffers, sizeof(ControlBlock *) * 19);
	for (i = 0; i < 19; i++) {
		buffers[i] = requestBuffer(255);
		fprintf(file, "\tAddress: %p\n", buffers[i]);
	}
	debugBuffers(file);

	// Request extra buffer
	fprintf(file, "Request extra 511 word buffer.\n"
		"Expected values:\n"
		"\tAddress: -1\n"
		"\tMemory status: tight\n\tCount of buffer sizes:\n\t511 : 0, 255 : 1, 127 : 0, 63 : 0, 31 : 0, 15 : 0, 7 : 0\n\n"
	);
	buffer = requestBuffer(400);
	fprintf(file, "\tAddress: %d\n", buffer);
	debugBuffers(file);

	// Return 19 255 word buffers
	fprintf(file, "Return 19 255 word buffers.\n"
		"Expected values:\n"
		"\tMemory status: OK\n\tCount of buffer sizes:\n\t511 : 10, 255 : 0, 127 : 0, 63 : 0, 31 : 0, 15 : 0, 7 : 0\n\n"
	);
	for (i = 0; i < 19; i++) {
		returnBuffer(buffers[i]);
	}
	debugBuffers(file);

	// Mix of sizes
	fprintf(file, "Request 5 7 word, 2 255 word, 2 127 word, 7 511 word buffers.\n"
		"Expected values:\n"
		"\tMemory status: tight\n\tCount of buffer sizes:\n\t511 : 1, 255 : 0, 127 : 1, 63 : 1, 31 : 0, 15 : 1, 7 : 1\n\n"
	);
	buffers = realloc(buffers, sizeof(ControlBlock *) * 16);
	for (i = 0; i < 5; i++) {
		buffers[i] = requestBuffer(7);
		fprintf(file, "\tAddress: %p\n", buffers[i]);
	}
	for ( ; i < 7; i++) {
		buffers[i] = requestBuffer(255);
		fprintf(file, "\tAddress: %p\n", buffers[i]);
	}
	for ( ; i < 9; i++) {
		buffers[i] = requestBuffer(127);
		fprintf(file, "\tAddress: %p\n", buffers[i]);
	}
	for ( ; i < 16; i++) {
		buffers[i] = requestBuffer(511);
		fprintf(file, "\tAddress: %p\n", buffers[i]);
	}
	debugBuffers(file);

	fprintf(file, "Return 5 7 word, 2 255 word, 2 127 word, 7 510 word buffers in random order.\n\n"
		"Return 5th 7 word buffer\n"
	);
	returnBuffer(buffers[4]);
	debugBuffers(file);

	fprintf(file, "Return 1st 127 word buffer\n");
	returnBuffer(buffers[7]);
	debugBuffers(file);

	fprintf(file, "Return 1st 511 word buffer\n");
	returnBuffer(buffers[9]);
	debugBuffers(file);

	fprintf(file, "Return 2nd 7 word buffer\n");
	returnBuffer(buffers[1]);
	debugBuffers(file);

	fprintf(file, "Return last through 3rd 511 word buffers\n");
	for (i = 15; i >= 11; i--) {
		returnBuffer(buffers[i]);
		debugBuffers(file);
	}

	fprintf(file, "Return 2nd 127 word buffer\n");
	returnBuffer(buffers[8]);
	debugBuffers(file);

	fprintf(file, "Return 3rd 7 word buffer\n");
	returnBuffer(buffers[2]);
	debugBuffers(file);

	fprintf(file, "Return 2nd 255 word buffer\n");
	returnBuffer(buffers[6]);
	debugBuffers(file);

	fprintf(file, "Return 2nd 511 word buffer\n");
	returnBuffer(buffers[10]);
	debugBuffers(file);

	fprintf(file, "Return 1st 7 word buffer\n");
	returnBuffer(buffers[0]);
	debugBuffers(file);

	fprintf(file, "Return 2nd 127 word buffer (duplicate: shouldn't change anything)\n");
	returnBuffer(buffers[8]);
	debugBuffers(file);

	fprintf(file, "Return 4th 7 word buffer\n");
	returnBuffer(buffers[3]);
	debugBuffers(file);

	fprintf(file, "Return 1st 255 word buffer\n");
	returnBuffer(buffers[5]);
	debugBuffers(file);

	// Request 16 7 word buffers
	fprintf(file, "Request 16 7 word buffers.\n"
		"Expect to see alternation between 1 and 0 at size 7\n"
		"Expect to see 2 1s followed by 2 os at size 15\n"
		"Expect to see amount of 1s and 0s in a row increase by one with each size\n"
	);
	for (i = 0; i < 16; i++) {
		buffers[i] = requestBuffer(7);
		debugBuffers(file);
	}
	// Return 16 7 word buffers
	fprintf(file, "Return 16 7 word buffers.\n"
		"Expected values:\n"
		"\tMemory status: OK\n\tCount of buffer sizes:\n\t511 : 10, 255 : 0, 127 : 0, 63 : 0, 31 : 0, 15 : 0, 7 : 0\n\n"
	);
	for (i = 0; i < 16; i++) {
		returnBuffer(buffers[i]);
	}
	debugBuffers(file);

	// Request 16 random sized buffers
	srand(time(0));

	fprintf(file, "Request 16 random sized buffers.\n");
	for (i = 0; i < 16; i++) {
		// Use 350 to keep largest size from having much larger probability
		buffers[i] = requestBuffer(rand() % 350 + 1);
		fprintf(file, "\tAddress: %p\n", buffers[i]);
		debugBuffers(file);
	}

	// Return 16 random word buffers
	fprintf(file, "Return 16 random sized buffers.\n"
		"Expected values:\n"
		"\tMemory status: OK\n\tCount of buffer sizes:\n\t511 : 10, 255 : 0, 127 : 0, 63 : 0, 31 : 0, 15 : 0, 7 : 0\n\n"
	);
	for (i = 0; i < 16; i++) {
		returnBuffer(buffers[i]);
	}
	debugBuffers(file);


	// free memory
	free(buffers);
	free(buddy_buffer_pointer);

	// Close file
	if (fclose(file) != 0) {
		fprintf(stderr, "Error closing file\n");
	}

	return 0;
}


/* Function Bodies */

// Initializes buddy buffer memory block and returns pointer to start of block
static ControlBlock * createBuddyBuffers() {

	// allocate space for all buffers
	ControlBlock * buffer = calloc(MAX_SIZE * MAX_COUNT, sizeof(ControlBlock));

	// Initialize 10 buffers of size 7 (511 words)
	int i;
	for (i = 0; i < MAX_COUNT; i++) {
		buffer->size = 7;
		buffer->available = true;
		buffer->top = true;
		buffer += MAX_SIZE;
	}

	// reset pointer
	return buffer -= MAX_SIZE * MAX_COUNT;
}


// Gets compact size representation based on amount of words requested
static unsigned short int getBufferSize(
	unsigned short int actual_size, unsigned short int size, unsigned short int max
) {
	// Return largest size requested size fits in
	// >= max used in place of > (max - 1)
	if (actual_size >= max / 2) {
		return size;
	}
	// End recursion with smallest size
	if (actual_size < MIN_SIZE) {
		return 1;
	}
	// Check next smallest size;
	return getBufferSize(actual_size, size - 1, max / 2);
}

// Gets offset to next control word based on compact size
static int getOffset(unsigned short int size) {

	switch (size) {
	case 7: return MAX_SIZE;
	case 6: return SIZE_6;
	case 5: return SIZE_5;
	case 4: return SIZE_4;
	case 3: return SIZE_3;
	case 2: return SIZE_2;
	case 1: return MIN_SIZE;
	default: return -1;
	}
}

// Splits block into smaller blocks until block size == request size
static ControlBlock * splitBuffer(unsigned short int request_size, ControlBlock * buffer) {
	unsigned short int buffer_size = buffer->size;

	// prevent infinite recursion if error
	if (buffer_size == 0) {
		return (ControlBlock *) -1;
	}
	// bottom of recursion
	if (buffer_size == request_size) {
		return buffer;
	}
	// splits buffer
	else {
		// decrease size of buffer / buffer is top of new pair
		buffer_size--;
		buffer->size = buffer_size;
		buffer->top = true;

		// create buddy buffer
		int buddy_offset = getOffset(buffer_size);
		ControlBlock * buddy = buffer + buddy_offset;

		// set buddy buffer initial values
		buddy->size = buffer_size;
		buddy->available = true;
		buddy->top = false;

		// Check if smaller buffer now fits
		return splitBuffer(request_size, buffer);
	}
}

// Returns true if memory is OK, false if memory is tight
bool bufferPoolStatus() {
	return max_size_count > 2;
}

// Returns a buffer of a specified size if possible
// Returns -2 for an invalid request, -1 if no memory available
void * requestBuffer(unsigned short int size) {

	// initialize buffers if not already initialized
	if (buddy_buffer_pointer == NULL) {
		buddy_buffer_pointer = createBuddyBuffers();
	}
	// check for illegal sizes/no check for < 0 because unsigned int
	if (size > MAX_SIZE - 1) {
		//fprintf(stderr, "Illegal Size Requested\n");
		return (void *) -2;
	}
	// sizes
	unsigned short int request_size = getBufferSize(size, 7, MAX_SIZE);
	unsigned short int current_size;

	// Greater than max size to start compares
	unsigned short int min_size = 8;
	bool available;

	// pointer offsets
	int request_offset = getOffset(request_size);

	// ControlBlock pointers
	ControlBlock * current_block = buddy_buffer_pointer;
	ControlBlock * end_block = buddy_buffer_pointer + MAX_SIZE * MAX_COUNT;
	ControlBlock * min_block = NULL;

	// Search buffer space
	while (current_block < end_block) {

		available = current_block->available;
		current_size = current_block->size;

		if (available && request_size == current_size) {
			// reduce count of available max sized buffers
			if (request_size == 7) {
				max_size_count--;
			}
			// warn user if less than 2 max sized buffers
			if (max_size_count <= 2) {
				//fprintf(stderr, "Memory is tight\n");
			}
			// Return 1st block after control word and set unavailable
			current_block->available = false;
			return ++current_block;
		}
		// If block too small; advance by size of requested block
		if (request_size > current_size) {
			// Move pointer to next control word
			current_block += request_offset;
		}
		// If block too large
		else {
			// if available split block
			if (available) {
				if (current_size == 7) {
					max_size_count--;
				}
				current_block = splitBuffer(request_size, current_block);
				// Return 1st block after control word and set unavailable
				current_block->available = false;
				return ++current_block;
			}
			// Move pointer to next control word
			current_block += getOffset(current_size);
		}
	}
	// If no blocks available

	//fprintf(stderr, "No buffer available\n");
	return (void *)-1;

}

// Gives back a buffer to the buddybuffer block
// Combines equal-sized available buddies into larger blocks
void returnBuffer(void * return_pointer) {

	// Make sure control block in valid range
	if (return_pointer < buddy_buffer_pointer || return_pointer > buddy_buffer_pointer + MAX_SIZE * MAX_COUNT) {
		return;
	}

	// Move incoming pointer to control block
	ControlBlock * buffer = (ControlBlock *) return_pointer;
	buffer--;

	// Assume buffer is available for now
	buffer->available = true;
	unsigned short int buffer_size = buffer->size;

	// If max size buffer simply set available
	if (buffer_size == 7) {
		max_size_count++;
		return;
	}
	// Merge higher buffers if necessary
	merge(buffer);
}

// Combines buffer wih equal sized buddy
// Continues to combine if new buffer is equal sized to its buddy
static void merge(ControlBlock * buffer) {

	unsigned short int buffer_size = buffer->size;

	// If max size buffer simply set available
	if (buffer_size == 7) {
		max_size_count++;
		return;
	}

	// If not check for merging opportunities
	unsigned short int buddy_offset;
	ControlBlock * buddy;

	buddy_offset = getOffset(buffer_size);

	// Calculate if buffer is top of its pair
	if ((buffer - buddy_buffer_pointer) / buddy_offset % 2) {
		// if (index of buffer / buffer size) is odd, buffer is bottom
		buffer->top = false;
		buddy = buffer - buddy_offset;
		if (buddy->available && buddy->size == buffer_size) {
			// Make buffer unavailable
			buffer->available = false;
			buffer->top = false;

			// Increase size of buddy and check for higher level merges
			buddy->top = true;
			buddy->size++;
			merge(buddy);
		}
	}
	else {
		// else buffer is top
		buffer->top = true;
		buddy = buffer + buddy_offset;
		if (buddy->available && buddy->size == buffer_size) {
			// Make buddy unavailable
			buddy->available = false;
			buddy->top = false;

			// Increase buffer size and check for higher level merges
			buffer->size++;
			merge(buffer);
		}
	}
}

// Prints status of buddy buffer block to file
// Prints memory status and number of blocks of each size;
int debugBuffers(FILE * file) {

	unsigned short int buffer_size;
	int offset;
	bool status;

	int count_7 = 0;
	int count_6 = 0;
	int count_5 = 0;
	int count_4 = 0;
	int count_3 = 0;
	int count_2 = 0;
	int count_1 = 0;


	// Make sure buddy buffers exist
	if (buddy_buffer_pointer == NULL) {
		//fprintf(stderr, "Buffers not initialized\n");
		return -1;
	}

	// Get status of available memory
	status = bufferPoolStatus();

	ControlBlock * current_block = buddy_buffer_pointer;
	ControlBlock * end_block = buddy_buffer_pointer + MAX_SIZE * MAX_COUNT;

	// Count amount of each sized block
	while (current_block < end_block) {

		buffer_size = current_block->size;

		if (current_block->available) {

			switch (buffer_size) {
			case 7:
				count_7++;
				break;
			case 6:
				count_6++;
				break;
			case 5:
				count_5++;
				break;
			case 4:
				count_4++;
				break;
			case 3:
				count_3++;
				break;
			case 2:
				count_2++;
				break;
			case 1:
				count_1++;
				break;
			default:;
			}
		}

		// Move to next buffer
		offset = getOffset(buffer_size);
		current_block += offset;
	}
	/*printf(
		"\tMemory status: %s\n\tCount of buffer sizes:\n\t511 : %d, 255 : %d, 127 : %d, 63 : %d, 31 : %d, 15 : %d, 7 : %d\n\n",
		status ? "OK" : "tight", count_7, count_6, count_5, count_4, count_3, count_2, count_1
	);*/

	// Print debug information
	fprintf(
		file,
		"\tMemory status: %s\n\tCount of buffer sizes:\n\t511 : %d, 255 : %d, 127 : %d, 63 : %d, 31 : %d, 15 : %d, 7 : %d\n\n",
		status ? "OK" : "tight", count_7, count_6, count_5, count_4, count_3, count_2, count_1
	);

	return 0;
}