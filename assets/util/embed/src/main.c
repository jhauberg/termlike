#include <stdio.h> // printf, f*, FILE, SEEK_END, SEEK_SET
#include <string.h> // strcmp
#include <stdint.h> // uint8_t, uint32_t, int32_t, int64_t
#include <stddef.h> // NULL

static int32_t embed(FILE * in, FILE * out);

int32_t
main(int32_t const argc, char ** const argv)
{
    if (argc < 3) {
        if (argc == 2 &&
            (strcmp(argv[1], "--help") == 0 ||
             strcmp(argv[1], "-h") == 0)) {
            printf("Convert binary file to embeddable C\n\n");
        }
        
        printf("Usage:\n"
               "  embed <file> <output>\n"
               "  embed -h | --help\n");
        
        return -1;
    }
    
    char * const input_filename = argv[1];
    char * const output_filename = argv[2];
    
    FILE * const in = fopen(input_filename, "rb");
    
    if (in == NULL) {
        printf("Could not open file '%s'\n", input_filename);
        
        return -1;
    }
    
    FILE * const out = fopen(output_filename, "wb");
    
    if (out == NULL) {
        printf("Could not create output file '%s'\n", output_filename);
        
        return -1;
    }
    
    embed(in, out);
    
    if (fclose(in) != 0) {
        return -1;
    }
    
    if (fclose(out) != 0) {
        return -1;
    }

    return 0;
}

static
int32_t
embed(FILE * const in, FILE * const out)
{
    if (fseek(in, 0, SEEK_END) != 0) {
        return -1;
    }
    
    int64_t const length = ftell(in);
    
    if (fseek(in, 0, SEEK_SET) != 0) {
        return -1;
    }
    
    fprintf(out, "uint8_t data[%llu] = {", length);
    
    // limit horizontal entries to result in less than 80 characters per line
    uint32_t const columns = 12;
    uint32_t i = 0;
    
    uint8_t element;
    
    while (1) {
        // read next element
        size_t const elements = 1;
        
        if (fread(&element, sizeof(uint8_t), elements, in) != elements) {
            // end of file or error
            break;
        }
        
        // comma-separate each element
        if (i > 0) {
            // except the first element
            fprintf(out, ", ");
        }
        
        if (i++ % columns == 0) {
            // break to next line
            fprintf(out, "\n    ");
        }
        
        // set element
        fprintf(out, "0x%02X", element);
    }
    
    fprintf(out, "\n};\n");
    
    return 0;
}
