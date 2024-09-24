#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>

//Endereços usados para mapeamento e funcionalidades dos botões
#define LW_BRIDGE_BASE 0xFF200000
#define LW_BRIDGE_SPAN 0x00005000
#define KEY_BASE 0x00000050
#define BUTTON_0_MASK 0x01
#define BUTTON_1_MASK 0x02
#define BUTTON_2_MASK 0x04
#define BUTTON_3_MASK 0x08

//Tamanho do tabuleiro do jogo
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define FALL_DELAY 500000 // 1 segundo

#define video_WHITE 0xFFFF
#define video_GREY 0xC618
#define video_MAGENTA 0xF81F
#define video_BLACK 0x0000
#define video_GREEN 0x07E0

//Endereços usados para mapeamento e funcionalidades do acelerometro
typedef int bool;
#define true 1
#define false 0
#define SYSMGR_BASE 0xFFD08000
#define SYSMGR_SPAN 0x00000800
#define SYSMGR_GENERALIO7 ((volatile unsigned int *) 0xffd0849C)
#define SYSMGR_GENERALIO8 ((volatile unsigned int *) 0xffd084A0)
#define SYSMGR_I2C0USEFPGA ((volatile unsigned int *) 0xffd08704)
#define I2C0_BASE 0xFFC04000
#define I2C0_SPAN 0x00000100
#define ADXL345_REG_DATAX0 0x32
#define ADXL345_REG_DATAX1 0x33
#define ADXL345_REG_DATAY0 0x34
#define ADXL345_REG_DATAY1 0x35
#define ADXL345_REG_DATA_FORMAT 0x31
#define XL345_RANGE_16G 0x03
#define XL345_FULL_RESOLUTION 0x08
#define ADXL345_REG_BW_RATE 0x2C
#define XL345_RATE_200 0x0b
#define ADXL345_REG_THRESH_ACT 0x24
#define ADXL345_REG_THRESH_INACT 0x25
#define ADXL345_REG_TIME_INACT 0x26
#define ADXL345_REG_ACT_INACT_CTL 0x27
#define ADXL345_REG_INT_ENABLE 0x2E
#define XL345_ACTIVITY 0x10
#define XL345_INACTIVITY 0x08
#define ADXL345_REG_POWER_CTL 0x2D
#define XL345_MEASURE 0x08
#define XL345_STANDBY 0x00
#define ADXL345_REG_INT_SOURCE 0x30
#define ADXL345_REG_OFSX 0x1E
#define ADXL345_REG_OFSY 0x1F
#define ADXL345_REG_OFSZ 0x20
#define XL345_RATE_100 0x0a
#define XL345_DATAREADY 0x80
#define ROUNDED_DIVISION(n, d) (((n < 0) ^ (d < 0)) ? ((n - d/2)/d) : ((n + d/2)/d))

//Tabuleiro do jogo
int board[BOARD_HEIGHT][BOARD_WIDTH];

