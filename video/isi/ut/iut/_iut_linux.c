#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
static int fid = -1;

char XXGETCH(void)
{
    char key[2];

    if (fid < 0) {
        fid = open(CONSOLE, O_RDONLY);
        if (fid < 0) {
            printf("cannot open %s\n", CONSOLE);
            return (-1);
        }
    }
    
    while (read(fid, key, 1) <= 0);
    return (key[0]);
}
