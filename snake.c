#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>

#define REFRESH_TIME 30
#define HEIGHT 20
#define WIDTH 60
#define Y 0
#define X 1

enum DIRECTIONS {
    UP=1000,
    LEFT,
    DOWN,
    RIGHT
};

struct termios usrDefault;

struct GameInformation{
    int gameOver;
    int previouslyPressedKey;
    int score;
    int foodCoord[2];
} GameInfo={0,RIGHT,0};

struct Snake_Body {
    int y;
    int x;
};


struct Snake_Struct {
    int head[2];
    int bodyWidth;
    struct Snake_Body *body;
} Snake = {{1,1},0,NULL};


struct GameBuff{
    char *buf;
    int len;
};

#define GAME_BUFF_INIT {NULL,0}


void disableRawMode(){
    tcsetattr(STDIN_FILENO,TCSAFLUSH,&usrDefault);
}

void enableRawMode(){
    if(tcgetattr(STDIN_FILENO, &usrDefault)==-1){
        exit(1);
    }

    atexit(disableRawMode);

    struct termios raw= usrDefault;

    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN]= 0;
    raw.c_cc[VTIME]= 1;


    tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw);
}


int getRandomNumber(int maxRange){
    return ((rand() % maxRange) +1);
}



void addSnakeBody(){
    int lastPosX= Snake.bodyWidth==0? Snake.head[X] : Snake.body[Snake.bodyWidth-1].x;
    int lastPosY= Snake.bodyWidth==0? Snake.head[Y] : Snake.body[Snake.bodyWidth-1].y;


    struct Snake_Body added = {.y=lastPosY,.x=lastPosX};


    struct Snake_Body *new = realloc(Snake.body,sizeof(struct Snake_Body)*( Snake.bodyWidth+1));

    if(new == NULL){
        return;
    }

    new[Snake.bodyWidth]= added;
    Snake.bodyWidth++;
    Snake.body=new;
}

void updateBody(){

    for(int i=Snake.bodyWidth-1;i>0;i--){
        Snake.body[i].x= Snake.body[i-1].x; 
        Snake.body[i].y= Snake.body[i-1].y; 
    }
    if(Snake.bodyWidth!=0){
        Snake.body[0].x= Snake.head[X];
        Snake.body[0].y= Snake.head[Y];
    }
}

int checkIfSpaceIsFree(int rx,int ry){
    int isSpaceFree=1;
     for(int k=0;k<Snake.bodyWidth;k++){
                int x= Snake.body[k].x;
                int y= Snake.body[k].y;
                if(ry==y && rx==x){
                        isSpaceFree=0; 
                       break;
                }
            }
    if(Snake.head[X]== rx && Snake.head[Y]== ry){
        isSpaceFree=0;
    }

    return isSpaceFree;
}

void generateFood(){
    int rx;
    int ry;
    while(1){
            rx= getRandomNumber(WIDTH-1);
            ry= getRandomNumber(HEIGHT-1);
            if(checkIfSpaceIsFree(rx,ry)) break;
    }

    GameInfo.foodCoord[X]= rx;
    GameInfo.foodCoord[Y]= ry;
            
}



void checkFoodEaten(){
    if(Snake.head[X]==GameInfo.foodCoord[X] && Snake.head[Y]==GameInfo.foodCoord[Y]){
        GameInfo.score++;
        addSnakeBody();
        generateFood();
    }
}

int isGameOver(){
        if(Snake.head[Y]==0 || Snake.head[Y]== HEIGHT || Snake.head[X]==0 || Snake.head[X]==WIDTH){
            return 1;
        }
        for(int i=0;i<Snake.bodyWidth;i++){
        
            if((Snake.head[X]== Snake.body[i].x && Snake.head[Y]== Snake.body[i].y)){
                return 1;
            }
        }

    return 0;
}

int updateSnakePosition(int c){
    if((c==UP || c==DOWN || c==LEFT || c==RIGHT) && c!=GameInfo.previouslyPressedKey){
        GameInfo.previouslyPressedKey=c;
    }
  
     updateBody();
        checkFoodEaten();
     
     switch(c){
         case UP:
         Snake.head[Y]--;
         break;
         case DOWN:
         Snake.head[Y]++;
         break;
         case LEFT:
         Snake.head[X]--;
         break;
         case RIGHT:
         Snake.head[X]++;
         break;
         default:
        }
        
       return isGameOver();
}



int readKeyPress(int currentKey){
    char c;
    if(read(STDIN_FILENO,&c,1)==1){
        if(!(c=='w' || c== 's' || c=='a' || c=='d')) return currentKey;

    
        switch(c){
            case 'w':
                if(currentKey==DOWN) return DOWN;
                return UP;
                break;
            case 's':
                if(currentKey==UP) return UP;
                return DOWN;
                break;
            case 'a':
                if(currentKey==RIGHT) return RIGHT;
                return LEFT;
                break;
            case 'd':
                if(currentKey==LEFT) return LEFT;

                return RIGHT;
                break;            
        }
    }
    return currentKey;
}


void drawGame(){
    char gameRenderData[HEIGHT][WIDTH];


    memset(gameRenderData,' ',HEIGHT*WIDTH);
    for(int i=0;i<=HEIGHT;i++){
        gameRenderData[i][0]='-';
        gameRenderData[i][WIDTH]='-';
    }
      for(int i=0;i<=WIDTH;i++){
        gameRenderData[0][i]='-';
        gameRenderData[HEIGHT][i]='-';
    }

    if(GameInfo.gameOver){
        char score[20];
        int scoreLen=snprintf(score,sizeof(score),"Score: %d",GameInfo.score);
        memcpy(&gameRenderData[(HEIGHT/2)-1][(WIDTH/2)-(scoreLen/2)],score,scoreLen);
        memcpy(&gameRenderData[HEIGHT/2][(WIDTH/2)-4],"Game Over",9);
    }else{
        gameRenderData[Snake.head[Y]][Snake.head[X]]='o';
        for(int i=0;i<Snake.bodyWidth;i++){
            gameRenderData[Snake.body[i].y][Snake.body[i].x]='#';
        }
        gameRenderData[GameInfo.foodCoord[Y]][GameInfo.foodCoord[X]]='f';
    }

      char buffer[(HEIGHT+1) * (WIDTH+2 +1)];
        char *p= buffer;
        
        for(int i=0;i<=HEIGHT;i++){
            memcpy(p,gameRenderData[i],WIDTH+1);
            p+=WIDTH+1;
            if(i<HEIGHT){
                *p++= '\r';
                *p++= '\n';
            }
        }
        
        write(STDOUT_FILENO,"\x1b[H\x1b[J",6);
        write(STDOUT_FILENO,buffer,p-buffer); 
}


void cleanUp(){
    free(Snake.body);
}

int main(){
    enableRawMode();
    int currentKey=RIGHT;
    srand(time(NULL));
    generateFood();
    while(1){
        currentKey=readKeyPress(currentKey);
        GameInfo.gameOver=updateSnakePosition(currentKey);
        drawGame();
        printf("\nScore: %d\n",GameInfo.score);
        if(GameInfo.gameOver){
           cleanUp();
            break;
        }
        fflush(NULL);
        usleep(REFRESH_TIME * 1000);
    }
}

