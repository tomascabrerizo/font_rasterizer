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

// NOTE(tomi): Tags to find the different types of tables
#define TAG(a, b, c, d) ((a) << 24 | (b) << 16 | (c) << 8 | (d) << 0)
#define CMAP_TAG TAG('c', 'm', 'a', 'p')

// NOTE(tomi): Font Directory code
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

void load_font_directory(char *start, FontDirectory *font_dir)
{
    OffsetSubtable *offset_sub = &font_dir->offset_sub;
    offset_sub->scaler_type = GET_32_MOVE(start);
    offset_sub->num_tables = GET_16_MOVE(start);
    offset_sub->search_range = GET_16_MOVE(start);
    offset_sub->entry_selector = GET_16_MOVE(start);
    offset_sub->range_shift = GET_16_MOVE(start);

    font_dir->table_dir = malloc(offset_sub->num_tables*sizeof(TableDirectory));

    for(i32 i = 0; i < offset_sub->num_tables; ++i)
    {
        TableDirectory *table_dir = font_dir->table_dir + i;
        table_dir->tag = GET_32_MOVE(start);
        table_dir->check_sum = GET_32_MOVE(start);
        table_dir->offset = GET_32_MOVE(start);
        table_dir->length = GET_32_MOVE(start);
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

// NOTE(tomi): CMap table code
typedef struct
{
    u16 platform_id;
    u16 platform_specific_id;
    u32 offset;
} Subtable;

typedef struct
{
    u16 version;
    u16 num_subtables;
    Subtable *subtables;

} CMap;

CMap get_cmap_table(char *start, FontDirectory font_dir)
{
    CMap result = {0};
    for(int i = 0; i < font_dir.offset_sub.num_tables; ++i)
    {
        TableDirectory *table_dir = font_dir.table_dir + i;
        if(table_dir->tag == CMAP_TAG)
        {
            char *cmap_start = start + table_dir->offset;
            char *cmap_data = start + table_dir->offset;
            result.version = GET_16_MOVE(cmap_data);
            result.num_subtables = GET_16_MOVE(cmap_data);

            result.subtables = malloc(result.num_subtables*sizeof(Subtable));

            for(int i = 0; i < result.num_subtables; ++i)
            {
                Subtable *subtable = result.subtables + i;
                subtable->platform_id = GET_16_MOVE(cmap_data);
                subtable->platform_specific_id = GET_16_MOVE(cmap_data);
                subtable->offset = GET_32_MOVE(cmap_data);

                fprintf(stdout, "-----------------------\n");
                fprintf(stdout, "Offset: %d\n", subtable->offset);
                fprintf(stdout, "Format: %d\n", GET_16(cmap_start + subtable->offset));
            }

            return result;
        }
    }
    return result;
}

void print_cmap_table(CMap cmap)
{
    fprintf(stdout, "-----------------------\n");
    fprintf(stdout, "Version: %d\n", cmap.version);
    fprintf(stdout, "Number of subtables: %d\n", cmap.num_subtables);
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
        CMap cmap = get_cmap_table(start, font_dir);
        print_cmap_table(cmap);
    }
    
    return 0;
}
