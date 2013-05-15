// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
#include "alloc.h"
#include <stdio.h>
#include <string.h>

void parse_error(int l) {
    error(1, 0, ":%d\n", l);
}

void null_char_push(char *c, int *position, size_t *size) {
    if ((*position) * sizeof(char) == *size) {
        *size = *size + sizeof(char);
        c = checked_realloc(c, *size);
    }
    c[*position] = '\0';
    (*position)++;
}

void push (char ***sq, char* word, int *position, size_t *size) {
    if ((*position) * sizeof(char *) == *size) {
        *sq = checked_grow_alloc(*sq, size);
    }
    (*sq)[*position] = word;
    (*position)++;
}

char* pop (char **stack, int *position) {
    (*position)--;
    char *word = stack[*position];
    stack[*position] = NULL;
    return word;
}

void push_c (command_t **stack, command_t c, int *position, size_t *size) {
    if ((*position) * sizeof(char *) == *size) {
        *stack = checked_grow_alloc(*stack, size);
    }
    (*stack)[*position] = c;
    (*position)++;
}

command_t pop_c (command_t *stack, int *position) {
    (*position)--;
    command_t c = stack[*position];
    stack[*position] = NULL;
    return c;
}

int precedence (char* a) {
    if (a[0] == '&' && a[1] == '&') { return 1;} 
    //else if (a[0] == '|' && a[1] == '|') { return 2;}
    else if (a[0] == '|' && a[1] == '|') { return 1;}
    else if (a[0] == '(' && a[1] == ')') { return -9;} 
    else if (a[0] == '|') { return 3;} 
    else if (a[0] == ';') { return 0;} 
    else if (a[0] == '<') { return 6;} 
    else if (a[0] == '>') { return 5;}

    return -1;
}

enum command_type get_type (char *a) {
    if (a[0] == '&' && a[1] == '&') { return AND_COMMAND;} 
    else if (a[0] == '|' && a[1] == '|') { return OR_COMMAND;}
    else if (a[0] == '|') { return PIPE_COMMAND;} 
    else if (a[0] == ';') { return SEQUENCE_COMMAND;} 

}

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */
struct command_stream {
    char ***command_queue;
    int dim3;
};

