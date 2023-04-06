#ifndef GAPBUFF_H
#define GAPBUFF_H

typedef struct GapBuffer {
    char* buffer;        
    unsigned int text_size;       
    unsigned int capacity;        
    unsigned int gap_start;       
    unsigned int gap_end;         
    unsigned int text_start;      
    unsigned int gap_size;        
    unsigned int cursor_pos;      
    unsigned int min_gap_size;    
    unsigned int* linebreaks;    // array of indices indicating line breaks, using darr.h for dynamic array
} GapBuffer;

void assert_failed(const char* file, int line);
unsigned int gb_get_line_length(GapBuffer* gb);
unsigned int gb_get_line_count(GapBuffer* gb);
char* gb_get_text(GapBuffer* gb);
void gb_delete_backward(GapBuffer* gb);
void gb_delete_char(GapBuffer* gb);
void gb_move_gap_to_cursor(GapBuffer* gb);
void gb_move_gap(GapBuffer* gb, int delta);
void gb_move_cursor(GapBuffer* gb, int delta);
void gb_insert_str(GapBuffer* gb, char* str);
void gb_insert_char(GapBuffer* gb, char c);
void gb_free(GapBuffer* gb);
void gb_resize(GapBuffer* gb, unsigned int new_capacity);
void gb_init(GapBuffer* gb);
unsigned int gb_get_text_end(GapBuffer* gb);

#endif // GAPBUFF_H