//a variavel shapes armazena todos os blocos possiveis de jogo e suas variacoes, vertical, horizontal, invertido verticalmente e invertido horizontalmente
int shapes[7][4][4][4] = {
 // I
 {
    { {0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0} },
    { {0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0} },
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

//variaveis que representam os registradores do acelerometro
void * I2C0_virtual;
void * sysmgr_virtual;
volatile int * ic_con;
volatile int * ic_tar;
volatile int * ic_fs_scl_hcnt; 
volatile int * ic_fs_scl_lcnt;
volatile int * ic_enable;
volatile int * ic_enable_status;
volatile int * ic_data_cmd;
volatile int * ic_rxflr;
unsigned int * sysmgr_virtual_ptr;

//variaveis de controle e variaveis de execucao do jogo
int currentShape;
int currentRotation;
int currentX;
int currentY;
int menu = 1;
int scr = 0;
int hscr[3] = {0, 0, 0};
int pause2 = 0;
int opt = 0;
int jogando = 0;
char scrStr[12] = "";
char scrStr1[12] = "";
char scrStr2[12] = "";
char scrStr3[12] = "";
char scr_msg[50] = "";

// Função para mapear a memória
void *map_physical_memory(off_t base, size_t span) {
    int fd;
    void *virtual_base;

    // Abre o arquivo de memória
    if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
        printf("ERROR: could not open \"/dev/mem\"...\n");
        return NULL;
    }

    // Mapeia a memória física para o espaço de endereçamento do processo
    virtual_base = mmap(NULL, span, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, base);
    if (virtual_base == MAP_FAILED) {
        printf("ERROR: mmap() failed...\n");
        close(fd);
        return NULL;
    }

    close(fd);
    return virtual_base;
}

void ADXL345_REG_READ(uint8_t address, uint8_t *value){
    *ic_data_cmd = address + 0x400;
    *ic_data_cmd = 0x100;
    while (*ic_rxflr == 0){}
    *value = *ic_data_cmd;
}

void ADXL345_REG_WRITE(uint8_t address, uint8_t value){
 
    // Send reg address (+0x400 to send START signal)
    *ic_data_cmd = address + 0x400;
    
    // Send value
    *ic_data_cmd = value;
}

bool ADXL345_WasActivityUpdated(){
    bool bReady = false;
    uint8_t data8;
    
    ADXL345_REG_READ(ADXL345_REG_INT_SOURCE,&data8);
    if (data8 & XL345_ACTIVITY)
    bReady = true;
    
    return bReady;
}

//inicia o acelerometro
void ADXL345_Init(){

    ADXL345_REG_WRITE(ADXL345_REG_DATA_FORMAT, XL345_RANGE_16G | XL345_FULL_RESOLUTION);

    ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_200);

    ADXL345_REG_WRITE(ADXL345_REG_THRESH_ACT, 0x04);
    ADXL345_REG_WRITE(ADXL345_REG_THRESH_INACT, 0x02);
    ADXL345_REG_WRITE(ADXL345_REG_TIME_INACT, 0x02);
    ADXL345_REG_WRITE(ADXL345_REG_ACT_INACT_CTL, 0xFF);
    ADXL345_REG_WRITE(ADXL345_REG_INT_ENABLE, XL345_ACTIVITY | XL345_INACTIVITY );

    ADXL345_REG_WRITE(ADXL345_REG_POWER_CTL, XL345_STANDBY);

    ADXL345_REG_WRITE(ADXL345_REG_POWER_CTL, XL345_MEASURE);

    ADXL345_Calibrate();
}

void ADXL345_REG_MULTI_READ(uint8_t address, uint8_t values[], uint8_t len){

    // Send reg address (+0x400 to send START signal)
    *ic_data_cmd = address + 0x400;
    
    // Send read signal len times
    int i;
    for (i=0;i<len;i++)
    *ic_data_cmd = 0x100;

    // Read the bytes
    int nth_byte=0;
    while (len){
        if ((*ic_rxflr) > 0){
            values[nth_byte] = *ic_data_cmd;
            nth_byte++;
            len--;
        }
    }
}

//le as coordenadas dos eixos do acelerometro
void ADXL345_XYZ_Read(int16_t szData16[3]){
    uint8_t szData8[6];
    ADXL345_REG_MULTI_READ(0x32, (uint8_t *)&szData8, sizeof(szData8));

    szData16[0] = (szData8[1] << 8) | szData8[0]; 
    szData16[1] = (szData8[3] << 8) | szData8[2];
    szData16[2] = (szData8[5] << 8) | szData8[4];
}

