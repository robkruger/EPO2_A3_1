/************************************************
    Written by group A3-1
    Jesse Koert, Rob Kruger, Tom Rietjens
    C Code for the Robot of EPO-2 
************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <Windows.h>
#include <assert.h>
#include <pthread.h>

#define COMPORT "COM2"
#define BAUDRATE CBR_9600
#define true 1
#define false 0


/********** Declaring constants and globals **************/
const int GRID_SIZE = 13;
const int MINES = 12;

int DONE_LEE = false;
int LEE_TO_START = false;
int VISUALIZE_LEE = false;
int TREASURES = 0;

HANDLE hSerial;
//variables
char character[32];

// 0 = North, 1 = East, 2 = South, 3 = West
int direction;

/********** Declaring Structs and methods ****************/
struct cell {
    // Value
    int v; 
    int lee_v;
    int lee_target;
    // Location in the maze
    int x, y; 
    int mine;
    int found;
};

// Matrix represantation of the maze
struct cell maze[13][13];

struct cell mines[40];
int number_of_mines = 0;
int found_mines = 0;

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

int cells_equal(struct cell cell_1, struct cell cell_2){
    if(cell_1.x == cell_2.x && cell_1.y == cell_2.y){
        return 1;
    }
    return 0;
}

struct robot {
    int x, y;
    int direction;
    int found;
};

struct robot robot;


/********** Utility Functions ****************************/
void gotoxy(int column, int line){
    COORD coord;
    coord.X = column;
    coord.Y = line;
    SetConsoleCursorPosition(
        GetStdHandle( STD_OUTPUT_HANDLE ),
        coord
    );
}

void debug(char *message){
    gotoxy(0, 28);
    printf("\33[2K\r");
    gotoxy(0, 28);
    printf(message);
    Sleep(5);
}


/********** Coloured output text functions ***************/
void red(){
  printf("\033[0;31m");
}

void green(){
    printf("\033[0;32m");
}

void yellow(){
  printf("\033[0;33m");
}

void blue(){
  printf("\033[0;34m");
}

void purple(){
  printf("\033[0;35m");
}

void cyan(){
  printf("\033[0;36m");
}

void reset(){
  printf("\033[0m");
}

/********** Maze *****************************************/
void initialize_maze(){
    int i, j;
    // Initialize everything as an edge first
    for(i = 0; i < 13; i++){
        for(j = 0; j < 13; j++){
            maze[i][j].v = -99;
            maze[i][j].lee_v = -1;
            maze[i][j].x = i;
            maze[i][j].y = j;
            maze[i][j].mine = false;
            maze[i][j].found = false;
        }
    }
    // Set the traversable cells to 0
    for(i = 2; i < 11; i++){
        maze[i][4].v = 0;
        maze[i][6].v = 0;
        maze[i][8].v = 0;
        maze[4][i].v = 0;
        maze[6][i].v = 0;
        maze[8][i].v = 0;
        maze[i][4].lee_v = 0;
        maze[i][6].lee_v = 0;
        maze[i][8].lee_v = 0;
        maze[4][i].lee_v = 0;
        maze[6][i].lee_v = 0;
        maze[8][i].lee_v = 0;
    }
    for(i = 2; i < 11; i++){
        maze[i][2].v = 0;
        maze[i][10].v = 0;
        maze[2][i].v = 0;
        maze[10][i].v = 0;
        maze[i][2].lee_v = 0;
        maze[i][10].lee_v = 0;
        maze[2][i].lee_v = 0;
        maze[10][i].lee_v = 0;
    }
    // maze[2][11].v = 0;
    // maze[2][12].v = 1;
    // for(i = 2; i <= 10; i = i + 2){
    //     for(j = 2; j <= 10; j = j + 2){
    //         maze[j][i].v = 1;
    //     }
    // }
    if(number_of_mines != 0){
        int i;
        for(i = 0; i < number_of_mines; i++){
            int x = mines[i].x;
            int y = mines[i].y;
            maze[x][y].mine = true;
        }
    }
}

