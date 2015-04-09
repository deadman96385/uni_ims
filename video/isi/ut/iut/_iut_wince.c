#include <stdio.h>

char keypress = 0;
char XXGETCH(void)
{
    char key[2];
    while(1) {
        fgets(key, 2, stdin);
        if ((keypress != '\n') && (key[0] == '\n')) {
            keypress=key[0];
        }
        else {
            keypress=key[0];
            break;
        }
    }
    return (key[0]);
}
