#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <Windows.h>

#define COMPORT "COM2"
#define BAUDRATE CBR_9600

const int GRID_SIZE = 13;

HANDLE hSerial;
//these variables are used by checkifcomchanged function
char lastrecievedbit[32] = "x";

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
    Sleep(500);
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
    printf("Byte read from read buffer is: %c \n", buffRead[0]);
    return(buffRead[0]);
}

//this function checks if readByte changes, if it does combithaschanged will go to 1
int checkifcomchanged(){
    char currentbit[32];
    int combithaschanged = 0;
    int i;
    readByte(hSerial, currentbit);
    if(strcmp(lastrecievedbit, currentbit) == 0){
        combithaschanged = 1;
        for(i=0; i<32; i++){
            lastrecievedbit[i] = currentbit[i];
        }
    } else {
        combithaschanged = 0;
    }
    return(combithaschanged);
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
    if(start == 1 || start == 2 || start == 3){
        direction = 0;
    }
    else if(start == 4 || start == 5 || start == 6){
        direction = 3;
    }
    else if(start == 7 || start == 8 || start == 9){
        direction = 2;
    }
    else{
        direction = 1;
    }
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
void make_route(){
    int station1, station2, i, j;
    struct path path;
    for(i = 0; i < 11; i++){
        if(stations[i + 1] == -1){
            break;
            // when the input of list becomes -1, which means there are no more stations to visit,
            // stop the function.
        }
        int k;
        for(k = 0; k < 100; k++){
            commands[k] = -1;
        }
        station1 = stations[i];
        station2 = stations[i+1];
        path = find_path(station1,station2);
        char buf[100];
        snprintf(buf, 100, "Starting at station %d with direction %d", station1, direction);
        debug(buf);
        debug("Go straight...");
        k = 0;
        for(j = 2; path.path_array[j + 2].x != -1; j = j + 2){
            int row = (path.path_array[j].y - 2) / 2;
            int column = (path.path_array[j].x - 2) / 2;
            int next_row = (path.path_array[j + 2].y - 2) / 2;
            int next_column = (path.path_array[j + 2].x - 2) / 2;
            char buf[100];
            snprintf(buf, 100, "c%d%d", row, column);
            debug(buf);
            if(row - next_row == -1){
                // Go south
                if(direction == 3){
                    debug("Go left...");
                    commands[k] = 1;
                }
                else if(direction == 2){
                    debug("Go straight...");
                    commands[k] = 0;
                }
                else if(direction == 1){
                    debug("Go right...");
                    commands[k] = 2;
                }
                direction = 2;
            }
            else if(row - next_row == 1){
                // Go north
                if(direction == 1){
                    debug("Go left...");
                    commands[k] = 1;
                }
                else if(direction == 0){
                    debug("Go straight...");
                    commands[k] = 0;
                }
                else if(direction == 3){
                    debug("Go right...");
                    commands[k] = 2;
                }
                direction = 0;
            }
            else if(column - next_column == 1){
                // Go west
                if(direction == 0){
                    debug("Go left...");
                    commands[k] = 1;
                }
                else if(direction == 3){
                    debug("Go straight...");
                    commands[k] = 0;
                }
                else if(direction == 2){
                    debug("Go right...");
                    commands[k] = 2;
                }
                direction = 3;
            }
            else if(column - next_column == -1){
                // Go east
                if(direction == 2){
                    debug("Go left...");
                    commands[k] = 1;
                }
                else if(direction == 1){
                    debug("Go straight...");
                    commands[k] = 0;
                }
                else if(direction == 0){
                    debug("Go right...");
                    commands[k] = 2;
                }
                direction = 1;
            }
            k += 1;
        }
    }
}

//these functions can be called for the instructions to be send to the robot
//also a handshake functionality is implementec
void sendcommandtorobot(int command){
    char character[32];
    if(command==0){ 
        //go forward
        writeByte(hSerial, "A");
        while(checkifcomchanged()==0 && character != "R"){
            readByte(hSerial, character);
            Sleep(5);
            //this is only necessary if the error margin of recieved bytes is big.
           // if(checkifcomchanged()==0 && readByte(hSerial, character) != "R"){
           //     writeByte(hSerial, "A"); }
        }
    } else if (command==1){ // go left
        writeByte(hSerial, "B");
        while(checkifcomchanged()==0 && character != "S"){
            readByte(hSerial, character);
            Sleep(5);
        }
    } else if (command==2){ // go right
        writeByte(hSerial, "C");
        while(checkifcomchanged()==0 && character != "T"){
            readByte(hSerial, character);
            Sleep(5);
        }
    } else if (command==3){ // turn around
        writeByte(hSerial, "D");
        while(checkifcomchanged()==0 && character != "U"){
            readByte(hSerial, character);
            Sleep(5);
        }
    } else if (command==4){ // stop
        writeByte(hSerial, "E");
        while(checkifcomchanged()==0 && character != "V"){
            readByte(hSerial, character);
            Sleep(5);
        }
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

    initialize_maze_test();

    robot.x = 0;
    robot.y = 4;

    // lee_start_2_target(robot.x, robot.y, 12, 4);

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

    //----------------------------------------------------------
    // Initialize the parameters of the COM port
    initSio(hSerial);
    //----------------------------------------------------------

    read_input();

    visualize_maze();
    make_route();


    writeByte(hSerial, "E"); //at last, send stop byte to robot to get it to stop.
    return 0;
}