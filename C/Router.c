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

#define COMPORT "COM2"
#define BAUDRATE CBR_9600



/********** Declaring constants and globals **************/
const int GRID_SIZE = 13;

HANDLE hSerial;
//variables
char character[32];
int start_station, end_station;

// Stations between which a path has to be found, -1 means there is no station.
int stations[]= {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

// 0 = North, 1 = East, 2 = South, 3 = West
int direction;

int commands[100];

/********** Declaring Structs and methods ****************/
struct cell {
    // Value
    int v; 
    // Location in the maze
    int x, y; 

    int visited;
    // Name, not used at the moment
    char name[8]; 
};

// Matrix represantation of the maze
struct cell maze[13][13];

struct cell mines[40];
int number_of_mines = 0;

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


// Linked list implementation for path
typedef struct Path_node {
    int x, y;
    struct Path_node *next;
} path_t;

path_t *insert_node_front(path_t *old_head, int x, int y) {
    path_t *new_head = (path_t *)malloc(sizeof(path_t));
    new_head->x = x;
    new_head->y = y;
    new_head->next = old_head;
    return new_head;
}

void free_path(path_t *head) {
    path_t *tmp;
    while (head) {
        tmp = head;
        head = head->next;
        free(head);
    }
}

void print_path(path_t *head) {
    path_t *cur = head;
    while (cur) {
        printf("(%d, %d)\n", cur->x, cur->y);
        cur = cur->next;
    }
}

// stack implementation for challenge C
struct stack {
    int max_size;
    int top_i;
    struct cell *cells;
};

struct stack *newstack(int capacity) {
    struct stack *pt = (struct stack*)malloc(sizeof(struct stack));

    pt->max_size = capacity;
    pt->top_i = -1;
    pt->cells = (struct cell*)malloc(sizeof(struct cell) * capacity);

    return pt;
}

int is_empty(struct stack *pt) {
    return pt->top_i == -1;
}

int is_full(struct stack *pt) {
    return pt->top_i == pt->max_size-1;
}

void push(struct stack *pt, struct cell cell) {
    if (is_full(pt)) {
        printf("stack is full!\n");
    } else {
        printf("pushing cell with xy: (%d, %d)      ", cell.x, cell.y);
        pt->cells[++(pt->top_i)] = cell;
        printf("new top_i: %d\n", pt->top_i);
    }
}

struct cell pop(struct stack *pt) {
    if (is_empty(pt)) {
        printf("stack is empty!\n");
    } else {
        return pt->cells[(pt->top_i)--];
    }
}


struct robot {
    int x, y;
    int direction;
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

void reset(){
  printf("\033[0m");
}


/********* Input *****************************************/
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

void read_input(){
    int numofblock, i, j, k, ci, cj, dir_n;
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
    start_station = stations[0];
    end_station = stations[1];
}


/********** Maze *****************************************/
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
    if(number_of_mines != 0){
        int i;
        for(i = 0; i <= number_of_mines; i++){
            int x = mines[i].x;
            int y = mines[i].y;
            maze[x][y].v = -2;
        }
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
    maze[3][4].v = 8;
    maze[4][4].v = 9;
    maze[4][3].v = 10;
    maze[4][2].v = 11;
    maze[5][2].v = 12;
    maze[6][2].v = 13;
    maze[6][3].v = 14;
    maze[6][4].v = 15;
    maze[7][4].v = 16;
    maze[8][4].v = 17;
    maze[8][3].v = 18;
    maze[8][2].v = 19;
    maze[8][1].v = 20;
    maze[8][0].v = 21;
}

void visualize_maze(){
    gotoxy(0,1);
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
                if(maze[j][i].v >= 0){
                    if(robot.x == j && robot.y == i){
                        blue();
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
                    else if(maze[j][i].v == -1){
                        red();
                    }
                    else if(maze[j][i].v == -2){
                        purple();
                    }
                    nDigits = 2;
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
            if(nDigits != 2){
                printf(" ");
            }
            if(robot.x == j && robot.y == i){
                if(robot.direction == 0){
                    printf("^");
                } else if(robot.direction == 1) {
                    printf(">");
                } else if(robot.direction == 2) {
                    printf("v");
                } else {
                    printf("<");
                }
            } else {
                printf("%d", maze[j][i].v);
            }
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
    maze[x][y].v = -2;
    mines[number_of_mines] = maze[x][y];
    number_of_mines++;
}

//this function will update the robot position in the maze accordingly to the command.
void update_robot_position(int command){
    if (command == 0){
        if(robot.direction == 0){
            robot.y = robot.y - 2;
        } else if (robot.direction == 1){
            robot.x = robot.x + 2;
        } else if (robot.direction == 2){
            robot.y = robot.y + 2;
        } else if (robot.direction == 3){
            robot.x = robot.x - 2;
        }
    } else if (command == 1){ //left
        if(robot.direction == 0){
            robot.x = robot.x - 2;
        } else if (robot.direction == 1){
            robot.y = robot.y - 2;
        } else if (robot.direction == 2){
            robot.x = robot.x + 2;
        } else if (robot.direction == 3){
            robot.y = robot.y + 2;
        }
    } else if (command == 2){ //right
        if(robot.direction == 0){
            robot.x = robot.x + 2;
        } else if (robot.direction == 1){
            robot.y = robot.y + 2;
        } else if (robot.direction == 2){
            robot.x = robot.x - 2;
        } else if (robot.direction == 3){
            robot.y = robot.y - 2;
        }
    }
}


/********** Lee's algorithm: pathfinding *****************/


path_t *generate_path(struct cell start_cell, struct cell target_cell, int reroute) {
    // initialise path list
    path_t *path = NULL;

    //start at the end and work backwards
    if(reroute){
        start_cell.x = robot.x;
        start_cell.y = robot.y;
    }

    struct cell cur_cell = target_cell;
    struct cell next_cell;
    path = insert_node_front(path, cur_cell.x, cur_cell.y);
    while (!cells_equal(cur_cell, start_cell)) {
        // look left, right, up, down for the cell with value lower than its own (there will always be one)
        // Lazy evaluation allows for the statements after && to not produce an error
        if ((cur_cell.x-1 >= 0) && (-1 < maze[cur_cell.x-1][cur_cell.y].v) && (maze[cur_cell.x-1][cur_cell.y].v < cur_cell.v)) {                    // LEFT
            next_cell = maze[cur_cell.x-1][cur_cell.y];
        } else if ((cur_cell.x+1 <= GRID_SIZE) && (-1 < maze[cur_cell.x+1][cur_cell.y].v) && (maze[cur_cell.x+1][cur_cell.y].v < cur_cell.v)) {     // RIGHT
            next_cell = maze[cur_cell.x+1][cur_cell.y];
        } else if ((cur_cell.y-1 >= 0) && (-1 < maze[cur_cell.x][cur_cell.y-1].v) && (maze[cur_cell.x][cur_cell.y-1].v < cur_cell.v)) {             // UP
            next_cell = maze[cur_cell.x][cur_cell.y-1];
        } else if ((cur_cell.y+1 <= GRID_SIZE) && (-1 < maze[cur_cell.x][cur_cell.y+1].v) && (maze[cur_cell.x][cur_cell.y+1].v < cur_cell.v)) {     // DOWN
            next_cell = maze[cur_cell.x][cur_cell.y+1];
        } else {
            printf("no cell in maze with lower value found\n");
            break;
        }
        path = insert_node_front(path, next_cell.x, next_cell.y);
        cur_cell = next_cell;
    }
    // path = insert_node_front(path, start_cell.x, start_cell.y);
    return path;
}

int *find_possible_neighbors(int i, int j){
    // returns a 1D array for i,j that are possible neighbours

    //allocate memory 4 x (i and j) = 8
    int *n = (int *)malloc(4*2 * sizeof(int));
    // initialise all to -1
    for (int k =0; k < 8; k++) {
        n[k] = -1;
    }
    // check indecies lie within the matrix and are unassigned (value = 0)
    if (0 <= i-1){                  // LEFT
        if (maze[i-1][j].v == 0) {
            n[0] = i-1;
            n[1] = j;
        }
    } 
    if (i+1 <= GRID_SIZE){          // RIGHT
        if (maze[i+1][j].v == 0) {
            n[2] = i+1;
            n[3] = j;
        } 
    }
    if (0 <= j-1){                  // UP
        if (maze[i][j-1].v == 0) {
            n[4] = i;
            n[5] = j-1;
        } 
    }
    if (j+1 <= GRID_SIZE){          // DOWN
        if (maze[i][j+1].v == 0) {
            n[6] = i;
            n[7] = j+1;
        } 
    } 
    return n;
}

void lee_start_2_target(struct cell start, struct cell target, int reroute){
    int counter = 1;
    int *neigbours;
    if(reroute){
        maze[robot.x][robot.y].v = counter;
    }
    else{
        maze[start.x][start.y].v = counter;
    }


    while (maze[target.x][target.y].v == 0) {
        // printf("%d, %d \n", target.x, target.y);
        // increment the neigbours of all cells with value = counter:
        for (int j=0; j<GRID_SIZE; j++){
            for (int i=0; i<GRID_SIZE; i++){
                if (maze[j][i].v == counter){
                    neigbours = find_possible_neighbors(j, i);
                    // for (int l=0; l<8; l++) {
                    //     printf("%d, ", neigbours[l]);
                    // }
                    // printf("\n");
                    for (int k=0; k < 4; k++){
                        if (neigbours[k*2] >= 0){
                            maze[neigbours[k*2]][neigbours[k*2+1]].v = counter+1;
                        }
                    }
                    free(neigbours);
                }
            }
        }
        counter++;
    }
}

/********** Generating commands **************************/

void write_commands(path_t *path, int reroute) {
    /*  Produce an array of instructions that can be sent to the robot
        we're using an array of integers to represent instructions:
            0: Continue straight
            1: Turn left
            2: Turn right
            4: stop     
            
        we also need to keep track of the robot's direction: N, E, S, W */ 
    
    char dir;
    path_t *cur = path;
    int i = 0;
    int buffer[200] = {0};

    // finding the starting direction
    // must start from a station, so we can just test for the edge that its on (assume dir is inwards)
    if(!reroute){
        if (path->x == 0) {
            dir = 'E';
            robot.direction = 1;
        } else if (path->x == GRID_SIZE-1) {
            dir = 'W';
            robot.direction = 3;
        } else if (path->y == 0) {
            dir = 'S';
            robot.direction = 2;
        } else {
            dir = 'N';
            robot.direction = 0;
        }
    }
    else {
        if(robot.direction == 0){
            dir = 'N';
        } else if(robot.direction == 1){
            dir = 'E';
        } else if(robot.direction == 2){
            dir = 'S';
        } else if(robot.direction == 3){
            dir = 'W';
        }
    }

    // translate to instructions
    while (cur->next) {  
        if (cur->x < cur->next->x) {
            switch (dir) {
                case 'E':
                    buffer[i++] = 0;
                    break;
                case 'S':
                    buffer[i++] = 1;
                    break;
                case 'N':
                    buffer[i++] = 2;
                    break;
                default:
                    buffer[i++] = 4;
            }
            dir = 'E';
        } else if (cur->x > cur->next->x) {
            switch (dir) {
                case 'W':
                    buffer[i++] = 0;
                    break;
                case 'S':
                    buffer[i++] = 2;
                    break;
                case 'N':
                    buffer[i++] = 1;
                    break;
                default:
                    buffer[i++] = 4;
            }
            dir = 'W';
        } else if (cur->y < cur->next->y) {
            switch (dir) {
                case 'S':
                    buffer[i++] = 0;
                    break;
                case 'E':
                    buffer[i++] = 2;
                    break;
                case 'W':
                    buffer[i++] = 1;
                    break;
                default:
                    buffer[i++] = 4;
            }
            dir = 'S';
        } else if (cur->y > cur->next->y) {
            switch (dir) {
                case 'N':
                    buffer[i++] = 0;
                    break;
                case 'E':
                    buffer[i++] = 1;
                    break;
                case 'W':
                    buffer[i++] = 2;
                    break;
                default:
                    buffer[i++] = 4;
            }
            dir = 'N';
        } else {
            buffer[i++] = 4;
        }
        cur = cur->next;
    } 
    buffer[i++] = 4;
    
    free(path);

    // we now have every possible transition in the matrix, 
    // but only the odd ones represent a crossing! 
    i=0;
    memset(commands, -1, sizeof commands);
    while (buffer[i] != 4) {
        if (i%2 == 0) {
            commands[i/2] = buffer[i];
        }
        i++;
    }
    commands[(i/2)] = 4;
}

void make_route(int start, int target, int reroute) {
    path_t *path;
    debug("Initialising maze...");
    initialize_maze();
    debug("performing Lee algorithm...\n");
    lee_start_2_target(get_station(start), get_station(target), reroute);
    visualize_maze();
    debug("generating path...\n");
    path = generate_path(get_station(start), get_station(target), reroute);
    debug("writing commands...\n");
    write_commands(path, reroute);
}

void print_commands() {
    int i = 0;
    while (commands[i] != 4) {
        printf("%d, ", commands[i++]);
    }
    printf("%d\n", commands[i]);
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
int listen_to_robot(int command){
    while(1){
        readByte(hSerial, character);
        if (strcmp(character, "Q") == 0){
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
            }
            found_mine(robot.x, robot.y, robot.direction);
            robot.direction = (robot.direction + 2) % 4; // Turn around
            return 2;
        }
        if (strcmp(character, "X") == 0) {
            printf("x has been recieved \n");
            update_robot_position(command);
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
            }
            return 1;
        }
        Sleep(100);
    }
}

/********* challenge C ***********************************/

void depth_first_search() {
    initialize_maze();
    
    // get starting station
    int start_station_num;
    printf("enter the starting station: ");
    scanf("%i", &start_station_num);
    
    // Use a stack for the DFS alg
    struct stack *cell_stack = newstack(100);
    // testing the stack
    struct cell test_cell;
    push(cell_stack, maze[0][0]);
    push(cell_stack, maze[1][0]);
    push(cell_stack, maze[2][0]);
    test_cell = pop(cell_stack);
    printf("%d, %d\n", test_cell.x, test_cell.y);
    test_cell = pop(cell_stack);
    printf("%d, %d\n", test_cell.x, test_cell.y);

    
    // added a visited variable to cell structure

    

}

int main(){
    depth_first_search();
    return 0;
    srand(time(NULL));
    initialize_maze();

    read_input();

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


    //this piece of code will send the commands to the robot
    int n = 0;
    while(n < 12){
        if(stations[n + 1] == -1){
            break;
        }

        struct cell starting_cell = get_station(stations[n]);
        struct cell end_cell = get_station(stations[n + 1]);
        robot.x = starting_cell.x;
        robot.y = starting_cell.y;

        make_route(stations[n], stations[n + 1], 0);
        
        int i = 0;
        int response;
        char character[32];
        while(commands[i+1] != -1){ //loop while there are actually commands
            visualize_maze();
            send_command_to_robot(commands[i]);
            response = listen_to_robot(commands[i]);
            if(response == 0){
                perror("Unknown command send by robot!");
            }
            else if(response == 1){
                debug("Sensors all black");
            }
            else if(response == 2){
                debug("Found mine");
                memset(commands, 0, sizeof commands);
                make_route(0, stations[n + 1], 1);
                i = -1;
            }
            // else if(response == 2){
            //     initialize_maze_test();
            //     struct cell station = get_station(end_station);
            //     lee_start_2_target(robot.x, robot.y, station.x, station.y);
            //     make_route(n);
            // }
        
            i++;
        }
        n++;
    }

    writeByte(hSerial, "E"); //at last, send stop byte to robot to get it to stop.
    return 0;
}