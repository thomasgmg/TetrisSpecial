#include "score.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char const *SCORE_FILE = "scores.txt";

static ScoreEntry scores[MAX_SCORES + 1];

ScoreEntry *getScores()
{
    return scores;
}

void loadScoresFromFile()
{
    FILE *file = fopen(SCORE_FILE, "r");
    if (!file)
    {
        // If file doesn't exist, initialize with empty values
        for (int i = 0; i < MAX_SCORES; i++)
        {
            strcpy(scores[i].name, "Empty");
            scores[i].linesCleared = 0;
        }
        return;
    }
    for (int i = 0; i < MAX_SCORES; i++)
    {
        if (fscanf(file, "%19s %d", scores[i].name, &scores[i].linesCleared) != 2)
        {
            // Default values in case of a read error
            strcpy(scores[i].name, "Empty");
            scores[i].linesCleared = 0;
        }
    }
    fclose(file);

    return;
}

void saveScoresToFile()
{
    FILE *file = fopen(SCORE_FILE, "w");
    if (!file)
    {
        return;
    }
    for (int i = 0; i < MAX_SCORES; i++)
    {
        fprintf(file, "%s %d\n", scores[i].name, scores[i].linesCleared);
    }
    fclose(file);
}

void insertScore(const char *name, const int linesCleared)
{
    ScoreEntry new_entry;
    strncpy(new_entry.name, name, NAME_LEN - 1);
    new_entry.name[NAME_LEN - 1] = '\0'; // Ensure null termination
    new_entry.linesCleared = linesCleared;

    scores[MAX_SCORES] = new_entry; // Add new score at the end

    // Sort the scores (Bubble sort-like insertion)
    for (int i = MAX_SCORES; i > 0; i--)
    {
        if (scores[i].linesCleared > scores[i - 1].linesCleared)
        {
            ScoreEntry temp = scores[i];
            scores[i] = scores[i - 1];
            scores[i - 1] = temp;
        }
    }
}

/* Usage example
int main()
{
    loadScoresFromFile();

    insertScore("PMG", 60);
    insertScore("Thomas", 70);

    saveScoresToFile();

    ScoreEntry* latestScores = getScores();

    printf("Top %d Scores:\n", MAX_SCORES);
    for (int i = 0; i < MAX_SCORES; i++)
    {
        printf("%d. %s - %d\n", i + 1, latestScores[i].name, latestScores[i].score);
    }
 }
*/
