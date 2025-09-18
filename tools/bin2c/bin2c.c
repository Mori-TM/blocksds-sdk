// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: Antonio Niño Díaz, 2014, 2019-2020, 2023-2024

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PATH_LEN    2048

char out_array_name[MAX_PATH_LEN];
char c_file_name[MAX_PATH_LEN];
char h_file_name[MAX_PATH_LEN];

void file_load(const char *path, void **buffer, size_t *size)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL)
    {
        fprintf(stderr, "Can't open %s for reading\n", path);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    *size = ftell(f);

    if (*size == 0)
    {
        fprintf(stderr, "%s is an empty file\n", path);
        fclose(f);
        exit(1);
    }

    rewind(f);
    *buffer = malloc(*size);
    if (*buffer == NULL)
    {
        fprintf(stderr, "Out of memory trying to load %s\n", path);
        fclose(f);
        exit(1);
    }

    if (fread(*buffer, *size, 1, f) != 1)
    {
        fprintf(stderr, "Error while reading file %s\n", path);
        fclose(f);
        exit(1);
    }

    fclose(f);
}

// Converts a file name to an array name and output file names
void generate_transformed_name(const char *_path, const char *dir_out,
                               bool save_ext)
{
    char *path = strdup(_path);
    if (path == NULL)
    {
        fprintf(stderr, "Can't allocate memory");
        exit(1);
    }

    int len = strlen(path);

    int start = 0;

    for (int i = len - 1; i > 0; i--)
    {
        if (path[i] == '/')
        {
            start = i + 1;
            break;
        }
    }

    // Remove extension from final names if requested
    if (!save_ext)
    {
        for (int i = len - 1; i > start; i--)
        {
            if (path[i] == '.')
            {
                path[i] = '\0';
                break;
            }
        }
    }

    const char *basename = &(path[start]);

    len = snprintf(c_file_name, sizeof(c_file_name), "%s/%s.c", dir_out, basename);
    if (len < 0 || len >= (int) sizeof(c_file_name))
    {
        fprintf(stderr, "Output file name too long\n");
        exit(1);
    }

    // h_file_name will be the same length as c_file_name.
    snprintf(h_file_name, sizeof(h_file_name), "%s/%s.h", dir_out, basename);

    // Fix names of header and C files and replace '.bin' by '_bin'.
    //
    //     out.path/34.my.bin.file.bin -> out.path/34.my.bin.file_bin.h
    //                                    out.path/34.my.bin.file_bin.c
    {
        for (int i = len - 3; i > 0; i--)
        {
            if (h_file_name[i] == '.')
            {
                h_file_name[i] = '_';
                c_file_name[i] = '_';
                break;
            }
            else if (h_file_name[i] == '/')
            {
                break;
            }
        }
    }

    // Create the array name. If the name begins by a digit, prefix it with a
    // '_' to form a valid identifier. Also, convert characters which are not
    // valid C identifier characters underscores.
    //
    //     my-bin-file.bin -> my_bin_file_bin[]
    //     34.my.bin.file.bin -> _34_my_bin_file_bin[]
    //     my-fancy-file%.bin -> my_fancy_file__bin[]
    {
        const char *prefix = "";
        if (isdigit(path[start]))
            prefix = "_";

        len = snprintf(out_array_name, sizeof(out_array_name), "%s%s",
               prefix, &(path[start]));
        if (len < 0 || len >= (int) sizeof(out_array_name))
        {
            fprintf(stderr, "Output array name too long\n");
            exit(1);
        }

        for (int i = 0; i < len; i++)
        {
            if (!isalnum(out_array_name[i]))
                out_array_name[i] = '_';
        }
    }

    free(path);
}

void print_help(const char *path)
{
    fprintf(stderr, "Invalid arguments.\n"
                    "\n"
                    "Usage:\n"
                    "    %s [--noext] file_in folder_out\n"
                    "\n"
                    "Options:\n"
                    "    --noext  Remove original extension from output\n"
                    "    -V       Print version string and exit\n",
                    path);

    exit(EXIT_FAILURE);
}

void print_version(void)
{
    printf("bin2c " VERSION_STRING "\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    void *file = NULL;
    size_t size;

    if (argc < 2)
        print_help(argv[0]);

    if ((argc == 2) && (strcmp(argv[1], "-V") == 0))
        print_version();

    bool save_ext;
    const char *path_in;
    const char *dir_out;

    if (strcmp(argv[1], "--noext") == 0)
    {
        save_ext = false;

        if (argc < 4)
            print_help(argv[0]);

        path_in = argv[2];
        dir_out = argv[3];
    }
    else
    {
        save_ext = true;

        if (argc < 3)
            print_help(argv[0]);

        path_in = argv[1];
        dir_out = argv[2];
    }

    file_load(path_in, &file, &size);

    generate_transformed_name(path_in, dir_out, save_ext);

    FILE *fc = fopen(c_file_name, "w");
    if (fc == NULL)
    {
        fprintf(stderr, "Can't open %s for writing\n", c_file_name);
        return 1;
    }

    const char *c_header =
        "// Autogenerated file. Do not edit.\n"
        "\n"
        "#include <stdint.h>\n"
        "\n";

    fprintf(fc, "%s", c_header);
    fprintf(fc, "const uint8_t %s[%zu] __attribute__((aligned(4)))  =\n",
            out_array_name, size);
    fprintf(fc, "{\n");

    uint8_t *data = file;

    for (size_t i = 0; i < size; i++)
    {
        if ((i % 12) == 0)
            fprintf(fc, "    ");

        fprintf(fc, "0x%02X", *data);
        data++;

        if (i == size - 1)
            fprintf(fc, "\n");
        else if ((i % 12) == 11)
            fprintf(fc, ",\n");
        else
            fprintf(fc, ", ");
    }

    fprintf(fc, "};\n");

    fclose(fc);

    FILE *fh = fopen(h_file_name, "w");
    if (fh == NULL)
    {
        fprintf(stderr, "Can't open %s for writing\n", h_file_name);
        return 1;
    }

    fprintf(fh,
        "// Autogenerated file. Do not edit.\n"
        "\n"
        "#pragma once\n"
        "\n"
        "#include <stdint.h>\n"
        "\n"
        "#define %s_size (%zu)\n"
        "extern const uint8_t %s[%zu];\n",
        out_array_name, size,
        out_array_name, size);

    fclose(fh);

    free(file);

    return 0;
}
