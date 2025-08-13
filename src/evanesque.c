/*
 * evanesque.c — a tiny, self-erasing stack language
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

/* -- configuration  */
#define STACKSZ 4096 /* data stack size                  */
#define CALLSZ 4096  /* call stack depth (no recursion)  */
#define LOOPSZ 4096  /* loop stack depth                 */
#define DICTSZ 256   /* max number of user words         */
#define BUFSZ 65536  /* compile-time storage for names/bodies */

typedef long cell;

static cell dstack[STACKSZ];
static cell *sp = dstack;

static void die(const char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
static void push(cell v)
{
    if (sp - dstack >= STACKSZ)
        die("stack overflow");
    *sp++ = v;
}
static cell pop(void)
{
    if (sp == dstack)
        die("stack underflow");
    return *--sp;
}

typedef struct
{
    char *name;
    char *body;
} word;

static word dict[DICTSZ];
static int dictlen = 0;

static int find(const char *s)
{
    for (int i = 0; i < dictlen; i++)
        if (strcmp(dict[i].name, s) == 0)
            return i;
    return -1;
}

static char arena[BUFSZ];
static char *nextb = arena;

/* the twist: erase a random existing word (if any) */
static void vanish(void)
{
    if (dictlen == 0)
        return;
    int r = rand() % dictlen;
    dictlen--;
    dict[r] = dict[dictlen];
}

typedef struct
{
    char *save;
    int loopbase;
} frame;

static frame cstack[CALLSZ];
static frame *cfp = cstack + CALLSZ;

static char *lstack[LOOPSZ];
static int ltop = 0;

static int isnum(const char *s, cell *v)
{
    char *e;
    long x = strtol(s, &e, 0);
    if (*s && *e == 0)
    {
        *v = x;
        return 1;
    }
    return 0;
}

static void skip_comment(char **psave)
{
    char *save = *psave;
    char *tok;
    while ((tok = strtok_r(NULL, " \t\r\n", &save)))
    {
        if (strcmp(tok, "*/") == 0)
        {
            *psave = save;
            return;
        }
    }
    *psave = save;
}

static void compile_word(char **psave)
{
    char *save = *psave;

    char *name = strtok_r(NULL, " \t\r\n", &save);
    if (!name)
        die("empty definition");
    if (dictlen >= DICTSZ)
        die("dictionary full");

    size_t need = strlen(name) + 1;
    if (nextb + need >= arena + BUFSZ)
        die("compile arena full");
    dict[dictlen].name = nextb;
    memcpy(nextb, name, need);
    nextb += need;

    char *body = nextb;
    char *tok;
    while ((tok = strtok_r(NULL, " \t\r\n", &save)))
    {
        if (strcmp(tok, ";") == 0)
        {
            break;
        }
        if (strcmp(tok, "/*") == 0)
        {
            skip_comment(&save);
            continue;
        }

        size_t tlen = strlen(tok);
        if (nextb + tlen + 2 >= arena + BUFSZ)
            die("compile arena full");
        memcpy(nextb, tok, tlen);
        nextb += tlen;
        *nextb++ = ' ';
    }
    *nextb++ = '\0';

    dict[dictlen].body = body;
    dictlen++;
    *psave = save;
    vanish(); /* the self-erasing twist :D */
}

static void run(char *line)
{
    char *save = NULL;
    char *ip = line;

    for (;;)
    {
        char *tok = strtok_r(ip, " \t\r\n", &save);
        ip = NULL;

        if (!tok)
        {
            if (cfp == cstack + CALLSZ)
                return;
            save = cfp->save;
            ltop = cfp->loopbase;
            cfp++;
            continue;
        }

        if (strcmp(tok, "/*") == 0)
        {
            skip_comment(&save);
            continue;
        }

        /* built-ins (arithmetic/comparison/stack) */
        if (strcmp(tok, "+") == 0)
        {
            push(pop() + pop());
            continue;
        }

        if (strcmp(tok, "-") == 0)
        {
            cell a = pop(), b = pop();
            push(b - a);
            continue;
        }

        if (strcmp(tok, "*") == 0)
        {
            push(pop() * pop());
            continue;
        }

        if (strcmp(tok, "/") == 0)
        {
            cell a = pop(), b = pop();
            if (a == 0)
                die("division by zero");
            push(b / a);
            continue;
        }

        if (strcmp(tok, "=") == 0)
        {
            push(pop() == pop());
            continue;
        }

        if (strcmp(tok, "<") == 0)
        {
            cell a = pop(), b = pop();
            push(b < a);
            continue;
        }

        if (strcmp(tok, ">") == 0)
        {
            cell a = pop(), b = pop();
            push(b > a);
            continue;
        }

        if (strcmp(tok, ".") == 0)
        {
            printf("%ld\n", pop());
            continue;
        }

        if (strcmp(tok, "emit") == 0)
        {
            putchar((int)pop());
            fflush(stdout);
            continue;
        }

        if (strcmp(tok, "key") == 0)
        {
            int ch = getchar();
            push(ch == EOF ? -1 : ch);
            continue;
        }

        if (strcmp(tok, "dup") == 0)
        {
            if (sp == dstack)
                die("stack underflow");
            push(sp[-1]);
            continue;
        }

        if (strcmp(tok, "drop") == 0)
        {
            (void)pop();
            continue;
        }

        if (strcmp(tok, "swap") == 0)
        {
            cell a = pop(), b = pop();
            push(a);
            push(b);
            continue;
        }
        if (strcmp(tok, "rot") == 0)
        { /* (a b c -- b c a) */
            cell c = pop(), b = pop(), a = pop();
            push(b);
            push(c);
            push(a);
            continue;
        }
        if (strcmp(tok, "-rot") == 0)
        { /* (a b c -- c a b) */
            cell c = pop(), b = pop(), a = pop();
            push(c);
            push(a);
            push(b);
            continue;
        }

        if (strcmp(tok, "over") == 0)
        {
            if (sp - dstack < 2)
                die("stack underflow");
            push(sp[-2]);
            continue;
        }

        if (strcmp(tok, "tuck") == 0)
        {
            if (sp - dstack < 2)
                die("stack underflow");
            cell a = pop(), b = pop(); /* ... b a -> ... a b a */
            push(a);
            push(b);
            push(a);
            continue;
        }

        /* flow control (inside definitions) */
        if (strcmp(tok, "begin") == 0)
        {
            if (ltop >= LOOPSZ)
                die("loop stack overflow");
            lstack[ltop++] = save;
            continue;
        };

        if (strcmp(tok, "while") == 0)
        {
            if (ltop <= 0)
                die("while without begin");
            cell cond = pop();
            if (cond)
            {
                continue;
            }

            ltop--;

            char *tmp = save;
            int depth = 1;
            char *t;

            while ((t = strtok_r(NULL, " \t\r\n", &tmp)))
            {
                if (strcmp(t, "/*") == 0)
                {
                    skip_comment(&tmp);
                    continue;
                }
                if (strcmp(t, "begin") == 0)
                    depth++;
                else if (strcmp(t, "repeat") == 0)
                {
                    if (--depth == 0)
                    {
                        save = tmp;
                        break;
                    }
                }
            }
            if (!t)
                die("while: missing matching 'repeat'");
            continue;
        }

        if (strcmp(tok, "repeat") == 0)
        {
            if (ltop <= 0)
                die("repeat without begin");
            save = lstack[ltop - 1];
            continue;
        }

        if (strcmp(tok, ":") == 0)
        {
            compile_word(&save);
            continue;
        }

        if (strcmp(tok, ";") == 0)
        {
            die("';' outside definition");
        }

        /* numbers */
        cell v;
        if (isnum(tok, &v))
        {
            push(v);
            continue;
        }

        /* user-defined word */
        int idx = find(tok);
        if (idx < 0)
        {
            fprintf(stderr, "unknown word: %s\n", tok);
            exit(1);
        }

        if (cfp == cstack)
            die("call stack overflow");
        *--cfp = (frame){.save = save, .loopbase = ltop};
        ip = dict[idx].body;
        save = NULL;
    }
}

int main(void)
{
    srand((unsigned)time(NULL));

    char line[8192];
    while (fgets(line, sizeof line, stdin))
        run(line);

    return 0;
}
