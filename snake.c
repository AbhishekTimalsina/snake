#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>

#define REFRESH_TIME 200
#define HEIGHT 20
#define WIDTH 60
#define Y 0
#define X 1
#define dir 2


enum DIRECTIONS {
    UP=1000,
    LEFT,
    DOWN,
    RIGHT
};

struct termios usrDefault;

int pos1[2]= {5,9};  //y, x
int pos2[2]= {5,8};

int previouslyPressedKey=RIGHT;
int gameOver=0;


struct GameInformation{
    int gameOver;
    int previouslyPressedKey;
    int score;
    int foodCoord[2];
} GameInfo={0,RIGHT,0};

struct Snake_Body {
    int y;
    int x;
    int (*breakPoints)[3];
    int breakPointWidth;
    int direction;
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



void appendBuff(struct GameBuff *gb,char *str,int len){
   char *new= realloc(gb->buf, gb->len + len);

    if(new==NULL){
        return;
    }

    memcpy(&new[gb->len],str,len);
    gb->buf= new;
    gb->len+=len;
}

int getRandomNumber(int maxRange){
    return (rand() % maxRange) +1;
}
// void addBreakPoint(int x,int y,int d){
//     for(int i=0;i<Snake.bodyWidth;i++){
//         int  (*newBreakPoint)[3]= realloc(Snake.body[i].breakPoints,sizeof(int[3]) * (Snake.body[i].breakPointWidth+1));
//         if(newBreakPoint==NULL) return;

//         Snake.body[i].breakPoints= newBreakPoint;

//         Snake.body[i].breakPoints[i][X]= x;
//         Snake.body[i].breakPoints[i][Y]= y;
//         Snake.body[i].breakPoints[i][dir]= d;

//         Snake.body[i].breakPointWidth++;
//     }

// }


void addBreakPoint(struct Snake_Body *body,int x, int y , int d){
    
        int  (*newBreakPoint)[3]= realloc(body->breakPoints,sizeof(int[3]) * (body->breakPointWidth+1));
        if(newBreakPoint==NULL) return;

        body->breakPoints= newBreakPoint;

        body->breakPoints[body->breakPointWidth][X]= x;
        body->breakPoints[body->breakPointWidth][Y]= y;
        body->breakPoints[body->breakPointWidth][dir]= d;

        body->breakPointWidth++;

}



void addSnakeBody(int direction){
    int lastPosX= Snake.bodyWidth==0? Snake.head[X] : Snake.body[Snake.bodyWidth-1].x;
    int lastPosY= Snake.bodyWidth==0? Snake.head[Y] : Snake.body[Snake.bodyWidth-1].y;

    struct Snake_Body added = {.y=lastPosY,.x=lastPosX-1, .breakPoints=NULL,.breakPointWidth=0,.direction=RIGHT};

    if(Snake.bodyWidth!=0){
        added.breakPointWidth= Snake.body[Snake.bodyWidth-1].breakPointWidth;
        added.direction= Snake.body[Snake.bodyWidth-1].direction;
    }

    for(int i=0;i<added.breakPointWidth;i++){
        addBreakPoint(&added,Snake.body[Snake.bodyWidth-1].breakPoints[i][X],Snake.body[Snake.bodyWidth-1].breakPoints[i][Y],Snake.body[Snake.bodyWidth-1].breakPoints[i][dir]);
    }

    struct Snake_Body *new = realloc(Snake.body,sizeof(struct Snake_Body)*( Snake.bodyWidth+1));

    if(new == NULL){
        return;
    }

    new[Snake.bodyWidth]= added;
    Snake.bodyWidth++;
    Snake.body=new;
}

void updateBody(struct Snake_Body *body){

    if(body->breakPointWidth>0 && body->breakPoints[0][X] == body->x && body->breakPoints[0][Y]== body->y){
            body->direction= body->breakPoints[0][dir];
            body->breakPoints= memmove(body->breakPoints[0],body->breakPoints[1],sizeof(int[3])* (body->breakPointWidth-1));
            body->breakPointWidth--;

    }
    
    if(body->direction==RIGHT){
        body->x++;
    }

    if(body->direction==LEFT){
        body->x--;
    }

    if(body->direction==UP){
        body->y--;
    }

    if(body->direction==DOWN){
        body->y++;
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
            rx= getRandomNumber(WIDTH);
            ry= getRandomNumber(HEIGHT);
            if(checkIfSpaceIsFree(rx,ry)) break;
    }

    GameInfo.foodCoord[X]= rx;
    GameInfo.foodCoord[Y]= ry;

}



void checkFoodEaten(){
    if(Snake.head[X]==10 && Snake.head[Y]==1){
        addSnakeBody(LEFT);
        addSnakeBody(LEFT);
        addSnakeBody(LEFT);
        addSnakeBody(LEFT);
    }
}

int isGameOver(){
        if(Snake.head[Y]==0 || Snake.head[Y]== HEIGHT || Snake.head[X]==0 || Snake.head[X]==WIDTH){
            return 1;
            printf("Collision Detected: %d %d",Snake.head[X],Snake.head[Y]);
        }
        printf("Coordinates: %d %d",Snake.head[X],Snake.head[Y]);
        fflush(NULL);
    for(int i=0;i<Snake.bodyWidth;i++){
        for(int j=0;j<Snake.body[i].breakPointWidth;j++){
        
            if((Snake.head[X]== Snake.body[i].breakPoints[j][X] && Snake.head[Y]== Snake.body[i].breakPoints[j][Y])){
                return 1;

                      printf("Collision Detected: %d %d",Snake.head[X],Snake.head[Y]);
                      fflush(NULL);
                      usleep(REFRESH_TIME * 1000);
            }
        }
    }

    return 0;
}

int updateSnakePosition(int c){
    if((c==UP || c==DOWN || c==LEFT || c==RIGHT) && c!=previouslyPressedKey){
        previouslyPressedKey=c;

    for(int i=0;i<Snake.bodyWidth;i++){
        addBreakPoint(&Snake.body[i],Snake.head[X],Snake.head[Y],c);    
        }
    }

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
        for(int i=0;i<Snake.bodyWidth;i++){
            updateBody(&Snake.body[i]);
            
        }
        checkFoodEaten();
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
    struct GameBuff buf= GAME_BUFF_INIT;
     appendBuff(&buf,"\x1b[H\x1b[J",6);

    for(int i=0;i<=HEIGHT;i++){
        for(int j=0;j<=WIDTH;j++){
            // if(j==10 && i==1){
            //     appendBuff(&buf,"p",1);
            //     continue;
            //   }
            if(i==0 || i== HEIGHT || j==0 || j==WIDTH) {
                appendBuff(&buf,"-",1);
                continue;
            }
            if(gameOver) {
                appendBuff(&buf," ",1);
                continue;
            };
            if(i==(Snake.head[0]) && j==(Snake.head[1])){
                appendBuff(&buf,"o",1);
                continue;
            }
              
            int hasBody=0;
            // i can check if a body exist in the i and j of the snake body instead of the k and then determine to continue or not
            for(int k=0;k<Snake.bodyWidth;k++){
                int x= Snake.body[k].x;
                int y= Snake.body[k].y;
                if(i==y && j==x){
                       hasBody=1;
                       appendBuff(&buf,"#",1);
                       continue;
                }
            }
            if(hasBody) continue;
            appendBuff(&buf," ",1);
        }
        
        if(i != HEIGHT){
            appendBuff(&buf,"\r\n",2);
        }
        
    }
  
    write(STDOUT_FILENO, buf.buf, buf.len);
}

int main(){
    enableRawMode();
    int currentKey=RIGHT;
    srand(time(NULL));
    while(1){
        currentKey=readKeyPress(currentKey);
        gameOver=updateSnakePosition(currentKey);
        drawGame();
        if(gameOver){
            printf("\nGame Over\n");
            fflush(NULL);
            break;
        }
        usleep(REFRESH_TIME * 1000);
    }
}

//// debugging essentail

    //   printf("New breakpoint added %d %d %d",Snake.head[X],Snake.head[Y],c);
    //     fflush(NULL);
    //         usleep(REFRESH_TIME * 1000);

        //             for(int j=0;j<Snake.bodyWidth;j++){
    //     printf("\n Body [%d] breakpoints: Total: %d ",j,Snake.body[j].breakPointWidth)  ;
    //     for(int k=0;k<Snake.body[j].breakPointWidth;k++){
    //         printf("\tX: %d | Y: %d dir: %d",Snake.body[j].breakPoints[k][X],Snake.body[j].breakPoints[k][Y],Snake.body[j].breakPoints[k][dir]);
    //     }

    // }
    // fflush(NULL);  
    // usleep(5000 * 1000);