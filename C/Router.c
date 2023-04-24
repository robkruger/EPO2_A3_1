#include <stdio.h>
#include <stdlib.h>

struct cell {
    int v;
    char name[8];
};

struct cell maze[13][13];

void initialize_maze(){
    int i, j;
    for(i = 0; i < 8; i++){
        for(j = 0; j < 8; j++){
            maze[i][j].v = -1;
        }
    }
}

void visualize_maze(){
    int i, j;
    for(i = 0; i < 8; i++){
        for(j = 0; j < 8; j++){
            printf("%d", maze[i][j].v);
        }
    }
}

int main(){
    initialize_maze();
    visualize_maze();
}