void lee(int to_start){
    int counter = 1;
    int x, y;
    for(x = 0; x < GRID_SIZE; x++){
        for(y = 0; y < GRID_SIZE; y++){
            if(maze[x][y].lee_v > 0){
                maze[x][y].lee_v = 0;
            }
        }
    }
    maze[robot.x][robot.y].lee_v = counter;
    int k = 1000;
    while(k > 0){
        if(counter > 50){
            perror("Got stuck in Lee!");
        }
        int i, j;
        for(i = 0; i < GRID_SIZE; i++){
            for(j = 0; j < GRID_SIZE; j++){
                if(maze[i][j].lee_v == counter){
                    if(maze[i + 1][j].lee_v == 0){
                        maze[i + 1][j].lee_v = counter + 1;
                    }
                    if(maze[i - 1][j].lee_v == 0){
                        maze[i - 1][j].lee_v = counter + 1;
                    }
                    if(maze[i][j + 1].lee_v == 0){
                        maze[i][j + 1].lee_v = counter + 1;
                    }
                    if(maze[i][j - 1].lee_v == 0){
                        maze[i][j - 1].lee_v = counter + 1;
                    }
                }
            }
        }
        k = 0;
        for(x = 0; x < GRID_SIZE; x++){
            for(y = 0; y < GRID_SIZE; y++){
                if(maze[x][y].lee_v == 0){
                    k += 1;
                }
            }
        }
        counter++; 
    } 
    if(to_start){
        maze[4][10].lee_target = true;
        LEE_TO_START = true;
    }
    else{
        int smallest_distance = 999;
        struct cell current_target = (const struct cell){ 0 };
        int target_x, target_y;
        for(x = 0; x < GRID_SIZE; x++){
            for(y = 0; y < GRID_SIZE; y++){
                if(x == robot.x && y == robot.y){
                    continue;
                }
                if(maze[x][y].v == 0){
                    if(maze[x][y].lee_v < smallest_distance && maze[x][y].lee_v > 0){
                        smallest_distance = maze[x][y].lee_v;
                        target_x = x;
                        target_y = y;
                    }
                }
            }
        }
        maze[target_x][target_y].lee_target = true;
        DONE_LEE = true;
    }
}

int follow_lee(){
    int x, y;
    int current_lee = -1;
    int last_direction = -1;
    int done = false;
    int i, j;
    for(i = 0; i < GRID_SIZE; i++){
        for(j = 0; j < GRID_SIZE; j++){
            if(maze[i][j].lee_target){
                x = i;
                y = j;
                current_lee = maze[i][j].lee_v;
                done = true;
            }
            if(done){
                break;
            }
        }
        if(done){
            break;
        }
    }

    while(current_lee > maze[robot.x][robot.y].lee_v){
        if(maze[x + 1][y].lee_v == current_lee - 1){
            last_direction = 3;
            x += 1;
        }
        else if(maze[x - 1][y].lee_v == current_lee - 1){
            last_direction = 1;
            x -= 1;
        }
        else if(maze[x][y + 1].lee_v == current_lee - 1){
            last_direction = 0;
            y += 1;
        }
        else if(maze[x][y - 1].lee_v == current_lee - 1){
            last_direction = 2;
            y -= 1;
        }
        current_lee -= 1;
    }
    return last_direction;
}

