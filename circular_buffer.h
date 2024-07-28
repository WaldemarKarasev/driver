#ifndef CIRCULAR_BUFFER
#define CIRCULAR_BUFFER

struct circular_buffer
{
    char* data_;
    int size_;
    int read_index_;
    int write_index_;
};

// Creating circular buffer. Allocating memory
int create_circular_buffer(struct circular_buffer* buffer, int size);

// Destroy buffer. Free allocated memory
void destroy_circular_buffer(struct circular_buffer* buffer);

// Check if the queue is full
int is_full(struct circular_buffer* buffer);

// Check if the queue is empty
int is_empty(struct circular_buffer* buffer);

// Adding an element
int write_to_circular_buffer(struct circular_buffer* buffer, const char* src, int size_to_write);

// Removing an element
int read_from_circular_buffer(struct circular_buffer* buffer, char* dest, int size_to_read);

#endif