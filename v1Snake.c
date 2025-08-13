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
    return ((rand() % maxRange) +1);
}



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

    direction=Snake.bodyWidth!=0? Snake.body[Snake.bodyWidth-1].direction: direction;

    if(direction==UP){
        lastPosY+=1;
    }else if(direction==DOWN){
        lastPosY-=1;
    }else if(direction==LEFT){
        lastPosX+=1;
    }else if(direction==RIGHT){
        lastPosX-=1;
    }

    int breakPointWidth= Snake.bodyWidth==0? 0 : Snake.body[Snake.bodyWidth-1].breakPointWidth;
    struct Snake_Body added = {.y=lastPosY,.x=lastPosX, .breakPoints=NULL,.breakPointWidth=0,.direction=direction};

    for(int i=0;i<breakPointWidth;i++){
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
             memmove(body->breakPoints,body->breakPoints+1,sizeof(int[3])* (body->breakPointWidth-1));
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
        addSnakeBody(GameInfo.previouslyPressedKey);
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
     
            if(i==0 || i== HEIGHT || j==0 || j==WIDTH) {
                appendBuff(&buf,"-",1);
                continue;
            }
            
            if(GameInfo.gameOver) {
                char score[20];
                int scoreLen=snprintf(score,sizeof(score),"Score: %d",GameInfo.score);
                if(i== (HEIGHT/2)-1 && j== ((WIDTH/2)-(scoreLen/2))){
                    appendBuff(&buf,score,scoreLen);
                    j+=(scoreLen-1);
                    continue;
                }
                if(i== HEIGHT/2 && j== ((WIDTH/2)-4)){
                    appendBuff(&buf,"Game Over",9);
                    j+=8;
                    continue;
                }
                appendBuff(&buf," ",1);
                continue;
            };
            if(i==(Snake.head[0]) && j==(Snake.head[1])){
                appendBuff(&buf,"o",1);
                continue;
            }
              
            int hasBody=0;
            for(int k=0;k<Snake.bodyWidth;k++){
                int x= Snake.body[k].x;
                int y= Snake.body[k].y;
                if(i==y && j==x){
                       hasBody=1;
                       appendBuff(&buf,"#",1);
                       continue;
                }
            }
            
            if(j==GameInfo.foodCoord[X] && i==GameInfo.foodCoord[Y]){
                 appendBuff(&buf,"f",1);
                continue;
            
            }

            if(hasBody) continue;
            appendBuff(&buf," ",1);
        }
        
        if(i != HEIGHT){
            appendBuff(&buf,"\r\n",2);
        }
        
    }
  
    write(STDOUT_FILENO, buf.buf, buf.len);
    free(buf.buf);
}

void cleanUp(){
    for(int i=0;i<Snake.bodyWidth;i++){
        free(Snake.body[i].breakPoints);
    }
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