void* visualize_maze(void *arg){
    while(1){
        gotoxy(0,1);
        if(VISUALIZE_LEE){
            int i, j;
            printf("------------------------------------------------------------------------------\n");
            for(i = 0; i < 13; i++){
                printf("! ");
                for(j = 0; j < 13; j++){
                    int nDigits = 1;
                    if(maze[j][i].lee_v != 0){
                        if(maze[j][i].lee_v >= 0){
                            if(robot.x == j && robot.y == i){
                                blue();
                            }
                            else if(maze[j][i].v != 0){
                                green();
                            }
                            else{
                                cyan();
                            }
                            nDigits = floor(log10(abs(maze[j][i].lee_v))) + 1;
                        }
                        else{
                            if(robot.x == j && robot.y == i){
                                blue();
                            }
                            else if(maze[j][i].lee_v == -99){
                                red();
                            }
                            else if(maze[j][i].lee_v == -98){
                                cyan();
                            }
                            else {
                                purple();
                            }
                            nDigits = floor(log10(abs(maze[j][i].lee_v))) + 1;
                            if(maze[j][i].lee_v > -10){
                                nDigits++;
                            }
                        }
                    }
                    else {
                        if(robot.x == j && robot.y == i){
                            blue();
                        }
                        else{
                            yellow();
                        }
                    }
                    if(robot.x == j && robot.y == i){
                        printf(" ");
                        if(robot.direction == 0){
                            printf("^");
                        } else if(robot.direction == 1) {
                            printf(">");
                        } else if(robot.direction == 2) {
                            printf("v");
                        } else {
                            printf("<");
                        }
                        printf(" ");
                    } else {
                        if(nDigits == 1){
                            printf(" ");
                        }
                        printf("%d", maze[j][i].lee_v);
                        if(maze[j][i].lee_v > -10){
                            printf(" ");
                        }
                    }
                    reset();
                    printf(" ! ");
                }
                printf("\n");
                for(j = 0; j < 13; j++){
                    printf("------");
                }
                printf("-");
                printf("\n");
            }
        }
        else{
            int i, j;
            printf("------------------------------------------------------------------------------\n");
            for(i = 0; i < 13; i++){
                printf("! ");
                for(j = 0; j < 13; j++){
                    int nDigits = 1;
                    if(maze[j][i].v != 0){
                        if(maze[j][i].v >= 0){
                            if(robot.x == j && robot.y == i){
                                blue();
                            }
                            else if(maze[j][i].mine && !maze[j][i].found){
                                cyan();
                            }
                            else if(maze[j][i].mine && maze[j][i].found){
                                green();
                            }
                            else{
                                green();
                            }
                            nDigits = floor(log10(abs(maze[j][i].v))) + 1;
                        }
                        else{
                            if(robot.x == j && robot.y == i){
                                blue();
                            }
                            else if(maze[j][i].v == -99){
                                red();
                            }
                            else if(maze[j][i].mine && !maze[j][i].found){
                                cyan();
                            }
                            else if(maze[j][i].mine && maze[j][i].found){
                                green();
                            }
                            else {
                                purple();
                            }
                            nDigits = floor(log10(abs(maze[j][i].v))) + 1;
                            if(maze[j][i].v > -10){
                                nDigits++;
                            }
                        }
                    }
                    else {
                        if(robot.x == j && robot.y == i){
                            blue();
                        }
                        else if(maze[j][i].mine && !maze[j][i].found){
                            cyan();
                        }
                        else if(maze[j][i].mine && maze[j][i].found){
                            green();
                        }
                        else{
                            yellow();
                        }
                    }
                    if(robot.x == j && robot.y == i){
                        if(robot.direction == 0){
                            printf(" ^ ");
                        } else if(robot.direction == 1) {
                            printf(" > ");
                        } else if(robot.direction == 2) {
                            printf(" v ");
                        } else {
                            printf(" < ");
                        }
                    } else {
                        if(nDigits == 1 && !maze[j][i].mine){
                            printf(" ");
                        }
                        if(!maze[j][i].mine){
                            printf("%d", maze[j][i].v);
                        } else {
                            printf("%d", -99);
                        }
                        if(maze[j][i].v > -10 && !maze[j][i].mine){
                            printf(" ");
                        }
                    }
                    reset();
                    printf(" ! ");
                }
                printf("\n");
                printf("-------------------------------------------------------------------------------\n");
            }
        }
    }
    return NULL;
}

