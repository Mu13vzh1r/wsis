#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "wsis.h"
#include "priority.h"

// change these to adjust the formula
#define W_SEVERITY    0.35
#define W_DENSITY     0.35
#define W_RECENCY     0.15
#define W_PERSISTENCE 0.15

#define DENSITY_CAP         1.0   // reports per 1000 residents that counts as "maximum"
#define RECENCY_SCALE_DAYS  3.0   // after this many days the recency factor halves
#define PERSISTENCE_CAP_DAYS 7.0  // a problem open this long counts as "maximum"

typedef struct {
    int id;
    double rating;
    int totalReports;
} Ranked;

// severity on a scale of 1 to 10
static int severityOf(Cause cause){
    switch (cause){
        case CAUSE_NO_SUPPLY:    return 10;
        case CAUSE_DIRTY_WATER:  return 8;
        case CAUSE_LOW_PRESSURE: return 5;
        case CAUSE_PIPE_LEAK:    return 4;
        case CAUSE_COUNT:        return 0;
    }
    return 0;
}

// rating from 1 to 10 for one neighborhood, or 0 if it has no reports
static double rateNeighborhood(int id, int *totalReports){
    *totalReports = 0;

    int pop = getPopulation(id);
    if (pop <= 0) return 0.0;                     // invalid id

    int total = 0;
    double severitySum = 0.0;
    long long newest = 0;                         // latest report of any cause
    long long oldest = 0;                         // earliest report of any cause

    for (int c = 0; c < CAUSE_COUNT; c++){
        int count = getReportCount(id, (Cause)c);
        if (count <= 0) continue;                 // no reports (or error)

        total += count;
        severitySum += (double)severityOf((Cause)c) * count;

        long long first = getFirstReported(id, (Cause)c);
        long long last  = getLastReported(id, (Cause)c);

        if (first > 0 && (oldest == 0 || first < oldest)) oldest = first;
        if (last > newest) newest = last;
    }

    if (total == 0) return 0.0;
    *totalReports = total;

    double severity = (severitySum / total) / 10.0;

    double density = ((double)total / pop * 1000.0) / DENSITY_CAP;
    if (density > 1.0) density = 1.0;

    time_t now = time(NULL);
    double daysSinceLast = difftime(now, (time_t)newest) / 86400.0;
    if (daysSinceLast < 0.0) daysSinceLast = 0.0;
    double recency = 1.0 / (1.0 + daysSinceLast / RECENCY_SCALE_DAYS);

    double daysOpen = difftime(now, (time_t)oldest) / 86400.0;
    double persistence = daysOpen / PERSISTENCE_CAP_DAYS;
    if (persistence > 1.0) persistence = 1.0;
    if (persistence < 0.0) persistence = 0.0;

    double combined = W_SEVERITY * severity
                    + W_DENSITY * density
                    + W_RECENCY * recency
                    + W_PERSISTENCE * persistence;

    return 1.0 + 9.0 * combined;                  // scale 0..1 to 1..10
}

// biggest rating goes first
static int compareRanked(const void *a, const void *b){
    const Ranked *x = a;
    const Ranked *y = b;
    if (y->rating > x->rating) return 1;
    if (y->rating < x->rating) return -1;
    return 0;
}

void showPriorityList(int topN){
    int n = getNeighborhoodCount();
    Ranked *list = malloc(sizeof(Ranked) * n);

    if (list == NULL){
        printf("Could not allocate memory.\n");
        return;
    }

    for (int id = 0; id < n; id++){
        list[id].id = id;
        list[id].rating = rateNeighborhood(id, &list[id].totalReports);
    }

    qsort(list, n, sizeof(Ranked), compareRanked);

    printf("\n=== Priority List ===\n");
    int shown = 0;
    for (int i = 0; i < n && shown < topN; i++){
        if (list[i].rating <= 0.0) break;         // the rest have no reports

        printf("%d) %s, %s - rating %.1f/10 (%d reports)\n", shown + 1,
               getNeighborhoodName(list[i].id), getAreaName(list[i].id),
               list[i].rating, list[i].totalReports);
        shown++;
    }
    if (shown == 0)
        printf("No reports yet.\n");

    free(list);
}
