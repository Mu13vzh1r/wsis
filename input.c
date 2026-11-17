#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "wsis.h"
#include "input.h"

static int readLine(char *input, int size){
    if (fgets(input, size, stdin) == NULL)
        return 0;

    size_t length = strlen(input);
    if (length > 0 && input[length-1] == '\n'){
        input[length-1] = '\0';
        return 1;
    }
    //remove leftover characters from the input stream
    if (length == (size_t)size-1){  
        int c;
        while ((c = getchar()) != '\n' && c != EOF){}
        return -1;
    }

    return 1;
}

static int getChoice(int min, int max){
    char buffer[64];
    int choice = 0;
    bool valid = false;

    while (!valid){
        int check = readLine(buffer, sizeof(buffer));
        
        if (check == 0) return -1;

        if (check == 1){
            char *end;
            long value = strtol(buffer, &end, 10);
            bool hasDigits = (end != buffer); //checks if the addresses are different

            while (*end == ' ' || *end == '\t')
                end++;

            if (hasDigits && *end == '\0' && value >= min && value <= max){
                choice = (int)value;
                valid = true;
            }
            
        }

        if (!valid)
            printf("Please enter a number between %d and %d inclusive.\n", min, max);
    }

    return choice;
}

static int selectNeighborhood(void){
    int total = getNeighborhoodCount();
    int *areaIDs = malloc(sizeof(int) * total);
    int *areaNeighborhoods = malloc(sizeof(int) * total);   //stores the neighborhood ids of the chosen area

    if (areaIDs == NULL || areaNeighborhoods == NULL){
        printf("Could not allocate memory.\n");
        free(areaIDs);
        free(areaNeighborhoods);
        return -1;
    }

    int areaCount = 0;
    bool isNew;
    for (int id =0; id<total; id++){
        isNew = true;
        for (int j = 0; j<id; j++){
            if (strcmp(getAreaName(j), getAreaName(id)) == 0)
                isNew = false;
        }
        if (isNew){
            areaIDs[areaCount] = id;
            areaCount++;
        }
        
    }
    int result = -1;
    bool done = false;
    while(!done){
        printf("\nSelect your area:\n");
        for (int a=0; a<areaCount; a++)
            printf("%2d) %s\n", a+1, getAreaName(areaIDs[a]));
        printf("0) Cancel\n");

        int area = getChoice(0, areaCount);
        if (area <= 0){
            done = true;
        }
        else{
            const char *areaName = getAreaName(areaIDs[area -1]);
            int count = 0;
            for (int id = 0; id<total; id++){
                if (strcmp(getAreaName(id), areaName) == 0){
                    areaNeighborhoods[count] = id;
                    count++;
                }
            }

            printf("\nSelect your neighborhood:\n");
            for (int i=0; i<count; i++)
                printf("%2d) %s\n", i+1, getNeighborhoodName(areaNeighborhoods[i]));
            printf("0) Back\n");

            int choice = getChoice(0, count);
            if (choice == -1)
                done = true;
            else if (choice > 0){
                result = areaNeighborhoods[choice - 1];
                done = true;
            }
        }
    }

    free(areaIDs);
    free(areaNeighborhoods);

    return result;
}

static int selectCause(void){
    printf("\nWhat is the problem?\n");
    for (int i=0; i<CAUSE_COUNT; i++)
        printf("%d) %s\n", i+1, getCauseName((Cause)i));
    printf("0) Cancel\n");

    int choice = getChoice(0, CAUSE_COUNT);

    if (choice <= 0) return -1;

    return choice-1;
}

static void submitReport(void){
    int id = selectNeighborhood();

    if (id < 0) return;

    int cause = selectCause();

    if (cause < 0) return;

    if (addReport(id, (Cause)cause) == 0){
        printf("\nReport recorded for %s, %s (%s).\n",
                getNeighborhoodName(id), getAreaName(id), getCauseName((Cause)cause));
    }
    else
        printf("\nReport could not be recorded.\n");
}

static void submitDescription(void){
    char text[302];
    bool valid = false;

    int id = selectNeighborhood();
    if (id < 0) return;

    while (!valid){
        printf("\nDescribe the problem (10 to 300 characters. Do not use '|'). Empty line to cancel.\n");

        int status = readLine(text, sizeof(text));
        if (status == 0 || (status == 1 && text[0] == '\0'))
            return;                

        if (status == -1)
            printf("Please use at most 300 characters.\n");
        else if (strchr(text, '|') != NULL)
            printf("The '|' character is not allowed.\n");
        else if (strlen(text) < 10)
            printf("Please write at least 10 characters.\n");
        else
            valid = true;
    }

    if (addDescription(id, text) == 0)
        printf("\nYour written complaint was saved.\n");
    else
        printf("\nYour complaint could not be saved.\n");    
}

void runMenu(void){
    int choice = 0;

    while (choice != 3 && choice != -1){
        printf("\n=== Water Shortage Reporting ===\n");
        printf("1) Submit a report\n");
        printf("2) Submit a written report\n");
        printf("3) Exit\n");

        choice = getChoice(1, 3);
        if (choice == 1)
            submitReport();
        else if (choice == 2)
            submitDescription();
    }
    printf("Goodbye.\n");
}