void found_mine(int x, int y, int direction){ //?
    if(direction == 0){
        y -= 1;
    }
    else if(direction == 1){
        x += 1;
    }
    else if(direction == 2){
        y += 1;
    }
    else if(direction == 3){
        x -= 1;
    }
    maze[x][y].v = -99;
    maze[x][y].lee_v = -1;
    maze[x][y].mine = true;
    maze[x][y].found = true;
    robot.found++;
    mines[number_of_mines] = maze[x][y];
    number_of_mines++;
}

//this function will update the robot position in the maze accordingly to the command.
void update_robot_position(int command, int step){
    if (command == 0){
        if(robot.direction == 0){
            robot.y = robot.y - step;
        } else if (robot.direction == 1){
            robot.x = robot.x + step;
        } else if (robot.direction == 2){
            robot.y = robot.y + step;
        } else if (robot.direction == 3){
            robot.x = robot.x - step;
        }
    } else if (command == 1){ //left
        if(robot.direction == 0){
            robot.x = robot.x - step;
            robot.direction = 3;
        } else if (robot.direction == 1){
            robot.y = robot.y - step;
            robot.direction = 0;
        } else if (robot.direction == 2){
            robot.x = robot.x + step;
            robot.direction = 1;
        } else if (robot.direction == 3){
            robot.y = robot.y + step;
            robot.direction = 2;
        }
    } else if (command == 2){ //right
        if(robot.direction == 0){
            robot.x = robot.x + step;
            robot.direction = 1;
        } else if (robot.direction == 1){
            robot.y = robot.y + step;
            robot.direction = 2;
        } else if (robot.direction == 2){
            robot.x = robot.x - step;
            robot.direction = 3;
        } else if (robot.direction == 3){
            robot.y = robot.y - step;
            robot.direction = 0;
        }
    } else if (command == 3){
        if(robot.direction == 0){
            robot.y = robot.y + step;
            robot.direction = 2;
        } else if (robot.direction == 1){
            robot.x = robot.x - step;
            robot.direction = 3;
        } else if (robot.direction == 2){
            robot.y = robot.y - step;
            robot.direction = 0;
        } else if (robot.direction == 3){
            robot.x = robot.x + step;
            robot.direction = 1;
        } 
    }
}

/********* Wireless communication ************************/

//--------------------------------------------------------------
// Function: initSio
// Description: intializes the parameters as Baudrate, Bytesize, 
//           Stopbits, Parity and Timeoutparameters of
//           the COM port
//--------------------------------------------------------------
void initSio(HANDLE hSerial){

    COMMTIMEOUTS timeouts ={0};
    DCB dcbSerialParams = {0};

    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams)) {
        //error getting state
        printf("error getting state \n");
    }

    dcbSerialParams.BaudRate = BAUDRATE;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity   = NOPARITY;

    if(!SetCommState(hSerial, &dcbSerialParams)){
        //error setting serial port state
        printf("error setting state \n");
    }

    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;

    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if(!SetCommTimeouts(hSerial, &timeouts)){
    //error occureed. Inform user
        printf("error setting timeout state \n");
    }
}

//--------------------------------------------------------------
// Function: readByte
// Description: reads a single byte from the COM port into
//              buffer buffRead
//--------------------------------------------------------------
int readByte(HANDLE hSerial, char *buffRead) {

    DWORD dwBytesRead = 0;

    if (!ReadFile(hSerial, buffRead, 1, &dwBytesRead, NULL))
    {
        printf("error reading byte from input buffer \n");
    }
    //printf("Byte read from read buffer is: %c \n", buffRead[0]);
    return(buffRead[0]);
}

//--------------------------------------------------------------
// Function: writeByte
// Description: writes a single byte stored in buffRead to
//              the COM port 
//--------------------------------------------------------------
int writeByte(HANDLE hSerial, char *buffWrite){

    DWORD dwBytesWritten = 0;

    if (!WriteFile(hSerial, buffWrite, 1, &dwBytesWritten, NULL))
    {
        printf("error writing byte to output buffer \n");
    }
    printf("Byte written to write buffer is: %c \n", buffWrite[0]);

    return(0);
}

