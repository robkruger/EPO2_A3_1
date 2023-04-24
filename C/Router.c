#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

struct cell {
    int v;
    char name[8];
};

struct cell maze[13][13];

int starting_station = 0;
int end_station = 0;

struct cell get_station(int station){
    if(station == 1){
        return maze[4][12];
    }
    else if(station == 2){
        return maze[6][12];
    }
    else if(station == 3){
        return maze[8][12];
    }
    else if(station == 4){
        return maze[12][8];
    }
    else if(station == 5){
        return maze[12][6];
    }
    else if(station == 6){
        return maze[12][4];
    }
    else if(station == 7){
        return maze[8][0];
    }
    else if(station == 8){
        return maze[6][0];
    }
    else if(station == 9){
        return maze[4][0];
    }
    else if(station == 10){
        return maze[0][4];
    }
    else if(station == 11){
        return maze[0][6];
    }
    else if(station == 12){
        return maze[0][8];
    }
}

struct cell get_crossing(int i, int j){
    int k, l;
    k = 2 + i * 2;
    l = 2 + j * 2;
    return maze[k][l];
}

// 0 - south, 1 - east, 2 - north, 3 - west
struct cell get_edge(int i, int j, int direction){
    int k, l;
    k = 2 + i * 2;
    l = 2 + j * 2;
    if(direction == 0){
        k += 1;
    }
    else if(direction == 1){
        l -= 1;
    }
    else if(direction == 2){
        k -= 1;
    }
    else if(direction == 3){
        l += 1;
    }
    return maze[k][l];
}

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
    // Initialize everything as an edge
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
    struct cell starting_cell = get_station(starting_station);
    struct cell end_cell = get_station(end_station);
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