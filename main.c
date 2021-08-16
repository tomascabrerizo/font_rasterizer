#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    char *cmap_ptr;
} FontDirectory;

void load_font_directory(char *start, FontDirectory *font_dir)
{
    char *saved_start = start;

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

    for(int i = 0; i < offset_sub->num_tables; ++i)
    {
        TableDirectory *table_dir = font_dir->table_dir + i;
        switch(table_dir->tag)
        {
            case CMAP_TAG:
            {
                font_dir->cmap_ptr = saved_start + table_dir->offset;
            }break;
        }
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

CMap load_cmap_table(FontDirectory font_dir)
{
    CMap result = {0};
    
    char *cmap_data = font_dir.cmap_ptr;

    result.version = GET_16_MOVE(cmap_data);
    result.num_subtables = GET_16_MOVE(cmap_data);

    result.subtables = malloc(result.num_subtables*sizeof(Subtable));

    for(int i = 0; i < result.num_subtables; ++i)
    {
        Subtable *subtable = result.subtables + i;
        subtable->platform_id = GET_16_MOVE(cmap_data);
        subtable->platform_specific_id = GET_16_MOVE(cmap_data);
        subtable->offset = GET_32_MOVE(cmap_data);
    }

    return result;
}

void print_cmap_table(CMap cmap)
{
    fprintf(stdout, "-----------------------\n");
    fprintf(stdout, "           CMAP        \n");
    fprintf(stdout, "-----------------------\n");
    fprintf(stdout, "Version: %d\n", cmap.version);
    fprintf(stdout, "Number of subtables: %d\n", cmap.num_subtables);

    for(int i = 0; i < cmap.num_subtables; ++i)
    {
        Subtable *subtable = cmap.subtables + i;
        fprintf(stdout, "-----------------------\n");
        fprintf(stdout, "PlatformID: %d\n", subtable->platform_id);
        fprintf(stdout, "PlatformEspID: %d\n", subtable->platform_specific_id);
        fprintf(stdout, "Offset: %d\n", subtable->offset);
    }
}

typedef struct
{
    u16 format;
    u16 length;
    u16 language;
    u16 seg_count_x2;
    u16 search_range;
    u16 entry_selector;
    u16 range_shift;
    
    // NOTE(tomi) The format dont take into account the size of this pointers
    u16 *end_code;
    u16 reserved_pad;
    
    u16 *start_code;
    u16 *id_delta;
    u16 *id_range_offset;
    u16 *glyph_index_array;
} Format4;

Format4 load_format4(FontDirectory font_dir, CMap cmap)
{
    Format4 result = {};
    char * format_data = font_dir.cmap_ptr + cmap.subtables[0].offset; 

    result.format = GET_16_MOVE(format_data);
    result.length = GET_16_MOVE(format_data);
    result.language = GET_16_MOVE(format_data);
    result.seg_count_x2 = GET_16_MOVE(format_data);
    result.search_range = GET_16_MOVE(format_data);
    result.entry_selector = GET_16_MOVE(format_data);
    result.range_shift = GET_16_MOVE(format_data);
 
    // NOTE(tomi): Only allocates memory for the arrays not for the format struct 
    u32 format_array_size = (result.length - (sizeof(Format4) - sizeof(u16 *)*5));
    u8 *format_array = malloc(format_array_size);
    
    result.end_code = (u16 *)format_array;
    result.start_code = result.end_code + (result.seg_count_x2/2); 
    result.id_delta = result.start_code + (result.seg_count_x2/2); 
    result.id_range_offset = result.id_delta + (result.seg_count_x2/2); 
    result.glyph_index_array = result.id_range_offset + (result.seg_count_x2/2);
    
    // NOTE(tomi): We are performing this subtraction in u16 so the result have
    // to be multiply by two to get the total bytes
    int bytes_used = (result.glyph_index_array - result.end_code) * 2;
    
    // NOTE(tomi): I cannot use memcpy here becouse the file have al is data
    // in big endian so we need to switch with GET_16 or GET_32
    for(int i = 0; i < result.seg_count_x2/2; i++)
    {
        result.end_code[i] = GET_16_MOVE(format_data);
    }
    // NOTE(tomi): Jump the reserve u16 
    MOVE_P(format_data, 2);
    for(int i = 0; i < result.seg_count_x2/2; i++)
    {
        result.start_code[i] = GET_16_MOVE(format_data);
    }
    for(int i = 0; i < result.seg_count_x2/2; i++)
    {
        result.id_delta[i] = GET_16_MOVE(format_data);
    }
    for(int i = 0; i < result.seg_count_x2/2; i++)
    {
        result.id_range_offset[i] = GET_16_MOVE(format_data);
    }
     
    int bytes_left = format_array_size - bytes_used;
    for(int i = 0; i < bytes_left/2; ++i)
    {
        result.glyph_index_array[i] = GET_16_MOVE(format_data); 
    }

    return result;
}

void print_format4(Format4 format)
{
    fprintf(stdout, "-----------------------\n");
    fprintf(stdout, "         FORMAT        \n");
    fprintf(stdout, "-----------------------\n");
    fprintf(stdout, "Format: %d\n", format.format);
    fprintf(stdout, "SegCountX2: %d\n", format.seg_count_x2);

    for(int i = 0; i < format.seg_count_x2/2; i++)
    {
        fprintf(stdout, "start code: %d\t\t", format.start_code[i]);
        fprintf(stdout, "end code: %d\t\t", format.end_code[i]);
        fprintf(stdout, "id_delta: %d\t\t", format.id_delta[i]);
        fprintf(stdout, "id_range_offset: %d\n", format.id_range_offset[i]);
    }
}

u16 get_glyph_index(Format4 format, u16 char_code)
{
    i32 index = -1;
    u16 *ptr = 0;
    for(int i = 0; i < format.seg_count_x2/2; i++)
    {
        if(format.end_code[i] >= char_code)
        {
            index = i;
            break;
        }
    }

    if(index != -1)
    {
        if(format.start_code[index] <= char_code)
        {
            if(format.id_range_offset[index])
            {
                ptr = (format.id_range_offset + index) + 
                      (format.id_range_offset[index] / 2) + 
                      (char_code - format.start_code[index]);
                if(*ptr)
                {
                    return (*ptr + format.id_delta[index]);
                }
            }
            else
            {
                return (char_code + format.id_delta[index]);
            }
        }
    }

    return 0;
}

int main()
{
    u32 file_size = 0;
    const char *file_content = read_entire_file("fonts/UbuntuMono-Regular.ttf", &file_size);
    //const char *file_content = read_entire_file("fonts/Envy Code R.ttf", &file_size);
    if(file_content)
    {
        char *start = (char *)file_content;
        FontDirectory font_dir = {0};
        load_font_directory(start, &font_dir);
        CMap cmap = load_cmap_table(font_dir);
        print_cmap_table(cmap);
        Format4 format = load_format4(font_dir, cmap);
        print_format4(format);
        int glyph_index = get_glyph_index(format, 'B');
        fprintf(stdout, "glyp index: %d\n", glyph_index);
    }
    
    return 0;
}
