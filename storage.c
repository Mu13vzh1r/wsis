#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "wsis.h"


typedef struct{
    int id;
    char neighborhood[100];
    char area[100];
    int population;
}Regions;

typedef struct{
    int counts[CAUSE_COUNT];
    long long firstReport[CAUSE_COUNT];
    long long lastReport[CAUSE_COUNT];
}Record;

static int loadNeighborhoods(const char *file, Regions **ActualList);

static Regions *globalNeighborhoods = NULL;
static Record *records = NULL;
static int neighborhoodCount = 0;


static int loadNeighborhoods(const char *file, Regions **ActualList){
    int elements = 2, count = 0; //elements is the size we allocate, count is the amount of items
    Regions *temp;
    Regions *Neighborhoods = calloc(elements, sizeof(Regions));

    if (Neighborhoods == NULL){
        printf("Memory allocation failed.\n");
        return -1;
    }

    FILE *f_ptr;
    f_ptr = fopen(file, "r");

    if (f_ptr == NULL){
        printf("Error opening file.\n");
        free(Neighborhoods);
        return -1;
    }

    int n;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), f_ptr) != NULL){
        if (count >= elements){
            elements*=2;
            temp = realloc(Neighborhoods, elements*sizeof(Regions));
            if (temp == NULL){
                printf("Reallocation failed. Could not store all data.\n");
                free(Neighborhoods);
                fclose(f_ptr);
                return -1;

            }
            else{
                Neighborhoods = temp;
                temp = NULL;
            }
        }
            
        n = sscanf(buffer, "%d,%99[^,],%99[^,],%d", &Neighborhoods[count].id,
                   Neighborhoods[count].neighborhood, Neighborhoods[count].area, &Neighborhoods[count].population);

        if (n != 4){
            printf("Line %d could not read 4 fields. Fix format error(s) in neighborhoods.txt.\n", count+1);
            free(Neighborhoods);
            fclose(f_ptr);
            return -1;
        }
        
        if (Neighborhoods[count].id != count){
            printf("Error! ID %d should not be at line %d.\n", Neighborhoods[count].id, count+1);
            free(Neighborhoods);
            fclose(f_ptr);
            return -1;
        }
        if (Neighborhoods[count].population <= 0){
            printf("Invalid population count at line %d.\n", count+1);
            free(Neighborhoods);
            fclose(f_ptr);
            return -1;
        }
        count++;
    }           
    
    fclose(f_ptr);

    if (count == 0){
        printf("Error, Empty File!\n");
        free(Neighborhoods);
        return -1;
    }

    *ActualList = Neighborhoods;

    return count;
}

int addReport(int id, Cause cause){
    long long now = (long long)(time(NULL));
    if (records == NULL || id >= neighborhoodCount || id < 0 || cause < 0 || cause >= CAUSE_COUNT){
        return -1;
    }

    Record *currRecord = &records[id];
    Record backup = *currRecord; 

    currRecord->counts[cause]++;
    if (currRecord->firstReport[cause] == 0)
        currRecord->firstReport[cause] = now;
    currRecord->lastReport[cause] = now;
        
    FILE *fPtr = fopen("reports.bin", "rb+");
    if (fPtr == NULL){
        printf("Problem opening file.\n");
        *currRecord = backup;
        return -1;
    }
    
    if (fseek(fPtr, id*sizeof(Record), SEEK_SET) != 0){
        printf("Could not reach this point in file.\n");
        *currRecord = backup;
        fclose(fPtr);
        return -1;
    }
    
    if (fwrite(currRecord, sizeof(Record), 1, fPtr) != 1){
        printf("Could not write to file correctly.\n");
        *currRecord = backup;
        fclose(fPtr);
        return -1;
    }

    if (fclose(fPtr) != 0){
        fPtr = NULL;
        printf("Could not close the file.\n");
        *currRecord = backup;
        return -1;
    }

    return 0;
}
    
