#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h> 
#include "darr.h" // a simple dynamic array lib, see c_dictionary
#include "gapbuff.h"

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

#define i8 int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t

#define MIN(x, y) (x < y ? x : y)
#define MAX(x, y) (x > y ? x : y)
#define CLAMP_TOP(x, max) (x > max ? max : x)

#define GAPBUF_DEFAULT_CAPACITY 256
#define GAPBUF_DEFAULT_MIN_GAP_SIZE 16

#define DELETED_LB UINT_MAX

// todo: add msg to assert
#define assert(expr)                            \
    do {                                        \
        if (!(expr)) {                          \
            assert_failed(__FILE__, __LINE__);  \
        }                                       \
    } while (0)

void assert_failed(const char* file, int line) {
    fprintf(stderr, "Assertion failed: file %s, line %d\n", file, line);
    abort();
}

void gb_init(GapBuffer* gb) {
    gb->buffer = darr_malloc(GAPBUF_DEFAULT_CAPACITY);
    memset(gb->buffer, 0xff, GAPBUF_DEFAULT_CAPACITY); 
    gb->text_size = 0;
    gb->capacity = GAPBUF_DEFAULT_CAPACITY;
    gb->gap_start = 0;
    gb->gap_end = GAPBUF_DEFAULT_CAPACITY;
    gb->gap_size = GAPBUF_DEFAULT_CAPACITY;
    gb->cursor_pos = 0;
    gb->min_gap_size = GAPBUF_DEFAULT_MIN_GAP_SIZE;
    gb->linebreaks = NULL;
}
void gb_resize(GapBuffer* gb, u32 new_capacity) {
    if(new_capacity < gb->text_size + gb->min_gap_size)
        new_capacity = gb->text_size + gb->min_gap_size;
    gb->buffer = darr_realloc(gb->buffer, new_capacity);
    gb->capacity = new_capacity;
}

// gb_free(): frees the memory used by the gap buffer and its associated data.
void gb_free(GapBuffer* gb) {
    free(gb->buffer);
    gb->buffer = NULL;
    darr_free(gb->linebreaks);
}

// gb_insert_char(): inserts a single character into the gap buffer at the current cursor position and updates the gap and cursor positions accordingly.
void gb_insert_char(GapBuffer* gb, char c) {
    if (gb->gap_size <= gb->min_gap_size + 1) { 
        u32 start_of_text_after_gap = 0;
        u32 size_of_text_after_gap = 0;
        if (gb->gap_end < gb->capacity) {
            start_of_text_after_gap = gb->gap_end;
            size_of_text_after_gap = gb->capacity - gb->gap_end;
        }
        gb_resize(gb, gb->capacity * 2);
        gb->gap_size = gb->capacity - gb->text_size;
        gb->gap_end = gb->gap_start + gb->gap_size;
        if(size_of_text_after_gap > 0) {
            memmove(&gb->buffer[gb->gap_end], &gb->buffer[start_of_text_after_gap], size_of_text_after_gap);
        }
    }
    if (c == '\n') {
        darr_push(gb->linebreaks, gb->cursor_pos);
    }
    gb->buffer[gb->gap_start] = c;
    gb->gap_start++;
    gb->text_size++;
    gb->gap_size--;
    gb->cursor_pos++;
}

// gb_insert_str(): inserts a string of characters into the gap buffer at the current cursor position and updates the gap and cursor positions accordingly.
void gb_insert_str(GapBuffer* gb, char* str) {
    u32 str_len = (u32)strlen(str);
    if (str_len == 0) return;
    u32 linebreak_pos = 0;
    for (u32 i = 0; i < str_len; i++) {
        if (str[i] == '\n') {
            darr_push(gb->linebreaks, gb->gap_start + i + linebreak_pos);
            linebreak_pos++;
        }
    }
    if (gb->gap_size <= gb->min_gap_size + str_len) { 
        u32 start_of_text_after_gap = 0;
        u32 size_of_text_after_gap = 0;
        if (gb->gap_end < gb->capacity) {
            start_of_text_after_gap = gb->gap_end;
            size_of_text_after_gap = gb->capacity - gb->gap_end;
        }
        gb_resize(gb, gb->capacity + str_len + gb->min_gap_size * 2);
        gb->gap_size = gb->capacity - gb->text_size;
        gb->gap_end = gb->gap_start + gb->gap_size;
        if(size_of_text_after_gap > 0) {
            memmove(&gb->buffer[gb->gap_end], &gb->buffer[start_of_text_after_gap], size_of_text_after_gap);
        }
    }
    // insert the new text at gb->gap_start
    memmove(&gb->buffer[gb->gap_start], str, str_len);
    gb->gap_start += str_len;
    gb->text_size += str_len;
    gb->gap_size -= str_len;
    gb->cursor_pos += str_len;
}

