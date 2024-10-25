#ifndef LEVENSHTEIN_H
#define LEVENSHTEIN_H

#include <string.h>

int levenshtein_distance(const char *str1, const char *str2);
int dist(int i, int j, const char *str1, const char *str2, int len1, int len2, int mat[len1 + 1][len2 + 1]);

#endif 
