#include <stdio.h>

#include "wsis.h"
#include "input.h"

int main(void){
    if (storageInit() != 0){
        printf("The system could not start.\n");
        return 1;
    }

    runMenu();

    storageShutdown();

    
    return 0;
}