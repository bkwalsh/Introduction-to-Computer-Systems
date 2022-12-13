#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

char *file = NULL;
char s;
int H = 0, M = 0, R = 0, E = 0, b = 0, bigS = 0, option = 0;

long bitmasker(long int x)
{
    long mask = 0;
    while (x)
    {
        x--;
        mask = (mask << 1) | 1;
    }
    return mask;
}

typedef struct line
{
    long int tagbit;
    int validbit;
} ln_t;

typedef struct linked_list
{
    struct linked_list *next;
    int index;
} l_l;

typedef struct set
{
    l_l *LRU_priority;
    ln_t *line_array;
    int capacity;
} st_t;

void freeCache(st_t **cache)
{
    for (int i = 0; i < bigS; i++)
    {
        st_t *curr = cache[i];
        l_l *ll = curr->LRU_priority;
        if (ll == NULL)
        {
            return;
        }
        l_l *prev = ll;
        while (ll)
        {
            prev = ll;
            ll = ll->next;
            free(prev);
        }
        free(curr->line_array);
    }
    free(cache);
}

st_t **buildcache()
{
    st_t **ch = (st_t **)malloc(sizeof(st_t *) * bigS);
    for (int x = 0; x < bigS; x++)
    {
        ln_t *line_array = (ln_t *)malloc(sizeof(ln_t) * E);
        for (int y = 0; y < E; y++)
        {
            line_array[y].tagbit = 0;
            line_array[y].validbit = 0;
        }
        st_t *st = (st_t *)malloc(sizeof(st_t));
        st->LRU_priority = NULL;
        st->line_array = line_array;
        st->capacity = 0;
        ch[x] = st;
    }
    return ch;
}

int get_LRU_place(l_l *currque)
{
    if (!currque)
    {
        return 0;
    }
    while (currque->next)
    {
        currque = currque->next;
    }
    return currque->index;
}

l_l *rmv_at_index(int index, l_l *currque)
{
    if (!currque)
    {
        return NULL;
    }
    l_l *head, *prev;
    head = currque;
    if (index == currque->index)
    {
        head = currque->next;
        free(currque);
        return head;
    }
    while (currque)
    {
        if (index == currque->index)
        {
            prev->next = currque->next;
            free(currque);
            return head;
        }
        prev = currque;
        currque = currque->next;
    }
    return head;
}

l_l *push(l_l *currque, int index)
{
    l_l *link = (l_l *)malloc(sizeof(l_l));
    link->index = index;
    link->next = currque;
    return link;
}

void updateset(long tag, st_t *set)
{
    int set_cap = set->capacity;
    for (int x = 0; x < E; x++)
    {
        if (set->line_array[x].validbit == 1 && tag == set->line_array[x].tagbit)
        {
            H++;
            l_l *priority_update = set->LRU_priority;
            priority_update = rmv_at_index(x, priority_update);
            priority_update = push(priority_update, x);
            set->LRU_priority = priority_update;
            return;
        }
    }
    set_cap++;
    int place = 0;
    for (place = 0; place < E; place++)
    {
        if (set->line_array[place].validbit != 1)
        {
            break;
        }
    }
    l_l *priority_update = set->LRU_priority;
    if (place >= E)
    {
        R++;
        place = get_LRU_place(priority_update);
    }
    priority_update = rmv_at_index(place, priority_update);
    priority_update = push(priority_update, place);
    set->LRU_priority = priority_update;
    set->capacity = set_cap;
    set->line_array[place].tagbit = tag;
    set->line_array[place].validbit = 1;
    M++;
    return;
}

void simulate()
{
    char option;
    long int add;
    int size;
    FILE *fileref = fopen(file, "r");
    st_t **cache = buildcache();
    while (fscanf(fileref, "%c %lx,%d", &option, &add, &size) > 0)
    {
        long int setref = (add >> b) & bitmasker(s);
        long int tagref = (add >> (s + b)) & bitmasker(64 - s - b);
        switch (option)
        {
        case 'S':
        case 'L':
            updateset(tagref, cache[setref]);
            break;
        case 'M':
            updateset(tagref, cache[setref]);
            updateset(tagref, cache[setref]);
            break;
        default:
            break;
        }
    }
    fclose(fileref);
    freeCache(cache);
}

int main(int argc, char *argv[])
{
    while (!((option = getopt(argc, argv, "t:b:E:s:")) == -1))
    {

        switch (option)
        {
        case 's':
            s = atoi(optarg);
            bigS = 1 << s;
            break;
        case 't':
            file = optarg;
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'I':

            break;
        default:
            fprintf(stderr, "Bad input, re-enter \n");
        }
    }
    simulate();
    printSummary(H, M, R);
    return 0;
}