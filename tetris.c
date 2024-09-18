#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define FALL_DELAY 500000 // 1 segundo

#define video_WHITE 0xFFFF
#define video_GREY 0xC618
#define video_MAGENTA 0xF81F
#define video_BLACK 0x0000
#define video_GREEN 0x07E0

int board[BOARD_HEIGHT][BOARD_WIDTH];

int shapes[7][4][4][4] = {
    // I
    {
        { {0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0} },
        { {0,0,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0} },
        { {0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0} },
        { {0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0} }
    },
    // O
    {
        { {1,1}, {1,1} },
        { {1,1}, {1,1} },
        { {1,1}, {1,1} },
        { {1,1}, {1,1} }
    },
    // T
    {
        { {0,1,0}, {1,1,1}, {0,0,0} },
        { {0,1,0}, {0,1,1}, {0,1,0} },
        { {0,0,0}, {1,1,1}, {0,1,0} },
        { {0,1,0}, {1,1,0}, {0,1,0} }
    },
    // L
    {
        { {0,0,1}, {1,1,1}, {0,0,0} },
        { {0,1,0}, {0,1,0}, {0,1,1} },
        { {0,0,0}, {1,1,1}, {1,0,0} },
        { {1,1,0}, {0,1,0}, {0,1,0} }
    },
    // J
    {
        { {1,0,0}, {1,1,1}, {0,0,0} },
        { {0,1,1}, {0,1,0}, {0,1,0} },
        { {0,0,0}, {1,1,1}, {0,0,1} },
        { {0,1,0}, {0,1,0}, {1,1,0} }
    },
    // Z
    {
        { {1,1,0}, {0,1,1}, {0,0,0} },
        { {0,0,1}, {0,1,1}, {0,1,0} },
        { {1,1,0}, {0,1,1}, {0,0,0} },
        { {0,0,1}, {0,1,1}, {0,1,0} }
    },
    // S
    {
        { {0,1,1}, {1,1,0}, {0,0,0} },
        { {0,1,0}, {0,1,1}, {0,0,1} },
        { {0,1,1}, {1,1,0}, {0,0,0} },
        { {0,1,0}, {0,1,1}, {0,0,1} }
    }
};

int currentShape;
int currentRotation;
int currentX;
int currentY;
int scr = 0;

//Cria o tabuleiro do jogo
void initBoard() {
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            board[i][j] = 0;
        }
    }
}

void drawBoard(){
 
    if(video_open() == 1){
        video_clear();
        video_box(80,10,210,239,video_MAGENTA);
        char scrStr[12];
        char scr_msg[50] = "SCORE ";
        fflush(stdin);
        sprintf(scrStr, "%d", scr);
        video_text(55,5,scr_msg);
        video_text(62,5,scrStr);

        for (int i = 0; i < BOARD_HEIGHT; i++) {
            for (int j = 0; j < BOARD_WIDTH; j++) {
                if (board[i][j] == 1) {
                    video_box(j*11 + 90,i*11 + 10,j*11 + 100,i*11 + 20,video_GREY);
                } else {
                    video_box(j*11 + 90,i*11 + 10,j*11 + 100,i*11 + 20,video_BLACK);
                }     
            } 
        }


        // Draw the current piece
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (shapes[currentShape][currentRotation][i][j] == 1) {
                    video_box((currentX + j)*11 + 90,(currentY + i)*11 + 10,(currentX + j)*11 + 100,(currentY + i)*11 + 20,video_GREEN);
                }
            }
        }

        video_show();
        video_close();
    } else {
        printf("Erro no vga");
    }

}

void removeLine(int y) {

    for (int j = 0; j < BOARD_WIDTH; j++) {
        board[y][j] = 0;
    }
    for (int i = y; i > 0; i--) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            board[i][j] = board[i - 1][j];
        }
    }
    for (int j = 0; j < BOARD_WIDTH; j++) {
        board[0][j] = 0;
    }

    scr += 100;
    FALL_DELAY -= 1000
    
}

void checkLines() {
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        int fullLine = 1;
    for (int j = 0; j < BOARD_WIDTH; j++) {
        if (board[i][j] == 0) {
            fullLine = 0;
            break;
        }
    }
    if (fullLine) {
        removeLine(i);
        }
    }
}

void placeShape() {

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (shapes[currentShape][currentRotation][i][j] == 1) {
                board[currentY + i][currentX + j] = 1;
            }
        }
    }
    checkLines();
}

int checkCollision(int x, int y, int rotation) {

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (shapes[currentShape][rotation][i][j] == 1) {
                int boardX = x + j;
                int boardY = y + i;
                if (boardX < 0 || boardX >= BOARD_WIDTH || boardY >= BOARD_HEIGHT || (boardY >= 0 && board[boardY][boardX] == 1)) {

                    return 1;
                }
            }
        }
    }

    return 0;
}

void newShape() {
    currentShape = rand() % 7;
    currentRotation = 0;
    currentX = BOARD_WIDTH / 2 - 2;
    currentY = 0;
    if (checkCollision(currentX, currentY, currentRotation)) {
        printf("Game Over!\n");
        exit(0);
    }
}

void moveDown() {

    if (!checkCollision(currentX, currentY + 1, currentRotation)) {
        currentY++;
    } else {
        placeShape();
        newShape();
    }
}

void rotateShape() {
    int newRotation = (currentRotation + 1) % 4;
    if (!checkCollision(currentX, currentY, newRotation)) {
        currentRotation = newRotation;
    }
}

void moveLeft() {
    if (!checkCollision(currentX - 1, currentY, currentRotation)) {
            currentX--;
    }
}

void moveRight() {
    if (!checkCollision(currentX + 1, currentY, currentRotation)) {
        currentX++;
    }
}

char getch() {
    struct termios oldt, newt;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

void setupNonBlockingInput() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

int kbhit() {
    struct termios oldt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

int main() {
    srand(time(NULL));
    setupNonBlockingInput();

    initBoard();
    newShape();
    time_t lastFallTime = time(NULL);

    while (1) {
        if (kbhit()) {
            char ch = getch();
            switch (ch) {
                case 'a': moveLeft(); break;
                case 'd': moveRight(); break;
                case 's': moveDown(); break;
                case 'w': rotateShape(); break;
                case 'q': // Quit the game
                printf("Quitting...\n");
                return 0;
            }
        }

        // Check for automatic falling
        if (difftime(time(NULL), lastFallTime) >= FALL_DELAY / 1000000.0) {
            moveDown();
            lastFallTime = time(NULL);
        }

        drawBoard();
        usleep(50000); 
    }
    return 0;
}