//faz a configuracao inicial do IC20
void initConfigIC20(){
    ic_con = (int *) (I2C0_virtual + 0x0);
    ic_tar = (int *) (I2C0_virtual + 0x4);
    ic_fs_scl_hcnt = (int *) (I2C0_virtual + 0x1C);
    ic_fs_scl_lcnt = (int *) (I2C0_virtual + 0x20);
    ic_enable = (int *) (I2C0_virtual + 0x6C);
    ic_enable_status = (int *) (I2C0_virtual + 0x9C);
    ic_data_cmd = (int *) (I2C0_virtual + 0x10);
    ic_rxflr = (int *) (I2C0_virtual + 0x78);

    *ic_enable = 2;

    while(((*ic_enable_status)&0x1) == 1){}

    *ic_con = 0x65;
    *ic_tar = 0x53;
    *ic_fs_scl_hcnt = 90;
    *ic_fs_scl_lcnt = 160;
    *ic_enable = 1;

    while(((*ic_enable_status)&0x1) == 0){}
}

bool ADXL345_IsDataReady(){
    bool bReady = false;
    uint8_t data8;
    
    ADXL345_REG_READ(ADXL345_REG_INT_SOURCE,&data8);
    if (data8 & XL345_DATAREADY)
    bReady = true;
    
    return bReady;
}

//calibra os valores do acelerometro
void ADXL345_Calibrate(){
 
    int average_x = 0;
    int average_y = 0;
    int average_z = 0;
    int16_t XYZ[3];
    int8_t offset_x;
    int8_t offset_y;
    int8_t offset_z;
    
    // stop measure
    ADXL345_REG_WRITE(ADXL345_REG_POWER_CTL, XL345_STANDBY);
    
    // Get current offsets
    ADXL345_REG_READ(ADXL345_REG_OFSX, (uint8_t *)&offset_x);
    ADXL345_REG_READ(ADXL345_REG_OFSY, (uint8_t *)&offset_y);
    ADXL345_REG_READ(ADXL345_REG_OFSZ, (uint8_t *)&offset_z);
    
    // Use 100 hz rate for calibration. Save the current rate.
    uint8_t saved_bw;
    ADXL345_REG_READ(ADXL345_REG_BW_RATE, &saved_bw);
    ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_100);
    
    // Use 16g range, full resolution. Save the current format.
    uint8_t saved_dataformat;
    ADXL345_REG_READ(ADXL345_REG_DATA_FORMAT, &saved_dataformat);
    ADXL345_REG_WRITE(ADXL345_REG_DATA_FORMAT, XL345_RANGE_16G | XL345_FULL_RESOLUTION);
    
    // start measure
    ADXL345_REG_WRITE(ADXL345_REG_POWER_CTL, XL345_MEASURE);
    
    // Get the average x,y,z accelerations over 32 samples (LSB 3.9 mg)
    int i = 0;
    while (i < 32){
        // Note: use DATA_READY here, can't use ACTIVITY because board is stationary.
        if (ADXL345_IsDataReady()){
            ADXL345_XYZ_Read(XYZ);
            average_x += XYZ[0];
            average_y += XYZ[1];
            average_z += XYZ[2];
            i++;
        }
    }

    average_x = ROUNDED_DIVISION(average_x, 32);
    average_y = ROUNDED_DIVISION(average_y, 32);
    average_z = ROUNDED_DIVISION(average_z, 32);
    
    // stop measure
    ADXL345_REG_WRITE(ADXL345_REG_POWER_CTL, XL345_STANDBY); 
    
    // Calculate the offsets (LSB 15.6 mg)
    offset_x += ROUNDED_DIVISION(0-average_x, 4);
    offset_y += ROUNDED_DIVISION(0-average_y, 4);
    offset_z += ROUNDED_DIVISION(256-average_z, 4);
    
    // Set the offset registers
    ADXL345_REG_WRITE(ADXL345_REG_OFSX, offset_x);
    ADXL345_REG_WRITE(ADXL345_REG_OFSY, offset_y);
    ADXL345_REG_WRITE(ADXL345_REG_OFSZ, offset_z);
    
    // Restore original bw rate
    ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, saved_bw);
    
    // Restore original data format
    ADXL345_REG_WRITE(ADXL345_REG_DATA_FORMAT, saved_dataformat);
    
    // start measure
    ADXL345_REG_WRITE(ADXL345_REG_POWER_CTL, XL345_MEASURE);
}


//Cria o tabuleiro do jogo
void initBoard() {
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            board[i][j] = 0;
        }
    }
}