int pos = 0;

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
    int cq_count = 0;       size_t cq_cap = 1 * sizeof(char **);
    command_stream_t cs = checked_malloc(sizeof(struct command_stream));
    cs->command_queue = checked_malloc(cq_cap);
    cs->dim3 = 0;

    int queue_count = 0;    size_t queue_cap = 1 * sizeof(char *);
    int stack_count = 0;    size_t stack_cap = 1 * sizeof(char *);
    int nchar = 0;          size_t char_cap = 8 * sizeof(char);
    char **queue = checked_malloc(queue_cap);
    char **stack = checked_malloc(stack_cap);
    char *operand = checked_malloc(char_cap);
    int line_num = 1;
    int num_lp = 0;
    int num_rp = 0;
    char op;
    bool on_op = false;
    bool on_2nd_arg = false;
    bool on_semi = false;
    bool skip_malloc = true;
    bool close_paren = false;
    bool on_comment = false;

    char c = get_next_byte(get_next_byte_argument);
    for (; c != EOF; c = get_next_byte(get_next_byte_argument)) {
        if (nchar == 0 && !skip_malloc) {
            //char_cap = 8 * sizeof(char);
            operand = checked_malloc(char_cap);
        }

        if (c == '\n') {
            if (on_comment) {
                on_comment = false;
                line_num++;
                continue;
            }
            if (on_2nd_arg && !on_semi) {
                on_2nd_arg = false;
                nchar = 0;
                line_num++;
                continue;
            }
            if (nchar == 0 && !close_paren) {
                line_num++;
                continue;
            }

            if (on_semi) {
                char *semicolon = checked_malloc(2 * sizeof(char));
                semicolon[0] = ';';
                semicolon[1] = '\0';
                push(&stack, semicolon, &stack_count, &stack_cap);
            }
            if (!on_semi && !close_paren) {
                null_char_push(operand, &nchar, &char_cap);
                push(&queue, operand, &queue_count, &queue_cap);
            }

            if (num_lp != num_rp) {
                parse_error(line_num);
            }
            num_lp = 0;
            num_rp = 0;

            while (stack_count > 0) {
                char *operator = pop(stack, &stack_count);
                push(&queue, operator, &queue_count, &queue_cap);
            }
            stack_count = 0;

            push(&queue, NULL, &queue_count, &queue_cap);

            if (cq_count * sizeof(char **) == cq_cap) {
                cs->command_queue = checked_grow_alloc(cs->command_queue, &cq_cap);
            }
            cs->command_queue[cq_count] = queue;
            cs->dim3 = cs->dim3 + 1;
            cq_count++;

            queue = checked_malloc(queue_cap);
            stack = checked_malloc(stack_cap);
            queue_count = 0;
            stack_count = 0;
           
            nchar = 0;
            line_num++;
            skip_malloc = false;
            continue;
        }

        if (c == '#' || on_comment) {
            on_comment = true;
            continue;
        }

        if (c == '&' || c == '|' || c == '<' || c == '>' || c == ';' || on_op) {
            if (on_semi && c ==';') { 
                parse_error(line_num);
            }
            bool single_char_op = false;
            on_semi = false;
            if (c != '&' && c != '|') {
                single_char_op = true;
            }
            if (on_op) {
                if (c != op && !single_char_op) {
                    parse_error(line_num);
                } else {
                    char *temp = checked_malloc(2 * sizeof(char));
                    temp[0] = op;
                    temp[1] = single_char_op ? '\0' : op;

                    if (stack_count > 0) {
                        char *top = stack[stack_count-1];
                        while (precedence(top) >= precedence(temp)) {
                            char* st = pop(stack, &stack_count);
                            push(&queue, st, &queue_count, &queue_cap);
                            if (stack_count == 0) {
                                break;
                            }
                            top = stack[stack_count-1];
                        }
                    }

                    push(&stack, temp, &stack_count, &stack_cap);

                    on_semi = op == ';';
                    on_op = false;
                    op = '\0';
                    on_2nd_arg = true;
                    if (!single_char_op || c == ' ' || c == '\t') {
                        continue;
                    }
                }
            } else if (nchar == 0 && !close_paren) {
                parse_error(line_num);
            } else {
                if (!close_paren) {
                    null_char_push(operand, &nchar, &char_cap);
                    push(&queue, operand, &queue_count, &queue_cap);
                }

                on_semi = c == ';';
                nchar = 0;
                skip_malloc = false;
                op = c;
                on_op = true;
                on_2nd_arg = c != '&';
                continue;
            }
        }

        if (c == '(') {
            char *paren = checked_malloc(2 * sizeof(char));
            paren[0] = '(';
            paren[1] = ')';
            push(&stack, paren, &stack_count, &stack_cap);
            num_lp++;
            continue;
        }
        if (c == ')') {
            if (stack_count > 0) {
                null_char_push(operand, &nchar, &char_cap);
                push(&queue, operand, &queue_count, &queue_cap);

                char *peek = stack[stack_count-1];
                while (precedence(peek) != -9) {
                    char *sv = pop(stack, &stack_count);
                    push(&queue, sv, &queue_count, &queue_cap);
                    if (stack_count == 0) {
                        parse_error(line_num);
                    }
                    peek = stack[stack_count-1];
                }
                char* open_paren = pop(stack, &stack_count);
                push(&queue, open_paren, &queue_count, &queue_cap);
            } else {
                parse_error(line_num);
            }

            num_rp++;
            nchar = 0;
            skip_malloc = false;
            close_paren = true;
            continue;
        }

        if (nchar == 0 && c == ' ') {
            skip_malloc = true;
            continue;
        }

        if (nchar * sizeof(char) == char_cap) {
            operand = checked_grow_alloc(operand, &char_cap);
        }

        if (c == '`' || c == '>' || c == '<' || c == '|' || c == ';' || c == '&') {
            parse_error(line_num);
        }

        operand[nchar] = c;
        nchar++;

        on_semi = false;
        on_2nd_arg = false;
        close_paren = false;
    }

    if (stack_count != 0) {
        parse_error(line_num - 1);
    }

    return cs;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
    if (pos == s->dim3) {
        return NULL;
    }

    char** carr = s->command_queue[pos];
    int sp = 0;     size_t sc = 1 * sizeof(command_t);
    command_t *stack = checked_malloc(sc);
    
    char *word = carr[0];
    for (int i = 0; word != NULL; i++) {
        word = carr[i];
        if (word == NULL) {
            break;
        }
        //printf("%s\n", word);
        if (word[0] == '&' || word[0] == '|' || word[0] == ';' || word[0] == '<' || word[0] == '>') {
            if (word[0] == ';' && sp < 2) {
                continue;
            }
            if (word[0] == ';' && carr[i+1] != NULL && precedence(carr[i+1]) != -1) {
                // treat semicolon as unary postfix operator here
                continue;
            }
            command_t arg2 = pop_c(stack, &sp);
            command_t arg1 = pop_c(stack, &sp);
            command_t bind = checked_malloc(sizeof(struct command));
            bind->type = SIMPLE_COMMAND;
            bind->status = -1;
            bind->input = NULL;
            bind->output = NULL;
            int p = precedence(word);
            if (p == 5) { 
                // I/O rediretion >
                if (arg1->type == SIMPLE_COMMAND && arg1->input != NULL) {
                    bind->input = arg1->input;
                    bind->output = arg2->u.word[0];
                    bind->u.word = arg1->u.word;
                } else { 
                    bind->output = arg2->u.word[0];
                    bind->u.word = arg1->u.word;
                }
            } else if (p == 6) {
                // I/O rediretion < 
                bind->input = arg2->u.word[0];
                bind->u.word = arg1->u.word;
            } else {
                //bind->type = p;
                bind->type = get_type(word);
                bind->u.command[0] = arg1;
                bind->u.command[1] = arg2;
            }

            push_c(&stack, bind, &sp, &sc);
            
            continue;
        }
        if (word[0] == '(') {
            command_t inside = pop_c(stack, &sp);
            command_t subsh = checked_malloc(sizeof(struct command));
            subsh->type = SUBSHELL_COMMAND;
            subsh->status = -1;
            subsh->input = NULL;
            subsh->output = NULL;
            //subsh->u.subshell_command = checked_malloc(1 * sizeof(command_t));
            subsh->u.subshell_command = inside;
            push_c(&stack, subsh, &sp, &sc);
            continue;
        }

        command_t simple_comm = checked_malloc(sizeof(struct command));
        simple_comm->type = SIMPLE_COMMAND;
        simple_comm->status = -1;
        simple_comm->input = NULL;
        simple_comm->output = NULL;
        size_t word_cap = 8 * sizeof(char *);
        //int word_cap = strlen(word) * sizeof(char *);
        simple_comm->u.word = checked_malloc(word_cap);
        char *w;
        w = strtok(word, " ");
        int j = 0;
        for (; w != NULL; j++) {
            if (j * sizeof(char *) == word_cap) {
                simple_comm->u.word = checked_grow_alloc(simple_comm->u.word, &word_cap);
            }
            simple_comm->u.word[j] = w;
            w = strtok(NULL, " ");
        }
        if (j * sizeof(char *) == word_cap) {
            word_cap += sizeof(char *);
            simple_comm->u.word = checked_realloc(simple_comm->u.word, word_cap);
        }
        simple_comm->u.word[j] = NULL;

        push_c(&stack, simple_comm, &sp, &sc);

    }

    command_t ret = pop_c(stack, &sp);
    pos++;

    return ret;
}

