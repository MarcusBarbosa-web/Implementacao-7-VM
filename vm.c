#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#define NUM_PAGES 256
#define NUM_FRAMES 128
#define PAGE_SIZE 256
#define MAX_ADDRESS 65535
#define LINE_SIZE 64
#define BACKING_FILE "BACKING_STORE.bin"
#define OUTPUT_FILE "correct.txt"
#define TEMP_FILE "correct.txt.tmp"
#define ALG_FIFO 0
#define ALG_LRU 1
#define TLB_SIZE 16

static int page_table[NUM_PAGES];
static signed char physical_memory[NUM_FRAMES][PAGE_SIZE];
static int next_frame = 0;
static int page_faults = 0;
static int translated = 0;
static int frame_page[NUM_FRAMES];
static int fifo_next = 0;
static int replace_alg = ALG_FIFO;
static unsigned long frame_last_use[NUM_FRAMES];
static unsigned long clock_counter = 0;
static int tlb_page[TLB_SIZE];
static int tlb_frame[TLB_SIZE];
static int tlb_next = 0;
static int tlb_hits = 0;
static int tlb_alg = ALG_FIFO;
static unsigned long tlb_last_use[TLB_SIZE];


typedef struct node{
    int slot;
    int page;
    int found;
}TlbSearch;


static void init_page_table(void){
    for(int i = 0; i < NUM_PAGES; i++){
        page_table[i] = -1;
    }
}


static void init_tlb(void){
    for(int i = 0; i < TLB_SIZE; i++){
        tlb_page[i] = -1;
        tlb_frame[i] = -1;
        tlb_last_use[i] = 0;
    }
}


static void init_frames(void){
    for(int i = 0; i < NUM_FRAMES; i++){
        frame_page[i] = -1;
        frame_last_use[i] = 0;
    }
}


static int parse_algorithm(const char *name, int *alg){
    if(strcmp(name, "fifo") == 0){
        *alg = ALG_FIFO;
        return 1;
    }

    if(strcmp(name, "lru") == 0){
        *alg = ALG_LRU;
        return 1;
    }

    return 0;
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


static int choose_lru_frame(void){
    int victim = 0;

    for(int i = 1; i < NUM_FRAMES; i++){
        if(frame_last_use[i] < frame_last_use[victim]){
            victim = i;
        }
    }

    return victim;
}


static int choose_frame(void){
    int frame;

    if(next_frame < NUM_FRAMES){
        return next_frame++;
    }

    if(replace_alg == ALG_LRU){
        return choose_lru_frame();
    }

    frame = fifo_next;
    fifo_next = (fifo_next + 1) % NUM_FRAMES;

    return frame;
}


static void *tlb_search_entry(void *arg){
    TlbSearch *search = (TlbSearch *)arg;

    search->found = (tlb_page[search->slot] == search->page);

    return NULL;
}


static int tlb_lookup(int page){
    pthread_t threads[TLB_SIZE];
    TlbSearch searches[TLB_SIZE];
    int created[TLB_SIZE];
    int result = -1;

    for(int i = 0; i < TLB_SIZE; i++){
        searches[i].slot = i;
        searches[i].page = page;
        searches[i].found = 0;

        created[i] = (pthread_create(&threads[i], NULL, tlb_search_entry, &searches[i]) == 0);

        if(!created[i]){
            tlb_search_entry(&searches[i]);
        }
    }

    for(int i = 0; i < TLB_SIZE; i++){
        if(created[i]){
            pthread_join(threads[i], NULL);
        }

        if(searches[i].found){
            result = i;
        }
    }

    return result;
}


static int tlb_choose_lru(void){
    int victim = 0;

    for(int i = 1; i < TLB_SIZE; i++){
        if(tlb_last_use[i] < tlb_last_use[victim]){
            victim = i;
        }
    }

    return victim;
}


static int tlb_insert(int page, int frame){
    int index;

    if(tlb_alg == ALG_LRU){
        index = tlb_choose_lru();
    }else{
        index = tlb_next;
        tlb_next = (tlb_next + 1) % TLB_SIZE;
    }

    tlb_page[index] = page;
    tlb_frame[index] = frame;

    return index;
}


static void tlb_invalidate(int page){
    for(int i = 0; i < TLB_SIZE; i++){
        if(tlb_page[i] == page){
            tlb_page[i] = -1;
            tlb_frame[i] = -1;
            tlb_last_use[i] = 0;
        }
    }
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
        tlb_invalidate(frame_page[frame]);
        page_table[frame_page[frame]] = -1;
    }

    page_table[page] = frame;
    frame_page[frame] = page;
    page_faults++;

    return frame;
}


static int translate(FILE *backing, int address, int *tlb_index, int *physical, int *value){
    int page = address >> 8;
    int offset = address & 0xFF;
    int index = tlb_lookup(page);
    int frame;

    if(index != -1){
        tlb_hits++;
        frame = tlb_frame[index];
    }else{
        frame = page_table[page];

        if(frame == -1){
            frame = load_page(backing, page);

            if(frame == -1){
                return -1;
            }
        }

        index = tlb_insert(page, frame);
    }

    frame_last_use[frame] = ++clock_counter;
    tlb_last_use[index] = clock_counter;

    *tlb_index = index;
    *physical = frame * PAGE_SIZE + offset;
    *value = physical_memory[frame][offset];

    return 0;
}





int main(int argc, char *argv[]){
    FILE *input, *backing, *output;

    char line[LINE_SIZE];
    int address, tlb_index, physical, value;
    int line_number = 0;

    if(argc != 4){
        fprintf(stderr, "Uso: %s <arquivo_de_enderecos> <fifo|lru> <fifo|lru>\n", argv[0]);
        return 1;
    }

    if(!parse_algorithm(argv[2], &replace_alg)){
        fprintf(stderr, "Erro: algoritmo de pagina '%s' invalido (use fifo ou lru)\n", argv[2]);
        return 1;
    }

    if(!parse_algorithm(argv[3], &tlb_alg)){
        fprintf(stderr, "Erro: algoritmo da TLB '%s' invalido (use fifo ou lru)\n", argv[3]);
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

    output = fopen(TEMP_FILE, "w");

    if(output == NULL){
        fprintf(stderr, "Erro: nao foi possivel criar '%s'\n", TEMP_FILE);

        fclose(input);
        fclose(backing);
        return 1;
    }

    init_page_table();
    init_frames();
    init_tlb();

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

        if(translate(backing, address, &tlb_index, &physical, &value) != 0){
            fprintf(stderr, "Erro: falha ao traduzir o endereco da linha %d\n", line_number);
            goto fail;
        }

        translated++;

        fprintf(output, "Virtual address: %d TLB: %d Physical address: %d Value: %d\n", address, tlb_index, physical, value);
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
    fprintf(output, "TLB Hits = %d\n", tlb_hits);
    fprintf(output, "TLB Hit Rate = %.3f\n", (double)tlb_hits / translated);

    if(ferror(output)){
        fprintf(stderr, "Erro: falha ao gravar o arquivo de saida\n");
        goto fail;
    }

    fclose(input);
    fclose(backing);

    if(fclose(output) != 0 || rename(TEMP_FILE, OUTPUT_FILE) != 0){
        fprintf(stderr, "Erro: nao foi possivel gravar '%s'\n", OUTPUT_FILE);
        remove(TEMP_FILE);
        return 1;
    }

    return 0;

fail:
    fclose(input);
    fclose(backing);
    fclose(output);
    remove(TEMP_FILE);
    return 1;
}