//these functions can be called for the instructions to be send to the robot
//also a handshake functionality is implementec
void send_command_to_robot(int command){

    if(command == 0){ 
        //go forward
        writeByte(hSerial, "A");
        while(1){
            readByte(hSerial, character);
            Sleep(100);
            if (strcmp(character, "R") == 0){
                break;
            }       
        }
    } 
    else if (command == 1){ // go left
        writeByte(hSerial, "B");
        while(1){
            readByte(hSerial, character);
            Sleep(100);
            if (strcmp(character, "S") == 0){
                break;
            }
        }
    } 
    else if (command == 2){ // go right
        writeByte(hSerial, "C");
        while(1){
            readByte(hSerial, character);
            Sleep(100);
            if (strcmp(character, "T") == 0){
                break;
            }
        }
    } 
    else if (command == 3){
        writeByte(hSerial, "D");
        while(1){
            readByte(hSerial, character);
            Sleep(100);
            if (strcmp(character, "U") == 0){
                break;
            }
        }
    }
    else if (command == 4){ // stop
        writeByte(hSerial, "E");
        while(1){
            readByte(hSerial, character);
            Sleep(100);
            if (strcmp(character, "V") == 0){
                break;
            }
        }
    }
}

// listens to response from robot and returns what happened, 0 means unknown command, 1 means robot successfully completed command,
// 2 means it found a mine and a new path needs to be calculated.
int listen_to_robot(int command, int part){
    while(1){
        readByte(hSerial, character);
        if (strcmp(character, "Q") == 0){
            if(part == 2){
                TREASURES++;
                if(TREASURES == 3){
                    writeByte(hSerial, "E");
                    return 3;
                }
                else{
                    writeByte(hSerial, "M");
                }
            }
            else{
                writeByte(hSerial, "Z");
            }
            if(command == 1){
                if(robot.direction == 0){
                    robot.direction = 3;
                } else {
                    robot.direction--;
                }
            } else if (command == 2){
                if (robot.direction == 3){
                    robot.direction = 0;
                } else {
                robot.direction++;
                }
            } else if (command == 3){
                robot.direction = (robot.direction + 2) % 4;
            }
            found_mine(robot.x, robot.y, robot.direction);
            robot.direction = (robot.direction + 2) % 4; // Turn around
            return 2;
        }
        if (strcmp(character, "X") == 0) {
            writeByte(hSerial, "Z");
            return 1;
        }
        Sleep(1);
    }
}

int max_array(int *neighbours, size_t size) {
    /* enforce the contract */
    assert(neighbours && size);
    size_t i;
    int max_value = neighbours[0];

    for (i = 1; i < size; ++i) {
        if (neighbours[i] > max_value) {
            max_value = neighbours[i];
        }
    }
    return max_value;
}

int get_neighbour(int x, int y, int direction){
    switch(direction){
        case 0:
            return maze[x][y - 1].v;
        case 1:
            return maze[x + 1][y].v;
        case 2:
            return maze[x][y + 1].v;
        case 3:
            return maze[x - 1][y].v;
    }
}

int index_array(int *neighbours, int value){
    int k;
    for(k = 0; k < 4; k++){
        if(neighbours[k] == value){
            switch(k){
                case 0:
                    return 1;
                case 1:
                    return 0;
                case 2:
                    return 3;
                case 3:
                    return 2;
            }
        }
    }
}

