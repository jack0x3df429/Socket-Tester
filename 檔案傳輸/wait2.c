#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void waitUntilNextMinute(int Wait) {
    time_t now;
    struct tm *currentTime;
    int currentMinute, currentSecond;
    int targetMinute,targetSecond;

    // 取得當前時間
    time(&now);
    currentTime = localtime(&now);
    printf("現在時間 %d:%d\n" ,currentTime->tm_min,currentTime->tm_sec);
    // 取得當前分鐘和秒數
    currentMinute = currentTime->tm_min;
    currentSecond = currentTime->tm_sec;

    // 計算目標分鐘數
    if (currentSecond >= 40) {
        targetMinute = (currentMinute + (int)(Wait/60)+2);
    } else {
        targetMinute = (currentMinute + (int)(Wait/60)+1) ;
    }
    targetSecond=Wait%60;
    printf("TG時間 %d:%d\n" ,targetMinute,targetSecond);
    //return;
    // 等待直到達到目標時間
    while (1) {
        time(&now);
        currentTime = localtime(&now);
        //printf("%d:%d\n",targetMinute,targetSecond);
        if (currentTime->tm_min == targetMinute && currentTime->tm_sec == targetSecond) {
            break;
        }
        if(targetSecond-currentTime->tm_sec>0)
          printf("%d:%d\n",targetMinute-currentTime->tm_min,targetSecond-currentTime->tm_sec);
        else
          printf("%d:%d\n",targetMinute-1-currentTime->tm_min,60-currentTime->tm_sec+targetSecond);
    }
    printf("現在時間 %d:%d\n" ,currentTime->tm_min,currentTime->tm_sec);
}

int main(int argc, char *argv[]) {
    //printf("現在時間");
    int Wait = atoi(argv[1]);
    waitUntilNextMinute(Wait);
    printf("已到達目標時間！\n");

    return 0;
}
