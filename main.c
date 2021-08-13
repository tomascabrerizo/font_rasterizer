#include <stdio.h>
#include <stdlib.h>

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef int i32;
typedef short i16; 
typedef char i8;

typedef float r32;

static char * 
read_entire_file(const char *file_path, u32 *file_size)
{
    char *result = 0;
    FILE *file = fopen(file_path, "rb"); 
    if(file)
    {
        u32 size_to_read = 0;
        fseek(file, 0, SEEK_END);
        size_to_read = ftell(file); 
        fseek(file, 0, SEEK_SET);
        if(size_to_read)
        {
            *file_size = size_to_read;
            result = malloc(size_to_read);
            fread(result, 1, size_to_read, file);
        }
        fclose(file);
    }
    return result;
}

// NOTE(tomi): This macros are use to manipulate the file contents
// ttf files are in big endian so we need to change its memory order
#define GET_16(mem) ((((u8 *)(mem))[0] << 8) | (((u8 *)(mem))[1] << 0))

#define GET_32(mem) ((((u8 *)(mem))[0] << 24) | (((u8 *)(mem))[1] << 16) | \
                     (((u8 *)(mem))[2] << 8 ) | (((u8 *)(mem))[3] << 0 ))

#define MOVE_P(mem, count) ((mem) += (count))

#define GET_16_MOVE(mem) GET_16(mem); MOVE_P(mem, 2)
#define GET_32_MOVE(mem) GET_32(mem); MOVE_P(mem, 4)

typedef struct
{
    u32 scaler_type;
    u16 num_tables;
    u16 search_range;
    u16 entry_selector;
    u16 range_shift;
} OffsetSubtable;

typedef struct
{
    u32 tag;
    u32 check_sum;
    u32 offset;
    u32 length;
} TableDirectory;

typedef struct
{
    OffsetSubtable offset_sub;
    TableDirectory *table_dir;
} FontDirectory;

void load_font_directory(char *data, FontDirectory *font_dir)
{
    OffsetSubtable *offset_sub = &font_dir->offset_sub;
    offset_sub->scaler_type = GET_32_MOVE(data);
    offset_sub->num_tables = GET_16_MOVE(data);
    offset_sub->search_range = GET_16_MOVE(data);
    offset_sub->entry_selector = GET_16_MOVE(data);
    offset_sub->range_shift = GET_16_MOVE(data);

    font_dir->table_dir = malloc(offset_sub->num_tables*sizeof(TableDirectory));

    for(i32 i = 0; i < offset_sub->num_tables; ++i)
    {
        TableDirectory *table_dir = font_dir->table_dir + i;
        table_dir->tag = GET_32_MOVE(data);
        table_dir->check_sum = GET_32_MOVE(data);
        table_dir->offset = GET_32_MOVE(data);
        table_dir->length = GET_32_MOVE(data);
    }
}

void print_font_directory(FontDirectory font_dir)
{
    fprintf(stdout, "Number of tables: %d\n", font_dir.offset_sub.num_tables);
    for(int i = 0; i < font_dir.offset_sub.num_tables; ++i)
    {
        TableDirectory *table_dir = font_dir.table_dir + i;
        u8 *tag = (u8 *)&table_dir->tag;
        fprintf(stdout, "-------------------------------\n");
        fprintf(stdout, "(Table %d)\n", i+1);
        fprintf(stdout, "-------------------------------\n");
        fprintf(stdout, "Tag: %c%c%c%c\n", tag[3], tag[2], tag[1], tag[0]);
        fprintf(stdout, "CheckSum: %u\n", table_dir->check_sum);
        fprintf(stdout, "Offset: %u\n", table_dir->offset);
        fprintf(stdout, "Length: %u\n", table_dir->length);
    }
}

int main()
{
    u32 file_size = 0;
    const char *file_content = read_entire_file("fonts/UbuntuMono-Regular.ttf", &file_size);
    if(file_content)
    {
        char *start = (char *)file_content;
        FontDirectory font_dir = {0};
        load_font_directory(start, &font_dir);
        print_font_directory(font_dir);
    }
    
    return 0;
}
