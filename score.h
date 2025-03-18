#ifndef SCORE_H
#define SCORE_H

int const MAX_SCORES = 5;
int const NAME_LEN = 20;

struct ScoreEntry
{
    char name[NAME_LEN];
    int linesCleared;
};

ScoreEntry *getScores();

void loadScoresFromFile();

void saveScoresToFile();

void insertScore(const char *name, const int linesCleared);

#endif // !SCORE_H
