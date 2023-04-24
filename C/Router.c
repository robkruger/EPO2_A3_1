#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

struct cell {
    int v;
    char name[8];
};

struct cell maze[13][13];

int starting_station = 0;
int end_station = 0;

#include <stdio.h>
void red(){
  printf("\033[0;31m");
}

void green(){
    printf("\033[0;32m");
}

void yellow(){
  printf("\033[0;33m");
}

void reset(){
  printf("\033[0m");
}

void initialize_maze_random(){
    int i, j;
    for(i = 0; i < 13; i++){
        for(j = 0; j < 13; j++){
            int r = rand() % 20 - 1;
            maze[i][j].v = r;
        }
    }
}

void initialize_maze(){
    int i, j;
    for(i = 0; i < 13; i++){
        for(j = 0; j < 13; j++){
            maze[i][j].v = -1;
        }
    }
    for(i = 0; i < 13; i++){
        maze[i][4].v = 0;
        maze[i][6].v = 0;
        maze[i][8].v = 0;
        maze[4][i].v = 0;
        maze[6][i].v = 0;
        maze[8][i].v = 0;
    }
    for(i = 2; i < 11; i++){
        maze[i][2].v = 0;
        maze[i][10].v = 0;
        maze[2][i].v = 0;
        maze[10][i].v = 0;
    }
}

void output(){
    
}

void visualize_maze(){
    int i, j;
    for(j = 0; j < 13; j++){
        printf("-----");
    }
    printf("-\n");
    for(i = 0; i < 13; i++){
        printf("! ");
        for(j = 0; j < 13; j++){
            int nDigits = 1;
            if(maze[i][j].v != 0){
                if(maze[i][j].v != -1){
                    green();
                    nDigits = floor(log10(abs(maze[i][j].v))) + 1;
                }
                else{
                    red();
                    nDigits = 2;
                }
            }
            else {
                yellow();
            }
            if(nDigits != 2){
                printf(" ");
            }
            printf("%d", maze[i][j].v);
            reset();
            printf(" ! ");
        }
        printf("\n");
        for(j = 0; j < 13; j++){
            printf("-----");
        }
        printf("-");
        printf("\n");
    }
}

int main(){
    srand(time(NULL));
    initialize_maze();
    visualize_maze();
}