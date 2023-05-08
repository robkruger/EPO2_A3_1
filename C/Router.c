#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <Windows.h>

#define COMPORT "COM3"
#define BAUDRATE CBR_9600

const int GRID_SIZE = 13;

HANDLE hSerial;
//variables
char character[32];
int start_station, end_station;

struct cell {
    // Value
    int v; 
    // Location in the maze
    int x, y; 
    // Name, not used at the moment
    char name[8]; 
};

// Matrix represantation of the maze
struct cell maze[13][13];

struct path {
    struct cell path_array[100];
};

int commands[100];

// Stations between which a path has to be found, -1 means there is no station.
int stations[]= {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

// 0 = North, 1 = East, 2 = South, 3 = West
int direction;

struct robot {
    int x, y;
    int direction;
};

struct robot robot;

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

void found_mine(int x, int y, int direction){
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
    maze[x][y].v = -1;
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

void blue(){
  printf("\033[0;34m");
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
    start_station = stations[0];
    end_station = stations[1];
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

//this function will update the robot position in the maze accordingly to the command.
void update_robot_position(int command){
    if (command = 0){
        if(robot.direction = 0){
            robot.y = robot.y - 1;
        } else if (robot.direction = 1){
            robot.x = robot.x + 1;
        } else if (robot.direction = 2){
            robot.y = robot.y + 1;
        } else if (robot.direction = 3){
            robot.x = robot.x - 1;
        }
    } else if (command = 1){
        if(robot.direction = 0){
            robot.y = robot.y - 1;
            robot.direction = 3;
        } else if (robot.direction = 1){
            robot.x = robot.x + 1;
            robot.direction = 0;
        } else if (robot.direction = 2){
            robot.y = robot.y + 1;
            robot.direction = 1;
        } else if (robot.direction = 3){
            robot.x = robot.x - 1;
            robot.direction = 2;
        }
    } else if (command = 2){
        if(robot.direction = 0){
            robot.y = robot.y - 1;
        } else if (robot.direction = 1){
            robot.x = robot.x + 1;
        } else if (robot.direction = 2){
            robot.y = robot.y + 1;
        } else if (robot.direction = 3){
            robot.x = robot.x - 1;
        }
    } else if (command = 3){
        if(robot.direction = 0){
            robot.y = robot.y - 1;
        } else if (robot.direction = 1){
            robot.x = robot.x + 1;
        } else if (robot.direction = 2){
            robot.y = robot.y + 1;
        } else if (robot.direction = 3){
            robot.x = robot.x - 1;
        }
    }
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
                if(maze[j][i].v != -1){
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
                    else{
                        red();
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

/******************************************************************************
Written by group A3-1
Path finding (Lee) algorithm
******************************************************************************/

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

void write_instruc_from_path_to(int *buffer, path_t *path) {
    /*  Produce an array of instructions that can be sent to the robot
        we're using an array of integers to represent instructions:
            0: Continue straight
            1: Turn left
            2: Turn right
            3: stop     
            
        we also need to keep track of the robot's direction: N, E, S, W */ 
    
    char dir;
    path_t *cur = path;
    int i = 0;

    // finding the starting direction
    // must start from a station, so we can just test for the edge that its on (assume dir is inwards)
    if (path->x == 0) {
        dir = 'E';
    } else if (path->x == GRID_SIZE-1) {
        dir = 'W';
    } else if (path->y == 0) {
        dir = 'S';
    } else {
        dir = 'N';
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
                    buffer[i++] = 3;
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
                    buffer[i++] = 3;
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
                    buffer[i++] = 3;
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
                    buffer[i++] = 3;
            }
            dir = 'N';
        } else {
            buffer[i++] = 3;
        }
        cur = cur->next;
    } 
    buffer[i++] = 3;
    
    free(path);

    // we now have every possible transition in the matrix, 
    // but only the odd ones represent a crossing! 
}


path_t *generate_path_start_2_target(int start, int target) {
    // initialise path list
    path_t *path = NULL;

    //start at the end and work backwards
    struct cell start_cell = get_station(start);
    struct cell cur_cell = get_station(target);
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
            printf("no cell in maze with lower value found");
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

void lee_start_2_target(int start_i, int start_j,
                        int target_i, int target_j){
    int counter = 1;
    int *neigbours;
    maze[start_i][start_j].v = counter;


    while (maze[target_i][target_j].v == 0) {
        // increment the neigbours of all cells with value = counter:
        for (int j=0; j<GRID_SIZE; j++){
            for (int i=0; i<GRID_SIZE; i++){
                if (maze[j][i].v == counter){
                    neigbours = find_possible_neighbors(j, i);
                    for (int k=0; k < 4; k++){
                        if (neigbours[k*2] != -1){
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

int main(){
    srand(time(NULL));
    initialize_maze();

    read_input();

    char byteBuffer[BUFSIZ+1];

    //----------------------------------------------------------
    // Open COMPORT for reading and writing
    //----------------------------------------------------------
    hSerial = CreateFile(COMPORT,
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0
    );

    if(hSerial == INVALID_HANDLE_VALUE){
        if(GetLastError()== ERROR_FILE_NOT_FOUND){
            //serial port does not exist. Inform user.
            printf(" serial port does not exist \n");
        }
        //some other error occurred. Inform user.
        printf(" some other error occured. Inform user.\n");
    }

    // //----------------------------------------------------------
    // // Initialize the parameters of the COM port
    initSio(hSerial);
    // //----------------------------------------------------------

    // make_route();
    visualize_maze();


    //this piece of code will send the commands to the robot
    int n = 0;
    while(n < 12){
        if(stations[n + 1] == -1){
            break;
        }

        int instructions[100] = {0};
        path_t *path;
        lee_start_2_target(0,4, 12,6);
        path = generate_path_start_2_target(start_station, end_station);
        write_instruc_from_path_to(instructions, path);
        
        int i = 0;
        int response;
        char character[32];
        while(commands[i+1] != -1){ //loop while there are actually commands
            send_command_to_robot(commands[i]);
            printf("sendcommand is gerund");
            response = listen_to_robot(i);
            printf("an response has been recieved");
            if(response == 0){
                perror("Unknown command send by robot!");
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