int best_direction_simple(int x, int y, int direction){
    int neighbours_0[4] = {maze[x + 1][y].v, maze[x][y - 1].v, maze[x - 1][y].v, maze[x][y + 1].v};
    int neighbours_1[4] = {maze[x][y + 1].v, maze[x + 1][y].v, maze[x][y - 1].v, maze[x - 1][y].v};
    int neighbours_2[4] = {maze[x - 1][y].v, maze[x][y + 1].v, maze[x + 1][y].v, maze[x][y - 1].v};
    int neighbours_3[4] = {maze[x][y - 1].v, maze[x - 1][y].v, maze[x][y + 1].v, maze[x + 1][y].v};
    switch(direction){
        case 0:
            return index_array(neighbours_0, max_array(neighbours_0, 4));
        case 1:
            return (index_array(neighbours_1, max_array(neighbours_1, 4)) + 1) % 4;
        case 2:
            return (index_array(neighbours_2, max_array(neighbours_2, 4)) + 2) % 4;
        case 3:
            return (index_array(neighbours_3, max_array(neighbours_3, 4)) + 3) % 4;
    }
}

int best_direction(int x, int y, int direction){
    int i, j;
    int north = -99;
    int east = -99;
    int south = -99;
    int west = -99;
    if(maze[x][y - 1].v != -99){
        int neighbours[3] = {maze[x + 1][y - 2].v, maze[x][y - 3].v, maze[x - 1][y - 2].v};
        north = maze[x][y - 1].v + maze[x][y - 2].v + max_array(neighbours, 3);
        if(maze[x][y - 1].v == 0){
            north = 0;
        }
    }
    if(maze[x + 1][y].v != -99 && maze[x + 1][y].v != -98){
        int neighbours[3] = {maze[x + 2][y + 1].v, maze[x + 3][y].v, maze[x + 2][y - 1].v};
        east = maze[x + 1][y].v + maze[x + 2][y].v + max_array(neighbours, 3);
        if(maze[x + 1][y].v == 0){
            east = 0;
        }
    }
    if(maze[x][y + 1].v != -99 && maze[x][y + 1].v != -98){
        int neighbours[3] = {maze[x - 1][y + 2].v, maze[x][y + 3].v, maze[x + 1][y + 2].v};
        south = maze[x][y + 1].v + maze[x][y + 2].v + max_array(neighbours, 3);
        if(maze[x][y + 1].v == 0){
            south = 0;
        }
    }
    if(maze[x - 1][y].v != -99 && maze[x - 1][y].v != -98){
        int neighbours[3] = {maze[x - 2][y - 1].v, maze[x - 3][y].v, maze[x - 2][y + 1].v};
        west = maze[x - 1][y].v + maze[x - 2][y].v + max_array(neighbours, 3);
        if(maze[x - 1][y].v == 0){
            west = 0;
        }
    }
    int directions[4] = {north, east, south, west};
    switch(direction){
        case 0:
            if(east == max_array(directions, 4)){
                return 1;
            } else if(north == max_array(directions, 4)){
                return 0;
            } else if(west == max_array(directions, 4)){
                return 3;
            }
            return 2;
        case 1:
            if(south == max_array(directions, 4)){
                return 2;
            } else if(east == max_array(directions, 4)){
                return 1;
            } else if(north == max_array(directions, 4)){
                return 0;
            }
            return 3;
        case 2:
            if(west == max_array(directions, 4)){
                return 3;
            } else if(south == max_array(directions, 4)){
                return 2;
            } else if(east == max_array(directions, 4)){
                return 1;
            }
            return 0;
        case 3:
            if(north == max_array(directions, 4)){
                return 0;
            } else if(west == max_array(directions, 4)){
                return 3;
            } else if(south == max_array(directions, 4)){
                return 2;
            }
            return 1;
    }
}

