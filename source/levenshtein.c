#include "../headers/levenshtein.h"

/*$$
\mathrm{lev}_{a,b}(i,j) = 
\begin{cases} 
    \max(i,j) & \text{se } \min(i,j) = 0, \\
    \min \begin{cases}
        \mathrm{lev}_{a,b}(i-1,j) + 1, \\
        \mathrm{lev}_{a,b}(i,j-1) + 1, \\
        \mathrm{lev}_{a,b}(i-1,j-1) + \mathrm{cost}(a_i, b_j)
    \end{cases} & \text{caso contrário}.
\end{cases}
$$

onde a=str1, b=string
*/

#include <string.h>

int levenshtein_distance(const char *str1, const char *str2){
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);

    int mat[len1+1][len2+1];
    for (size_t i=0; i<=len1; i++)
        for (size_t j=0; j<=len2; j++)
            mat[i][j] = -1;

    return dist(0, 0, str1, str2, len1, len2, mat);
}

int dist(int i, int j, const char *str1, const char *str2, int len1, int len2, int mat[len1 + 1][len2 + 1]) {
    if (mat[i][j] >= 0) return mat[i][j]; // ja calc

    int x;
    if (i == len1) x = len2 - j; // final 1 e 2
    else if (j == len2) x = len1 - i;
    else {
        int cost = str1[i] == str2[j] ? 0 : 1;

        x = dist(i + 1, j + 1, str1, str2, len1, len2, mat) + cost;

        int y;
        if ((y = dist(i, j + 1, str1, str2, len1, len2, mat) + 1) < x) x = y;
        if ((y = dist(i + 1, j, str1, str2, len1, len2, mat) + 1) < x) x = y;
    }

    return mat[i][j] = x;
}