// Função para criar uma sombra nas letras do nome Tetris no menu
void draw_box_with_shadow(int x1, int y1, int x2, int y2, int color, int shadow_color) {
    // Desenhar a sombra (deslocada para a direita e para baixo)
    video_box(x1 + 2, y1 + 2, x2 + 2, y2 + 2, shadow_color); // Sombra

    // Desenhar a caixa principal
    video_box(x1, y1, x2, y2, color); // Parte principal
}

// Função que desenha todas as letras de "Tetris" com sombra e cores diferentes
void draw_menu() {
    // Definir a cor da sombra (Cinza escuro)
    int shadow_color = 0x202020;

    draw_box_with_shadow(10, 20, 50, 30, video_GREEN, shadow_color); // Parte superior
    draw_box_with_shadow(25, 30, 35, 70, video_GREEN, shadow_color); // Parte vertical

    // Desenhar a letra E (Verde)
    int color_E = 0x00FF00; // Cor Verde
    draw_box_with_shadow(60, 20, 100, 30, video_WHITE, shadow_color); // Parte superior
    draw_box_with_shadow(60, 40, 90, 50, video_WHITE, shadow_color); // Parte do meio
    draw_box_with_shadow(60, 60, 100, 70, video_WHITE, shadow_color); // Parte inferior
    draw_box_with_shadow(60, 30, 70, 70, video_WHITE, shadow_color); // Parte vertical

    // Desenhar a letra T (segunda vez - Azul)
    // Cor Azul
    draw_box_with_shadow(110, 20, 150, 30, video_GREEN, shadow_color); // Parte superior
    draw_box_with_shadow(125, 30, 135, 70, video_GREEN, shadow_color); // Parte vertical

    // Desenhar a letra R (Amarelo)
    int color_R = 0xFFFF00; // Cor Amarela
    draw_box_with_shadow(160, 20, 200, 30, video_GREY, shadow_color); // Parte superior
    draw_box_with_shadow(160, 30, 170, 70, video_GREY, shadow_color); // Parte vertical
    draw_box_with_shadow(170, 40, 200, 50, video_GREY, shadow_color); // Parte intermediária
    draw_box_with_shadow(190, 20, 200, 40, video_GREY, shadow_color); // Curva do R
    draw_box_with_shadow(190, 50, 200, 70, video_GREY, shadow_color); // Perna do R

    // Desenhar a letra I (Ciano)
    int color_I = 0x00FFFF; // Cor Ciano
    draw_box_with_shadow(210, 20, 250, 30, video_GREEN, shadow_color); // Parte superior
    draw_box_with_shadow(225, 30, 235, 70, video_GREEN, shadow_color); // Parte vertical
    draw_box_with_shadow(210, 60, 250, 70, video_GREEN, shadow_color); // Parte inferior

    // Desenhar a letra S (Magenta)
    int color_S = 0xFF00FF; // Cor Magenta
    draw_box_with_shadow(260, 20, 300, 30, video_WHITE, shadow_color); // Parte superior
    draw_box_with_shadow(260, 30, 270, 50, video_WHITE, shadow_color); // Parte esquerda superior
    draw_box_with_shadow(270, 40, 300, 50, video_WHITE, shadow_color); // Parte intermediária
    draw_box_with_shadow(290, 50, 300, 70, video_WHITE, shadow_color); // Parte direita inferior
    draw_box_with_shadow(260, 60, 300, 70, video_WHITE, shadow_color); // Parte inferior
}

