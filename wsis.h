#ifndef WSIS_H
#define WSIS_H

typedef enum{
    CAUSE_NO_SUPPLY,
    CAUSE_LOW_PRESSURE,
    CAUSE_DIRTY_WATER,
    CAUSE_PIPE_LEAK,
    CAUSE_COUNT
}Cause;

//returns the total number of neighborhoods
int getNeighborhoodCount(void);

//All of the below functions return -1 for an error.

//returns the population for the neighborhood with the corresponding id.
int getPopulation(int id);

//returns the number of reports of a specific problem in a neighborhood. 
int getReportCount(int id, Cause cause);

//returns the timestamp of the first report on an issue in a neighborhood.
long long getFirstReported(int id, Cause cause);

//returns the timestamp of the last report on an issue in a neighborhood. 
long long getLastReported(int id, Cause cause);

//All of the functions below return an empty string if an error occurs.

//returns a string for the name of the neighborhood
const char *getNeighborhoodName(int id);

//returns a string for the name of the area in which the neighborhood is located
const char *getAreaName(int id);

//returns the name as a string for the enum cause being passed
const char *getCauseName(Cause cause);

//the functions below are only for input.c and main.c

//loads the data files. returns 0 on success, -1 on failure
int storageInit(void);

//frees everything. call once before the program exits
void storageShutdown(void);

//records one report. returns 0 on success, -1 if the id or cause is invalid
int addReport(int id, Cause cause);

//writes the descriptive report by a user into descriptions.txt. Returns 0 on success and -1 on failure.
int addDescription(int id, const char *text);

#endif