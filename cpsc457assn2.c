#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// structs
struct Page
{
    int pageNum;
    int dirtyBit;
    int referenceBit;
};

struct Queue {
    int size;
    struct QueueNode* front;
    struct QueueNode* rear;
};

struct QueueNode {
    struct Page page;
    struct QueueNode* next;
};

struct Queue* new_queue() {
    struct Queue* queue = (struct Queue*)malloc(sizeof(struct Queue));
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
    return queue;
}

struct QueueNode* new_node(struct Page page) {
    struct QueueNode* new = (struct QueueNode*)malloc(sizeof(struct QueueNode));
    new->page = page;
    new->next = NULL;
    return new;
}

// function prototypes
void enqueue(struct Queue *queue, struct Page page);
struct Page dequeue(struct Queue *queue);
int *FIFO(struct Page *references, int ref_length, int frame);
bool inQueue(struct Queue *queue, struct Page *this_page);
void FIFO_output(struct Page *references);
int *OPT(struct Page *references, int ref_length, int frame);
void OPT_output(struct Page *references);
int *secondChance(struct Page *references, int ref_length, int frame);
void secondChance_output(struct Page *references);


// constants
int ref_length = 15050; // make it 150 for now so the data is manageable
//int frame_count = 100;
char cmp_string[] = "OPT";
char p1_table_line1[] = "+--------+-------------+-------------+\n";
char p1_table_line2[] = "| Frames | Page Faults | Write backs |\n";
char p1_table_line3[] = "| %6d | %11d | %11d |\n";

int main()
//parameters - int argc, char *argv[]
{
    //if (argc < 2)
    //{
        //printf("Usage: %s <input_file>\n", argv[0]);
        //return 1;
    //}

    FILE *file = fopen("input_file.csv", "r");
    if (!file)
    {
        //printf("Error opening file %s\n", argv[1]);
        return 1;
    }

    struct Page references[15050]; // 15052 entries
    int i = 0;
    int pageNum, dirtyBit;
    char header[100];

    // read header (assuming first line is the header)
    fgets(header, sizeof(header), file);

    // read each row from the input file
    while (fscanf(file, "%d,%d\n", &pageNum, &dirtyBit) == 2)
    {
        references[i].pageNum = pageNum;
        references[i].dirtyBit = dirtyBit;
        i++;
        if (i >= 15052)
            break; // stop once array is full
    }

    if (strcmp(cmp_string, "FIFO") == 0) {
        //argv[1]
        FIFO_output(references);
    } else if (strcmp(cmp_string, "OPT") == 0)
        OPT_output(references);

    fclose(file);
    return 0;
}


//Algos

// queue algorithms - enqueue and dequeue
void enqueue(struct Queue* queue, struct Page page) {
    struct QueueNode* new = new_node(page);
    if (queue->rear == NULL) {
        queue->front = new;
        queue->rear = new;
    } else {
        queue->rear->next = new;
        queue->rear = new;
    }
    queue->size++;
}

struct Page dequeue(struct Queue* queue) {
    if (queue->front == NULL) {
        printf("Error - empty queue.");
    }

    struct QueueNode* temp_node = queue->front;
    struct Page page = temp_node->page;
    queue->front = queue->front->next;

    if (queue->front == NULL) { queue->rear = NULL; }

    free(temp_node);
    queue->size--;
    return page;
}

bool inQueue(struct Queue *queue, struct Page *this_page) {
    struct QueueNode *node = queue->front;
    while (node != NULL) {
        int node_num = node->page.pageNum;
        int comp_num = this_page->pageNum;
        if (node_num == comp_num) {
            return true;
        }
        node = node->next;
    }
    return false;
}

//FIFO: replaces the page that has been in memory the longest

int* FIFO(struct Page *references, int ref_length, int frame) {
    struct Queue* pages_queue = new_queue();
    int page_faults = 0;
    int write_backs = 0;

    for (int i = 0; i < ref_length; i++) {
        struct Page *this_page = &references[i];
        int this_pn = this_page->pageNum;
        int this_db = this_page->dirtyBit;
        
        if (inQueue(pages_queue, this_page) == false) {
            page_faults++;

            if (pages_queue->size == frame) {
                struct Page old_page = dequeue(pages_queue);
                int dirty_bit = old_page.dirtyBit;
                if (dirty_bit == 1) { write_backs++; }
            }
            enqueue(pages_queue, *this_page);
        }
    }
    int* pfwb = (int*)malloc(2 * sizeof(int));
    pfwb[0] = page_faults;
    pfwb[1] = write_backs;
    return pfwb;
}