int main(){
    srand(time(NULL));
    int k;
    initialize_maze();
    robot.x = 4;
    robot.y = 10;
    robot.found = 0;

    pthread_t visualization_thread;

    pthread_create(&visualization_thread, NULL, visualize_maze, NULL);

    // read_input();

    char byteBuffer[BUFSIZ+1];

    // //----------------------------------------------------------
    // // Open COMPORT for reading and writing
    // //----------------------------------------------------------
    hSerial = CreateFile(COMPORT,
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0
    );

    initSio(hSerial);

    while(1){
        // int new_direction = best_direction_simple(robot.x, robot.y, robot.direction);
        int command = 0;
        int new_direction;
        if(robot.x % 2 == 0 && robot.y % 2 == 0){
            int i = 0;
            int x, y;
            for(x = 0; x < GRID_SIZE; x++){
                for(y = 0; y < GRID_SIZE; y++){
                    if(maze[x][y].v == 0){
                        i++;
                    }
                }
            }
            if(i > 24 && !DONE_LEE && !LEE_TO_START){
                if(robot.found >= MINES){
                    lee(true);
                    new_direction = follow_lee();
                }
                else{
                    new_direction = best_direction(robot.x, robot.y, robot.direction);
                }
            }
            else if(DONE_LEE || LEE_TO_START){
                new_direction = follow_lee();
            }
            else{
                VISUALIZE_LEE = true;
                if(robot.found < MINES){
                    lee(false);
                }
                else{
                    lee(true);
                }
                new_direction = follow_lee();
            }
            int response;
            switch(robot.direction){
                case 0:
                    switch(new_direction){
                        case 0:
                            command = 0;
                            break;
                        case 1:
                            command = 2;
                            break;
                        case 2:
                            command = 3;
                            break;
                        case 3:
                            command = 1;
                            break;
                    }
                    break;
                case 1:
                    switch(new_direction){
                        case 0:
                            command = 1;
                            break;
                        case 1:
                            command = 0;
                            break;
                        case 2:
                            command = 2;
                            break;
                        case 3:
                            command = 3;
                            break;
                    }
                    break;
                case 2:
                    switch(new_direction){
                        case 0:
                            command = 3;
                            break;
                        case 1:
                            command = 1;
                            break;
                        case 2:
                            command = 0;
                            break;
                        case 3:
                            command = 2;
                            break;
                    }
                    break;
                case 3:
                    switch(new_direction){
                        case 0:
                            command = 2;
                            break;
                        case 1:
                            command = 3;
                            break;
                        case 2:
                            command = 1;
                            break;
                        case 3:
                            command = 0;
                            break;
                    }
                    break;
            }
        }
        if(maze[robot.x][robot.y].lee_target){
            if(robot.x == 4 && robot.y == 10 && robot.found >= MINES){
                if(robot.direction == 0){
                    writeByte(hSerial, "A");
                }
                else if(robot.direction == 3){
                    writeByte(hSerial, "B");
                }
                else if(robot.direction == 1){
                    writeByte(hSerial, "C");
                }
                Sleep(300);
                writeByte(hSerial, "E");
                part_2();
                return 0;
            }
            printf("At target");
            DONE_LEE = false;
            maze[robot.x][robot.y].lee_target = false;
        }
        maze[robot.x][robot.y].v--;
        switch(new_direction){
            case 0:
                if(maze[robot.x][robot.y - 1].lee_target){
                    DONE_LEE = false;
                    maze[robot.x][robot.y - 1].lee_target = false;
                }
                maze[robot.x][robot.y - 1].v--;
                break;
            case 1:
                if(maze[robot.x + 1][robot.y].lee_target){
                    DONE_LEE = false;
                    maze[robot.x + 1][robot.y].lee_target = false;
                }
                maze[robot.x + 1][robot.y].v--;
                break;
            case 2:
                if(maze[robot.x][robot.y + 1].lee_target){
                    DONE_LEE = false;
                    maze[robot.x][robot.y + 1].lee_target = false;
                }
                maze[robot.x][robot.y + 1].v--;
                break;
            case 3:
                if(maze[robot.x - 1][robot.y].lee_target){
                    DONE_LEE = false;
                    maze[robot.x - 1][robot.y].lee_target = false;
                }
                maze[robot.x - 1][robot.y].v--;
                break;
        }
        send_command_to_robot(command);
        int response = listen_to_robot(command, 1);
        if(response != 2){
            update_robot_position(command, 2);
        }
    }
    return 0;
}

