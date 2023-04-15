#ifndef GAPBUFF_H
#define GAPBUFF_H

typedef struct GapBuffer {
    char* buffer;        
    unsigned int text_size;       
    unsigned int capacity;        
    unsigned int gap_start;       
    unsigned int gap_end;         
    unsigned int gap_size;        
    unsigned int cursor_pos;      
    unsigned int min_gap_size;    
    unsigned int* linebreaks;    // array of indices indicating line breaks, using darr.h for dynamic array
} GapBuffer;

void assert_failed(const char* file, int line);

void gb_resize(GapBuffer* gb, unsigned int new_capacity);
void gb_insert_str(GapBuffer* gb, char* str);
void gb_insert_char(GapBuffer* gb, char c);

void gb_delete_char(GapBuffer* gb);
void gb_delete_char_after_cursor(GapBuffer* gb);

void gb_init(GapBuffer* gb);

void gb_move_gap_by_delta(GapBuffer* gb, int delta);
void gb_move_cursor_by_delta(GapBuffer* gb, int delta);
void gb_move_gap_to_cursor(GapBuffer* gb);

void gb_move_cursor(GapBuffer* gb, unsigned int virtual_pos);
void gb_move_gap(GapBuffer* gb, unsigned int virtual_pos);

unsigned int gb_virtual_pos_to_pos(GapBuffer* gb, unsigned int virtual_pos);
unsigned int gb_pos_to_virtual_pos(GapBuffer *gb, unsigned int pos);
unsigned int gb_get_cursor_virtual_pos(GapBuffer *gb);
unsigned int gb_get_text_start(GapBuffer* gb);
unsigned int gb_get_text_end(GapBuffer* gb);

char* gb_get_text(GapBuffer* gb);

unsigned int gb_get_line_length(GapBuffer* gb, unsigned int line_number);
unsigned int gb_get_line_count(GapBuffer* gb);
unsigned int gb_get_line_start_pos(GapBuffer* gb, unsigned int line_number);

void gb_free(GapBuffer* gb);

#endif // GAPBUFF_H