void FIFO_output(struct Page *references) {
    printf("FIFO\n");
    printf("%s", p1_table_line1);
    printf("%s", p1_table_line2);
    printf("%s", p1_table_line1);
    for (int i = 1; i < 101; i++) {
        int* curr_array = FIFO(references, ref_length, i);
        int pf = curr_array[0];
        int wb = curr_array[1];
        printf(p1_table_line3, i, pf, wb);
        printf("%s", p1_table_line1);
    }
}

//Optimal:  replaces the page that will not be used for the longest period in the future
int* OPT(struct Page *references, int ref_length, int frame) {
    struct Queue* queue = new_queue();
    int page_faults = 0;
    int write_backs = 0;

    for (int i =0; i < ref_length; i++) {
        struct Page *this_page = &references[i];

        if (inQueue(queue, this_page) == false) {
            page_faults++;

            if (queue->size == frame) {
                int replace_with = -1;
                int furthest_away = -1;

                struct QueueNode* node = queue->front;
                for (int j = 0; j < queue->size; j++) {
                    struct Page curr_page = node->page;
                    int used_next = -1;

                    for (int k = i + 1; k < ref_length; k++) {
                        struct Page comp_page = references[k];
                        if (comp_page.pageNum == curr_page.pageNum) {
                            used_next = k;
                            break;
                        }
                    }

                    if (used_next == 1) {
                        replace_with = j;
                        break;
                    } 
                    if (used_next > furthest_away) {
                        furthest_away = used_next;
                        replace_with = j;
                    }

                    node = node->next;
                }

                struct Page old_page = dequeue(queue);
                if (old_page.dirtyBit == 1) { write_backs++; }
            }
        }
        enqueue(queue, *this_page);
    }

    int* pfwb = (int*)malloc(2 * sizeof(int));
    pfwb[0] = page_faults;
    pfwb[1] = write_backs;
    return pfwb;
}

void OPT_output(struct Page *references) {
    printf("OPT\n");
    printf("%s", p1_table_line1);
    printf("%s", p1_table_line2);
    printf("%s", p1_table_line1);
    for (int i = 1; i < 101; i++) {
        int* curr_array = OPT(references, ref_length, i);
        int pf = curr_array[0];
        int wb = curr_array[1];
        printf(p1_table_line3, i, pf, wb);
        printf("%s", p1_table_line1);
    }
}

//LRU: replaces the page that has not been used for the longest period of time


// Second Chance

int *secondChance(struct Page *references, int ref_length, int frame)
{
    struct Queue *pages_queue = new_queue();
    int page_faults = 0;
    int write_backs = 0;

    for (int i = 0; i < ref_length; i++)
    {
        struct Page *this_page = &references[i];
        int this_pn = this_page->pageNum;
        int this_db = this_page->dirtyBit;

        if (inQueue(pages_queue, this_page))
        { // if reference page is in queue, set reference bit to 1
            struct QueueNode *node = pages_queue->front;
            while (node != NULL)
            {
                if (node->page.pageNum == this_pn) //if reference page is current node in queue
                {
                    node->page.referenceBit = 1; // set reference bit to 1
                    break;
                }
                node = node->next; // increment until node is found
            }
        }
        else
        {
            // if page isn't in queue, page fault occurs
            page_faults++;

            if (pages_queue->size == frame)
            {
                // need to check for second chance if full
                while (true)
                {
                    struct Page old_page = dequeue(pages_queue); 
                    if (old_page.referenceBit == 0) //if reference bit is 0 
                    {
                        int dirty_bit = old_page.dirtyBit; 
                        if (dirty_bit == 1) // count a write back if bit to be replaced is dirty
                        {
                            write_backs++;
                        }
                        break; //break as page has been dequed
                    }
                    else // otherwise we change the reference bit from 1 to 0 and re enqueue the page until a page with a bit of 0 is found
                    {
                        old_page.referenceBit = 0;
                        enqueue(pages_queue, old_page);
                    }
                }
            }

        //  if page is not in queue and queue isn't full, set reference bit to 1 and enqueue
            this_page->referenceBit = 1;
            enqueue(pages_queue, *this_page);
        }
    }
    int *pfwb = (int *)malloc(2 * sizeof(int)); 
    pfwb[0] = page_faults;
    pfwb[1] = write_backs;
    return pfwb;
}

void secondChance_output(struct Page *references)
{
    printf("Second Chance\n");
    printf("%s", p1_table_line1);
    printf("%s", p1_table_line2);
    printf("%s", p1_table_line1);
    for (int i = 1; i < 101; i++)
    {
        int *curr_array = secondChance(references, ref_length, i);
        int pf = curr_array[0];
        int wb = curr_array[1];
        printf(p1_table_line3, i, pf, wb);
        printf("%s", p1_table_line1);
    }
}