u32 gb_pos_to_virtual_pos(GapBuffer *gb, u32 pos) {
    if (pos <= gb->gap_start) {
        return pos;
    } else {
        return pos - gb->gap_size;
    }
}
u32 gb_virtual_pos_to_pos(GapBuffer* gb, u32 virtual_pos) {
    if (virtual_pos < gb->gap_start) {
        return virtual_pos;
    } else {
        return virtual_pos + gb->gap_size;
    }
}
// gb_get_cursor_virtual_pos(): returns the virtual position of the cursor. 
// A virtual position is the position of the cursor in the text buffer, ignoring the gap.
u32 gb_get_cursor_virtual_pos(GapBuffer* gb) {
    return gb_pos_to_virtual_pos(gb, gb->cursor_pos);
}


// gb_move_cursor_to_position(): moves the cursor to a new position in the gap buffer
void gb_move_cursor(GapBuffer* gb, u32 virtual_pos) {
    if (virtual_pos >= gb->text_size) {
        gb->cursor_pos = gb_get_text_end(gb);
    } else if (virtual_pos == 0) {
        gb->cursor_pos = gb_get_text_start(gb);
    } else {
        if (virtual_pos < gb->gap_start - gb_get_text_start(gb)) {
            gb->cursor_pos = gb_get_text_start(gb) + virtual_pos;
        } else {
            gb->cursor_pos = gb->gap_end + (virtual_pos - (gb->gap_start - gb_get_text_start(gb)));
        }
    }
}
void gb_move_cursor_by_delta(GapBuffer* gb, i32 delta) {
    u32 virtual_cursor_pos = gb_pos_to_virtual_pos(gb, gb->cursor_pos);
    i32 new_virtual_pos = (i32)virtual_cursor_pos + delta;

    // Ensure that the new virtual position is within the bounds of the text
    if (new_virtual_pos < 0) {
        new_virtual_pos = 0;
    } else if (new_virtual_pos > (i32)gb->text_size) {
        new_virtual_pos = gb->text_size;
    }

    gb_move_cursor(gb, (u32)new_virtual_pos);
}

// gb_move_gap(): moves the gap in the gap buffer by a given delta
void gb_move_gap_by_delta(GapBuffer* gb, i32 delta) {
    i32 new_virtual_gap_start = (i32)gb->gap_start + delta;
    // Ensure that the new virtual gap start position is within the bounds of the text
    if (new_virtual_gap_start < 0) {
        new_virtual_gap_start = 0;
    } else if (new_virtual_gap_start > (i32)gb->text_size) {
        new_virtual_gap_start = gb->text_size;
    }
    gb_move_gap(gb, (u32)new_virtual_gap_start);
}


// gb_move_gap(): moves the gap in the gap buffer to a given position
void gb_move_gap(GapBuffer* gb, u32 virtual_pos) {
    if (virtual_pos == gb->gap_start) {
        return;
    }
    if (virtual_pos > gb->gap_start) {
        u32 delta = virtual_pos - gb->gap_start;
        memmove(&gb->buffer[gb->gap_start], &gb->buffer[gb->gap_end], delta);
        gb->gap_start += delta;
        gb->gap_end += delta;
    } else {
        u32 delta = gb->gap_start - virtual_pos;
        memmove(&gb->buffer[gb->gap_end - delta], &gb->buffer[virtual_pos], delta);
        gb->gap_start -= delta;
        gb->gap_end -= delta;
    }
}
void gb_move_gap_to_cursor(GapBuffer* gb) {
    u32 cursor_virtual_pos = gb_pos_to_virtual_pos(gb, gb->cursor_pos);
    gb_move_gap(gb, cursor_virtual_pos);
}

u32 gb_get_text_end(GapBuffer* gb) {
    return gb->gap_start < gb->text_size ? gb->gap_size + gb->text_size : gb->text_size;
}