// Função principal que exibe na tela as paginas do jogo
void drawBoard(){
    
    //if para inicializar o driver de video
    if(video_open() == 1){
    
    //a variavel menu faz o controle para checar se o usuario esta no menu do jogo ou se esta jogando
    if( menu == 1 ){
        video_clear();
        draw_menu();

        char hscr_msg[50] = "HIGHSCORE: ";
        char hscr1_msg[50] = "1- ";
        char hscr2_msg[50] = "2- ";
        char hscr3_msg[50] = "3- ";
        fflush(stdin);
        sprintf(scrStr1, "%d", hscr[0]);
        fflush(stdin);
        sprintf(scrStr2, "%d", hscr[1]);
        fflush(stdin);
        sprintf(scrStr3, "%d", hscr[2]);
        fflush(stdin);

        if(opt == 0){
            video_text(35,26,">JOGAR");
            video_text(35,28,"SAIR");

            video_text(35,33,hscr_msg);
            video_text(35,35,strcat(hscr1_msg,scrStr1));
            video_text(35,37,strcat(hscr2_msg,scrStr2));
            video_text(35,39,strcat(hscr3_msg,scrStr3));
        } else if(opt == 1){
            video_text(35,26,"JOGAR");
            video_text(35,28,">SAIR");

            video_text(35,33,hscr_msg);
            video_text(35,35,strcat(hscr1_msg,scrStr1));
            video_text(35,37,strcat(hscr2_msg,scrStr2));
            video_text(35,39,strcat(hscr3_msg,scrStr3));
        }
    
    }else{
        
        video_clear();
        video_box(80,10,210,239,video_WHITE);
        char scr_msg[50] = "SCORE: ";
        char hscr_msg[50] = "HIGHSCORE: ";
        char hscr1_msg[50] = "1- ";
        char hscr2_msg[50] = "2- ";
        char hscr3_msg[50] = "3- ";
        fflush(stdin);
        sprintf(scrStr1, "%d", hscr[0]);
        fflush(stdin);
        sprintf(scrStr2, "%d", hscr[1]);
        fflush(stdin);
        sprintf(scrStr3, "%d", hscr[2]);
        fflush(stdin);
        sprintf(scrStr, "%d", scr);
        video_text(55,5,strcat(scr_msg,scrStr));
        video_text(55,10,hscr_msg);
        video_text(55,12,strcat(hscr1_msg,scrStr1));
        video_text(55,14,strcat(hscr2_msg,scrStr2));
        video_text(55,16,strcat(hscr3_msg,scrStr3));
        //video_text(62,5,scrStr);

        //loop for que desenha a matriz do jogo (peças ja caidas e espaços vazios)
        for (int i = 0; i < BOARD_HEIGHT; i++) {
            for (int j = 0; j < BOARD_WIDTH; j++) {
                if (board[i][j] == 1) {
                    video_box(j*11 + 90,i*11 + 10,j*11 + 100,i*11 + 20,video_GREY);
                } else {
                    video_box(j*11 + 90,i*11 + 10,j*11 + 100,i*11 + 20,video_BLACK);
                } 
            } 
        }


        // Desenha a peça atual
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (shapes[currentShape][currentRotation][i][j] == 1) {
                    video_box((currentX + j)*11 + 90,(currentY + i)*11 + 10,(currentX + j)*11 + 100,(currentY + i)*11 + 20,video_GREEN);
                }
            }
        }

        // If que checa se o jogo esta em pause
        if(pause2 == 1){
            // If que checa a opção a ser inserida no menu de pausa e muda o indicador (>)
            if(opt == 0){
                video_box(110,50,180,150,video_WHITE);
                video_box(115,55,175,145,video_BLACK);
                video_text(30,16,"Jogo pausado");
                video_text(30,20,">Continuar");
                video_text(30,24,"Sair");
            } else if(opt == 1){
                video_box(110,50,180,150,video_WHITE);
                video_box(115,55,175,145,video_BLACK);
                video_text(30,16,"Jogo pausado");
                video_text(30,20,"Continuar");
                video_text(30,24,">Sair");
            }
        }

        //Jogando = 2 significa que o jogador perdeu
        if(jogando == 2){
            video_box(110,50,180,90,video_WHITE);
            video_box(115,55,175,85,video_BLACK);
            video_text(30,16,"Você perdeu");
            video_text(30,20,">OK");
        } 
    }

    video_show();
    video_close();

    } else {
        printf("Erro no vga");
    }

}

