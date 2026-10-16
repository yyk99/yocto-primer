/*
 * debug-demo: a small console program to practise remote debugging with
 * gdbserver. Each mode is meant for one exercise:
 *
 *   debug-demo fact [n]   recursion: breakpoints, backtrace, finish
 *   debug-demo list       heap data: step through, print structs and pointers
 *   debug-demo loop       runs until killed: attach, watchpoints, Ctrl-C
 *   debug-demo crash      NULL dereference: post-mortem look at SIGSEGV
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct node {
    int value;
    struct node *next;
};

static unsigned long factorial(unsigned n)
{
    if (n <= 1)
        return 1;
    return n * factorial(n - 1);
}

static struct node *push(struct node *head, int value)
{
    struct node *n = malloc(sizeof(*n));

    if (!n) {
        perror("malloc");
        exit(1);
    }
    n->value = value;
    n->next = head;
    return n;
}

static int sum(const struct node *n)
{
    int total = 0;

    for (; n; n = n->next)
        total += n->value;
    return total;
}

static void run_fact(unsigned n)
{
    printf("%u! = %lu\n", n, factorial(n));
}

static void run_list(void)
{
    struct node *head = NULL;
    int i;

    for (i = 1; i <= 5; i++)
        head = push(head, i * i);
    printf("sum of squares = %d\n", sum(head));

    while (head) {
        struct node *next = head->next;
        free(head);
        head = next;
    }
}

static void run_loop(void)
{
    unsigned long ticks = 0;

    for (;;) {
        ticks++;
        printf("tick %lu (pid %d)\n", ticks, getpid());
        fflush(stdout);
        sleep(2);
    }
}

static void run_crash(void)
{
    struct node *volatile n = NULL;

    printf("about to crash\n");
    fflush(stdout);
    printf("%d\n", n->value);
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "";

    if (!strcmp(mode, "fact"))
        run_fact(argc > 2 ? (unsigned)atoi(argv[2]) : 5);
    else if (!strcmp(mode, "list"))
        run_list();
    else if (!strcmp(mode, "loop"))
        run_loop();
    else if (!strcmp(mode, "crash"))
        run_crash();
    else {
        fprintf(stderr, "usage: %s fact [n] | list | loop | crash\n", argv[0]);
        return 2;
    }
    return 0;
}
