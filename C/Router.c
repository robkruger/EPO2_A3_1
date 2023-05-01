#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

struct cell {
    int v; // Value
    int x, y; // Location in the maze
    char name[8]; // Name, not used at the moment
};

// Matrix represantation of the maze
struct cell maze[13][13];

struct path {
    struct cell path_array[100];
};

// Stations between which a path has to be found, -1 means there is no station.
int stations[]= {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

// Returns the cell corresponding to the according station
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

// Returns cell corresponding to the according crossing
struct cell get_crossing(int i, int j){
    int k, l;
    k = 2 + j * 2;
    l = 2 + i * 2;
    return maze[k][l];
}

// Returns cell corresponding to the according edge
// 0 - south, 1 - east, 2 - north, 3 - west
void change_edge(int i, int j, int direction, int v){
    int k, l;
    k = 2 + j * 2;
    l = 2 + i * 2;
    if(direction == 0){
        l += 1;
    }
    else if(direction == 1){
        k += 1;
    }
    else if(direction == 2){
        l -= 1;
    }
    else if(direction == 3){
        k -= 1;
    }
    maze[k][l].v = v;
}

int cells_equal(struct cell cell_1, struct cell cell_2){
    if(cell_1.x == cell_2.x && cell_1.y == cell_2.y){
        return 1;
    }
    return 0;
}

// Functions to print text in a certain color
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

void read_input(){
    int numofblock, i, j, k, ci, cj, dir_n; //variables
    char dir_l;
    j = 0;
    //scans for number of blockades
    scanf("%i", &numofblock);
    for(i=0; i<numofblock; i++){ //loops scan for blockade info, runs for amount of inputs
        scanf("%i %i %c", &ci,&cj,&dir_l);
        //make dir_n the number corresponding to direction
        if (dir_l == 's'){
            dir_n = 0;
        }
        else if (dir_l == 'w'){
            dir_n = 3;
        }
        else if (dir_l == 'n'){
            dir_n = 2;
        }
        else {
            dir_n = 1;
        }
        //function to get the respective edges and change them
        change_edge(ci, cj, dir_n, -1);
    }
    //scan for input stations, stop when newline  is detected
    char discard;
    while(j<11 && scanf("%d%1[^\n]s", &stations[j], &discard) == 2){
        j++;
    }

}



// Initialize the maze with random values
void initialize_maze_random(){
    int i, j;
    for(i = 0; i < 13; i++){
        for(j = 0; j < 13; j++){
            int r = rand() % 20 - 1;
            maze[i][j].v = r;
            maze[i][j].x = i;
            maze[i][j].y = j;
        }
    }
}

// Initialize the maze with a predefined path
void initialize_maze_test(){
    int i, j;
    // Initialize everything as an edge first
    for(i = 0; i < 13; i++){
        for(j = 0; j < 13; j++){
            maze[i][j].v = -1;
            maze[i][j].x = i;
            maze[i][j].y = j;
        }
    }
    // Set the traversable cells to 0
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
    maze[0][8].v = 1;
    maze[1][8].v = 2;
    maze[2][8].v = 3;
    maze[2][7].v = 4;
    maze[2][6].v = 5;
    maze[2][5].v = 6;
    maze[2][4].v = 7;
    maze[1][4].v = 8;
    maze[0][4].v = 9;
}

void initialize_maze(){
    int i, j;
    // Initialize everything as an edge first
    for(i = 0; i < 13; i++){
        for(j = 0; j < 13; j++){
            maze[i][j].v = -1;
            maze[i][j].x = i;
            maze[i][j].y = j;
        }
    }
    // Set the traversable cells to 0
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

// Find the shortest path from starting station to end station
struct path find_path(int start, int end){
    struct cell ending_cell = get_station(end);
    struct cell current_cell = get_station(start);
    struct path path_object;
    int j;
    for(j=0; j<100; j++){
        path_object.path_array[j].x = -1;
        path_object.path_array[j].y = -1;
    }

    int i = 0;
    while(!cells_equal(current_cell, ending_cell)){
        int next_v = current_cell.v + 1;
        int x = current_cell.x;
        int y = current_cell.y;

        path_object.path_array[i].x = current_cell.x;
        path_object.path_array[i].y = current_cell.y;

        if(maze[x + 1][y].v == next_v){
            current_cell = maze[x + 1][y];
        }
        else if(maze[x - 1][y].v == next_v){
            current_cell = maze[x - 1][y];
        }
        else if(maze[x][y + 1].v == next_v){
            current_cell = maze[x][y + 1];
        }
        else if(maze[x][y - 1].v == next_v){
            current_cell = maze[x][y - 1];
        }
        i++;
    }
    
    path_object.path_array[i].x = current_cell.x;
    path_object.path_array[i].y = current_cell.y;

    return path_object;
}

//this function gets the start and end stations+ ones in between, calls the getroute functions 
//for two stations at one time, and thus outputs a route for as much stations that is needed.
void makeroute(){
int station1, station2, i, j;
struct path path;
for(i=0; i<11; i++){
    if(stations[i+1] == -1){
        break;
        // when the input of list becomes -1, which means there are no more stations to visit,
        // stop the function.
    }
    station1 = stations[i];
    station2 = stations[i+1];
    path = find_path(station1,station2);
    for(j=0; path.path_array[j].x!=-1; j=j+2){
        printf("c%d%d ", path.path_array[j].x, path.path_array[j].y);
        //prints the crossings of the path
    }
    printf("\n");
}

}

// Output the path
// void output(){
//     struct cell starting_cell = get_station(starting_station);
//     struct cell end_cell = get_station(end_station);
// }

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
            if(maze[j][i].v != 0){
                if(maze[j][i].v != -1){
                    green();
                    nDigits = floor(log10(abs(maze[j][i].v))) + 1;
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
            printf("%d", maze[j][i].v);
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
    initialize_maze_test();
    read_input();
    makeroute();
    visualize_maze();
    return 0;
}