//Função para limpar tudo da tela (caixas e letras)
void limpaTela(){

    if(video_open() == 1){
        video_clear();
        video_erase();
        video_show();
        video_close();
    }
}

//Função que remove a linha caso totalmente preenchida e move as que estão acima dela uma unidade abaixo, ao final soma 100 ao score do jogdor
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
 
}

//Checa se existem linhas preenchidas na matriz e as remove
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

//Função que "Encaixa" a peça na matriz do jogo, alterando o valor anterior de 0 (vazio) para 1 (bloco)
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

//Checa se houve colisao da peça atual com a matriz do jogo
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

//Gera uma nova peça
void newShape() {
    currentShape = rand() % 7;
    currentRotation = 0;
    currentX = BOARD_WIDTH / 2 - 2;
    currentY = 0;
    if (checkCollision(currentX, currentY, currentRotation)) {
        printf("Game Over!\n");
        //jogando = 2 simboliza game over
        jogando = 2;
    }
}

//Função que faz o movimento para baixo da peça
void moveDown() {

    if (!checkCollision(currentX, currentY + 1, currentRotation)) {
        currentY++;
    } else {
        placeShape();
        newShape();
    }
}

//Função para rotacionar a peça
void rotateShape() {
    int newRotation = (currentRotation + 1) % 4;
    if (!checkCollision(currentX, currentY, newRotation)) {
        currentRotation = newRotation;
    }
}

//Função para movar a peça para a esquerda
void moveLeft() {
    if (!checkCollision(currentX - 1, currentY, currentRotation)) {
        currentX--;
    }
}

//Função para movar a peça para a direita
void moveRight() {
    if (!checkCollision(currentX + 1, currentY, currentRotation)) {
        currentX++;
    }
}

