#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NUM_PAGES 256
#define NUM_FRAMES 128
#define PAGE_SIZE 256
#define MAX_ADDRESS 65535
#define LINE_SIZE 64
#define BACKING_FILE "BACKING_STORE.bin"
#define OUTPUT_FILE "correct.txt"

static int page_table[NUM_PAGES];
static signed char physical_memory[NUM_FRAMES][PAGE_SIZE];
static int next_frame = 0;
static int page_faults = 0;
static int translated = 0;
static int frame_page[NUM_FRAMES];
static int fifo_next = 0;



static void init_page_table(void){
    for(int i = 0; i < NUM_PAGES; i++){
        page_table[i] = -1;
    }
}


static void init_frames(void){
    for(int i = 0; i < NUM_FRAMES; i++){
        frame_page[i] = -1;
    }
}


static int parse_address(const char *line, int *address){
    char *end;
    long value;

    while(isspace((unsigned char)*line)){
        line++;
    }

    if(!isdigit((unsigned char)*line)){
        return 0;
    }

    value = strtol(line, &end, 10);

    while(isspace((unsigned char)*end)){
        end++;
    }

    if(*end != '\0'){
        return 0;
    }

    if(value < 0 || value > MAX_ADDRESS){
        return 0;
    }

    *address = (int)value;

    return 1;
}


static int choose_frame(void){
    int frame;

    if(next_frame < NUM_FRAMES){
        return next_frame++;
    }

    frame = fifo_next;
    fifo_next = (fifo_next + 1) % NUM_FRAMES;

    return frame;
}


static int load_page(FILE *backing, int page){
    int frame = choose_frame();

    if(fseek(backing, (long)page * PAGE_SIZE, SEEK_SET) != 0){
        return -1;
    }

    if(fread(physical_memory[frame], 1, PAGE_SIZE, backing) != PAGE_SIZE){
        return -1;
    }

    if(frame_page[frame] != -1){
        page_table[frame_page[frame]] = -1;
    }

    page_table[page] = frame;
    frame_page[frame] = page;
    page_faults++;

    return frame;
}


static int translate(FILE *backing, int address, int *physical, int *value){
    int page = address >> 8;
    int offset = address & 0xFF;
    int frame = page_table[page];

    if(frame == -1){
        frame = load_page(backing, page);

        if(frame == -1){
            return -1;
        }
    }

    *physical = frame * PAGE_SIZE + offset;
    *value = physical_memory[frame][offset];

    return 0;
}





int main(int argc, char *argv[]){
    FILE *input, *backing, *output;

    char line[LINE_SIZE];
    int address, physical, value;
    int line_number = 0;

    if(argc != 2){
        fprintf(stderr, "Uso: %s <arquivo_de_enderecos>\n", argv[0]);
        return 1;
    }

    input = fopen(argv[1], "r");

    if(input == NULL){
        fprintf(stderr, "Erro: nao foi possivel abrir o arquivo '%s'\n", argv[1]);
        return 1;
    }

    backing = fopen(BACKING_FILE, "rb");

    if(backing == NULL){
        fprintf(stderr, "Erro: nao foi possivel abrir '%s'\n", BACKING_FILE);

        fclose(input);
        return 1;
    }

    output = fopen(OUTPUT_FILE, "w");

    if(output == NULL){
        fprintf(stderr, "Erro: nao foi possivel criar '%s'\n", OUTPUT_FILE);

        fclose(input);
        fclose(backing);
        return 1;
    }

    init_page_table();
    init_frames();

    while(fgets(line, sizeof(line), input) != NULL){
        line_number++;

        if(strchr(line, '\n') == NULL && !feof(input)){
            fprintf(stderr, "Erro: linha %d muito longa\n", line_number);
            goto fail;
        }

        const char *p = line;

        while(isspace((unsigned char)*p)){
            p++;
        }

        if(*p == '\0'){
            continue;
        }

        if(!parse_address(line, &address)){
            fprintf(stderr, "Erro: formato invalido na linha %d\n", line_number);
            goto fail;
        }

        if(translate(backing, address, &physical, &value) != 0){
            fprintf(stderr, "Erro: falha ao traduzir o endereco da linha %d\n", line_number);
            goto fail;
        }

        translated++;

        fprintf(output, "Virtual address: %d Physical address: %d Value: %d\n", address, physical, value);
    }

    if(ferror(input)){
        fprintf(stderr, "Erro: falha ao ler o arquivo de enderecos\n");
        goto fail;
    }

    if(translated == 0){
        fprintf(stderr, "Erro: arquivo de enderecos vazio\n");
        goto fail;
    }

    fprintf(output, "Number of Translated Addresses = %d\n", translated);
    fprintf(output, "Page Faults = %d\n", page_faults);
    fprintf(output, "Page Fault Rate = %.3f\n", (double)page_faults / translated);

    fclose(input);
    fclose(backing);
    fclose(output);
    return 0;

fail:
    fclose(input);
    fclose(backing);
    fclose(output);
    return 1;
}