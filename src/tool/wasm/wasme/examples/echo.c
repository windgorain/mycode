#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <editline/readline.h>

int main()
{
    for(;;) {
        char *line;
        line = readline("echo> ");
        if (!line) return 0; 
        add_history(line); 

        printf("%s\n", line);
    }
}