int storageInit(void){
    size_t size;
    long bytes;
    if (records != NULL)
        return 0; 

    int count = loadNeighborhoods("neighborhoods.txt", &globalNeighborhoods);
    if (count < 0){
        printf("COULD NOT LOAD NEIGHBORHOODS!\n");
        free(globalNeighborhoods);
        globalNeighborhoods = NULL;
        return -1;
    }

    records = calloc(count, sizeof(Record));
    if (records == NULL){
        printf("COULD NOT ALLOCATE MEMORY TO RECORDS.\n");
        free(globalNeighborhoods);
        globalNeighborhoods = NULL;
        return -1;
    }

    FILE *fPtr = fopen("reports.bin", "rb+");
    if (fPtr == NULL){
        fPtr = fopen("reports.bin", "wb");

        if (fPtr == NULL){
            printf("Could not create file.\n");
            goto cleanup;
        }
        

        if (fwrite(records, sizeof(Record), count, fPtr) != (size_t)count){
            printf("Could not write to file.\n");
            goto cleanup;
        }
    }
    else{
        if (fseek(fPtr, 0, SEEK_END) != 0 || (bytes = ftell(fPtr)) < 0 ||
            bytes % (long)sizeof(Record) != 0){
                printf("reports.bin seems to be damaged. Delete it to start fresh.\n");
                goto cleanup;
        }
        size = (size_t)bytes / sizeof(Record);

        if ((size_t)count < size){
            printf("Unexpected behavior. Neighborhoods have been deleted.\n");
            goto cleanup; 
        }           
        else if ((size_t)count > size){
            if (fwrite(records, sizeof(Record), count-size, fPtr) != count-size){
                printf("Could not write to file.\n");
                goto cleanup;
            }         
        }

        fseek(fPtr, 0, SEEK_SET);        
        if (fread(records, sizeof(Record), size, fPtr) != size){
            printf("Could not read from file.\n");
            goto cleanup;
        }
    }

    if (fclose(fPtr) != 0){ 
        fPtr = NULL; 
        printf("Problem closing file.\n"); 
        goto cleanup; 
    }

    neighborhoodCount = count;

    return 0;

    cleanup:
        if (fPtr != NULL) fclose(fPtr);
        free(globalNeighborhoods);
        free(records);
        globalNeighborhoods = NULL;
        records = NULL;       

        return -1;
}

void storageShutdown(void){
    free(globalNeighborhoods);
    free(records);
    globalNeighborhoods = NULL;
    records = NULL;
    
    neighborhoodCount = 0;
}

int getNeighborhoodCount(void){
    return neighborhoodCount;
}

int getPopulation(int id){
    if (records == NULL || id < 0 || id >= neighborhoodCount) 
        return -1;

    return globalNeighborhoods[id].population;
}

int getReportCount(int id, Cause cause){
    if (records == NULL || id < 0 || id >= neighborhoodCount || cause < 0 || cause >= CAUSE_COUNT) 
        return -1;

    return ((records[id].counts)[cause]);
}

long long getFirstReported(int id, Cause cause){
    if (records == NULL || id < 0 || id >= neighborhoodCount || cause < 0 || cause >= CAUSE_COUNT) 
        return -1;
   
    return ((records[id].firstReport)[cause]);
}

long long getLastReported(int id, Cause cause){
    if (records == NULL || id < 0 || id >= neighborhoodCount || cause < 0 || cause >= CAUSE_COUNT) 
        return -1;
   
    return ((records[id].lastReport)[cause]);
}

const char *getNeighborhoodName(int id){
    if (records == NULL || id < 0 || id >= neighborhoodCount) 
        return "";
    
    return globalNeighborhoods[id].neighborhood;
}

const char *getAreaName(int id){
    if (records == NULL || id < 0 || id >= neighborhoodCount) 
        return "";
    
    return globalNeighborhoods[id].area;
}

const char *getCauseName(Cause cause){
    if (cause < 0 || cause > CAUSE_COUNT) 
        return "";    

    switch (cause){
        case CAUSE_NO_SUPPLY:
            return "No Supply";
        
        case CAUSE_LOW_PRESSURE:
            return "Low Pressure";
            
        case CAUSE_DIRTY_WATER:
            return "Dirty Water";
        
        case CAUSE_PIPE_LEAK:
            return "Pipe Leak";
        
        case CAUSE_COUNT:
            return "";
    }

    return "";

}

int addDescription(int id, const char *text){
    const char *neighborhoodName = getNeighborhoodName(id);
    const char *areaName = getAreaName(id);
    time_t raw_time = 0;
    struct tm *currTime = NULL;
    time(&raw_time);
    currTime = localtime(&raw_time);
    char timestamp[32];

    FILE *fPtr = NULL;

    if (strcmp(neighborhoodName, "") != 0 && strcmp(areaName, "") != 0){

        fPtr = fopen("descriptions.txt", "a");
        if (fPtr == NULL) goto cleanup;
        
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", currTime);

        if (fprintf(fPtr, "%d|%s|%s|%s|%s\n", id, timestamp, neighborhoodName, areaName, text) < 0)
            goto cleanup;         
        
        if (fclose(fPtr) != 0){
            fPtr = NULL;
            goto cleanup;
        }
        return 0;
    }

    cleanup:
        printf("Error! Couldn't write descriptive report.\n");
        if (fPtr != NULL) fclose(fPtr);
        return -1;
}