// gb_delete_char(): deletes the character immediately before the cursor and updates the gap and cursor positions accordingly.
void gb_delete_char(GapBuffer* gb) { // backspace key
    if (gb->cursor_pos <= gb_get_text_start(gb)) {
        return;
    }
    if(gb->buffer[gb->cursor_pos - 1] == '\n') {
        assert(gb->linebreaks != NULL);
        u32 lb_idx = 0;
        while (lb_idx < (u32)darr_len(gb->linebreaks) ) { // test this code path TODO
            if (gb->linebreaks[lb_idx] == DELETED_LB) {
                lb_idx++;
            } else if (gb->linebreaks[lb_idx] < gb->cursor_pos) {
                lb_idx++;
            } else {
                break;
            }
        }
        lb_idx--;
        // delete the line break from the linebreaks array
        gb->linebreaks[lb_idx] = DELETED_LB; 
    }
    gb->buffer[--gb->gap_start] = '\0';
    gb->gap_size++;
    gb->text_size--;
    gb->cursor_pos--;
}
u32 gb_get_text_start(GapBuffer* gb) {
    return gb->gap_start == 0 ? gb->gap_end : 0;
}
// gb_delete_backward(): deletes the character immediately after the cursor and updates the gap and cursor positions accordingly.
void gb_delete_char_after_cursor(GapBuffer* gb) { // delete key was pressed
    if (gb->cursor_pos >= gb_get_text_end(gb)) { 
        return;
    }
    if (gb->gap_end < gb_get_text_end(gb)) { // todo: test this code path, and make sure DELETED_LB is handled correctly
        if(gb->buffer[gb->gap_end] == '\n') {
            assert(gb->linebreaks != NULL);
            u32 lb_idx = 0;

            while (lb_idx < (u32)darr_len(gb->linebreaks) ) { // test this code path TODO
                if (gb->linebreaks[lb_idx] == DELETED_LB) {
                    lb_idx++;
                } else if (gb->linebreaks[lb_idx] < gb->cursor_pos) {
                    lb_idx++;
                } else {
                    break;
                }
            }
            // mark the line break as deleted in the linebreaks array
            gb->linebreaks[lb_idx] = DELETED_LB;
        }
        gb->buffer[gb->gap_end++] = '\0';
        gb->gap_size++;
        gb->text_size--;
    }
}

// gb_get_text(): returns a string of all the characters in the gap buffer, excluding the gap. 
char* gb_get_text(GapBuffer* gb) {
    if(gb->text_size == 0) {
        return "";
    }
    char *text = darr_malloc(gb->text_size + 1);
    if (gb->gap_start < gb_get_text_start(gb)) {
        memmove(text, &gb->buffer[gb->gap_end], gb_get_text_end(gb) - gb->gap_end);
        text[gb->text_size] = '\0';
        return text;
    }
    if (gb->gap_start < gb_get_text_end(gb) && gb->gap_start > gb_get_text_start(gb)) {
        memmove(text, &gb->buffer[gb_get_text_start(gb)], gb->gap_start);
        memmove(&text[gb->gap_start], &gb->buffer[gb->gap_end], gb_get_text_end(gb) - gb->gap_end);
    } else {
        memmove(text, &gb->buffer[gb_get_text_start(gb)], gb_get_text_end(gb));
    }
    text[gb->text_size] = '\0';
    return text;
}

// gb_get_line_count(): returns the number of lines in the gap buffer. 
u32 gb_get_line_count(GapBuffer* gb) {
    return darr_len(gb->linebreaks) + 1;
}

// gb_get_line_length() calculates the length of a specific line in the GapBuffer given its line number. 
u32 gb_get_line_length(GapBuffer* gb, u32 line_number) {
    // does a '\n' count as a character? Not for rendering purposes.
    // todo: I need to parse '\t' as 4 spaces, and look at other special chars. 
    if (line_number > (u32)darr_len(gb->linebreaks)) { // line does not exist so return 0
        return 0; 
    }
    if (line_number == 0) {
        // Calculate length of the first line
        if(darr_len(gb->linebreaks) == 0) {
            return gb_get_text_end(gb) - gb_get_text_start(gb);
        } else {
            return gb->linebreaks[0] - gb_get_text_start(gb);
        }
    } else if (line_number < (u32)darr_len(gb->linebreaks)) {
        // Calculate length of an intermediate line
        return gb->linebreaks[line_number] - gb->linebreaks[line_number - 1] - 1;
    } else {
        // Calculate length of the last line
        return gb_get_text_end(gb) - gb->linebreaks[line_number - 1] - 1;
    }
}
u32 gb_get_line_start_pos(GapBuffer* gb, u32 line_number) {
    if (line_number == 0) {
        return gb_get_text_start(gb);
    } else if (line_number <= (u32)darr_len(gb->linebreaks)) {
        return gb_virtual_pos_to_pos(gb, gb->linebreaks[line_number - 1] + 1);
    } else {
        return (u32)-1;
    }
}


// get_char_at(): returns the character at a specific position in the gap buffer. 
// todo