void part_2(){
    initialize_maze();

    while(1){
        int command;
        int new_direction;
        if(robot.x % 2 == 0 && robot.y % 2 == 0){
            int i = 0;
            int x, y;
            for(x = 0; x < GRID_SIZE; x++){
                for(y = 0; y < GRID_SIZE; y++){
                    if(maze[x][y].v == 0){
                        i++;
                    }
                }
            }
            if(i > 24 && !DONE_LEE && !LEE_TO_START){
                if(robot.found >= MINES){
                    lee(true);
                    new_direction = follow_lee();
                }
                else{
                    new_direction = best_direction(robot.x, robot.y, robot.direction);
                }
            }
            else if(DONE_LEE || LEE_TO_START){
                new_direction = follow_lee();
            }
            else{
                VISUALIZE_LEE = true;
                if(robot.found < MINES){
                    lee(false);
                }
                else{
                    lee(true);
                }
                new_direction = follow_lee();
            }
            int response;
            switch(robot.direction){
                case 0:
                    switch(new_direction){
                        case 0:
                            command = 0;
                            break;
                        case 1:
                            command = 2;
                            break;
                        case 2:
                            command = 3;
                            break;
                        case 3:
                            command = 1;
                            break;
                    }
                    break;
                case 1:
                    switch(new_direction){
                        case 0:
                            command = 1;
                            break;
                        case 1:
                            command = 0;
                            break;
                        case 2:
                            command = 2;
                            break;
                        case 3:
                            command = 3;
                            break;
                    }
                    break;
                case 2:
                    switch(new_direction){
                        case 0:
                            command = 3;
                            break;
                        case 1:
                            command = 1;
                            break;
                        case 2:
                            command = 0;
                            break;
                        case 3:
                            command = 2;
                            break;
                    }
                    break;
                case 3:
                    switch(new_direction){
                        case 0:
                            command = 2;
                            break;
                        case 1:
                            command = 3;
                            break;
                        case 2:
                            command = 1;
                            break;
                        case 3:
                            command = 0;
                            break;
                    }
                    break;
            }
        }
        if(maze[robot.x][robot.y].lee_target){
            printf("At target");
            DONE_LEE = false;
            maze[robot.x][robot.y].lee_target = false;
        }
        // if(maze[robot.x][robot.y].mine){
        //     maze[robot.x][robot.y].lee_v = -1;
        //     maze[robot.x][robot.y].v = -99;
        //     maze[robot.x][robot.y].found = true;
        //     robot.found++;
        //     command = 3;
        //     DONE_LEE = false;
        // }
        maze[robot.x][robot.y].v--;
        switch(new_direction){
            case 0:
                if(maze[robot.x][robot.y - 1].lee_target){
                    DONE_LEE = false;
                    maze[robot.x][robot.y - 1].lee_target = false;
                }
                maze[robot.x][robot.y - 1].v--;
                break;
            case 1:
                if(maze[robot.x + 1][robot.y].lee_target){
                    DONE_LEE = false;
                    maze[robot.x + 1][robot.y].lee_target = false;
                }
                maze[robot.x + 1][robot.y].v--;
                break;
            case 2:
                if(maze[robot.x][robot.y + 1].lee_target){
                    DONE_LEE = false;
                    maze[robot.x][robot.y + 1].lee_target = false;
                }
                maze[robot.x][robot.y + 1].v--;
                break;
            case 3:
                if(maze[robot.x - 1][robot.y].lee_target){
                    DONE_LEE = false;
                    maze[robot.x - 1][robot.y].lee_target = false;
                }
                maze[robot.x - 1][robot.y].v--;
                break;
        }
        send_command_to_robot(command);
        int response = listen_to_robot(command, 2);
        if(response != 2){
            update_robot_position(command, 2);
        }
        if(response == 3){
            return 0;
        }
    }
}