int main() {
    //mapeia a memoria do I2CO e o sysmgr
    I2C0_virtual = map_physical_memory(I2C0_BASE, I2C0_SPAN);
    sysmgr_virtual = map_physical_memory(SYSMGR_BASE, SYSMGR_SPAN);

    uint8_t devid;
    int16_t mg_per_lsb = 4;
    int16_t XYZ[3];

    initConfigIC20();

    ADXL345_REG_READ(0x00, &devid);

    //checa o endereco do device (acelerometro)
    if (devid == 0xE5){
        ADXL345_Init();
        srand(time(NULL));
        limpaTela();

        time_t lastFallTime = time(NULL);

        void *LW_virtual;
        volatile int *edge_cap;
        volatile int *button_ptr;

        // Mapeia a memória
        LW_virtual = map_physical_memory(LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
        if (LW_virtual == NULL) {
            return -1;
        }

        // Ponto de memória para os botões e o edge capture
        button_ptr = (int *)(LW_virtual + KEY_BASE);
        edge_cap = (int *)(LW_virtual + 0x5C); 
        *edge_cap = 0xF;

        while (1) {
            int buttons_pressed = *edge_cap;

            //if checa se o jogador esta no jogo (menu == 0) e se não perdeu (jogando != 2)
            if(menu == 0 && jogando != 2){
                jogando = 1;
                //if checa se o jogo não esta pausado (pause2 == 0) e se esta em jogo (jogando == 1)
                if(pause2 == 0 && jogando == 1){

                    ADXL345_XYZ_Read(XYZ);
                    //checa se a placa foi inclinada a direita e move a peça
                    if(XYZ[0]*mg_per_lsb >= 150){
                        moveRight();
                        usleep(50000); 
                        //checa se a placa foi inclinada a esquerda e move a peça
                    }else if(XYZ[0]*mg_per_lsb <= -150){
                        moveLeft();
                        usleep(50000); 
                    }
                    //checa se a placa foi inclinada para baixo e move a peça
                    if(XYZ[1]*mg_per_lsb <= -200){
                        moveDown();
                    }
                    
                    //checa se o botao 0 foi pressionado e caso seja, pausa o jogo
                    if (buttons_pressed & BUTTON_0_MASK) {
                        printf("Pausando...\n");
                        pause2 = 1;
                        *edge_cap = BUTTON_0_MASK; // Limpa a interrupção para o botão 0
                    }
                    //checa se o botao 1 foi pressionado e caso seja, gira a peça atual
                    if (buttons_pressed & BUTTON_1_MASK) {
                        rotateShape();
                        *edge_cap = BUTTON_1_MASK; // Limpa a interrupção para o botão 1
                    }

                    // checagem para queda automatica da peça
                    if (difftime(time(NULL), lastFallTime) >= FALL_DELAY / 1000000.0) {
                        moveDown();
                        lastFallTime = time(NULL);
                    }

                    //desenha a matriz na tela do monitor
                    drawBoard();
                    usleep(50000); 

                //Checa se o jogo foi pausado    
                } else if(pause2 == 1 && jogando == 1){
                    drawBoard();
                    
                    //Muda a opção a ser pressionada
                    if (buttons_pressed & BUTTON_1_MASK) {
                        opt = !opt;
                        limpaTela();
                        // Limpa a interrupção
                        *edge_cap = BUTTON_1_MASK;
                    } 

                    //despausa o jogo
                    if (buttons_pressed & BUTTON_0_MASK && opt == 0) {
                        printf("Continuando...\n");
                        pause2 = 0;
                        limpaTela();
                        // Limpa a interrupção
                        *edge_cap = BUTTON_0_MASK;

                    //sai da partida    
                    } else if(buttons_pressed & BUTTON_0_MASK && opt == 1){
                        menu = 1;
                        jogando = 0;
                        opt = 0;
                        pause2 = 0;
                        scr = 0;
                        printf("saindo...\n");
                        limpaTela();
                        // Limpa a interrupção
                        *edge_cap = BUTTON_0_MASK;
                    
                    }

                }

            //Checagem para reiniciar o tabuleiro sempre que o jogador vai do menu inicial para o jogo
            }else if(jogando == 0){

                initBoard();
                newShape();
                                
                drawBoard();

                if (buttons_pressed & BUTTON_1_MASK) {
                    opt = !opt;
                    limpaTela();
                    // Limpa a interrupção
                    *edge_cap = BUTTON_1_MASK;
                } 

                //inicia o jogo
                if (buttons_pressed & BUTTON_0_MASK && opt == 0) {
                    printf("iniciando...\n");
                    menu = 0;
                    limpaTela();
                    // Limpa a interrupção
                    *edge_cap = BUTTON_0_MASK;
                //sai do jogo    
                } else if(buttons_pressed & BUTTON_0_MASK && opt == 1){
                    printf("saindo...\n");
                    limpaTela();
                    // Limpa a interrupção
                    *edge_cap = BUTTON_0_MASK;
                    exit(0);
                }

            //checa se o jogador perdeu (jogando == 2) e caso o score dele seja maior que um dos 3 recordes, salva o score dele
            }else if(jogando == 2){

                drawBoard();
                
                if(scr > hscr[0]){
                    hscr[2] = hscr[1];
                    hscr[1] = hscr[0];
                    hscr[0] = scr;
                    scr = 0;
                }else if(scr > hscr[1]){
                    hscr[2] = hscr[1];
                    hscr[1] = scr;
                    scr = 0;
                }else if(scr > hscr[2]){
                    hscr[2] = scr;
                    scr = 0;
                }
                
                
                //volta ao menu
                if (buttons_pressed & BUTTON_0_MASK && opt == 0) {
                    menu = 1;
                    jogando = 0;
                    opt = 0;
                    pause2 = 0;
                    scr = 0;
                    printf("saindo...\n");
                    limpaTela();
                    // Limpa a interrupção
                    *edge_cap = BUTTON_0_MASK;
                }
            
            }
        }
        
        if (munmap(LW_virtual, LW_BRIDGE_SPAN) != 0) {
            printf("ERROR: munmap() failed...\n");
            return -1;
        }

    } else {
        printf("Incorrect device ID\n");
    }
    
    return 0;
}