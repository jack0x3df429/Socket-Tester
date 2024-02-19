#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <math.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <time.h>
#define MAX_BUFFER_SIZE 1000
long long tstoll(struct timespec ts) {
    return ts.tv_sec*(int)1e9+ts.tv_nsec;
}
void delay(int milliseconds) {
    clock_t start_time = clock();
    while (clock() < start_time + milliseconds)
        ;
}
void delayns(long nanoseconds) {
    struct timespec start_time, now_time;
    clock_gettime(CLOCK_REALTIME, &start_time);
    //printf("%d\n",nanoseconds);
    do{
        clock_gettime(CLOCK_REALTIME, &now_time);
        //printf("%d\n",(int)1e9*(now_time.tv_sec - start_time.tv_sec) + now_time.tv_nsec - start_time.tv_nsec);
    }while(tstoll(now_time)-tstoll(start_time) < nanoseconds);
}
int brownian(int Now, int Bitrate, int Jitter,int max_steps) {
    int max = Bitrate+Jitter; // 最大位置
    int min = Bitrate-Jitter; // 最小位置
    int random_step = rand() % (max_steps*2+1) - max_steps; // 生成隨機步長，取值範圍為[-1, 1]
    Now += random_step; // 更新位置
    // 範圍檢查
    if (Now > max) {
        Now = max;
    } else if (Now < min) {
        Now = min;
    }
    return Now;
}
int prob_jump(int Bitrate,float rate, float amp) {
    float a=log2f(amp);
    int Now = Bitrate;
    float random_value = (float)(rate+a)*((float)rand()/(float)(RAND_MAX));
    int random_step = pow(2,random_value-rate);
    Now += random_step; // 更新位置
    return Now;
}
void transfer(const char* ip, int port, int priority, int duration,const int MbytesPerSecond,const int mode,const int jitter) {
	char file_name_byte_rate[7]="t_sbrt";
	sprintf(file_name_byte_rate+5,"%d",port%10);
	FILE *file_Bytesrate = fopen(file_name_byte_rate,"wb");
    int buffer_size = MAX_BUFFER_SIZE;
    char* buffer = (char*)malloc(buffer_size * sizeof(char));
    long long transmitted_bytes = 0;
    //socket
    //int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("無法建立套接字");
        return;
    }
    
    int flag = 1;
    /*if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int))<0){
        perror("nodelay");
        return;
    }*/

    /*if (setsockopt(sockfd, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority)) < 0){
        perror("so_priority");
        return;
    }*/
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip);
    /*if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("連線失敗");
        return;
    }*/
    //printf("%d\n",target_time);
    struct timeval start, end;
    struct timespec t_s,t_e;
    int buffer_send;
    int c=0;
    int BITRATE=MbytesPerSecond;
    long delay_time=0,wait_time=0;
    long long t_s_llong;
    memset(buffer,0,sizeof(buffer));
    gettimeofday(&start,NULL);
    do {
        clock_gettime(CLOCK_REALTIME, &t_s);
        t_s_llong=tstoll(t_s);
        memcpy(buffer,&(t_s_llong),sizeof(long long));
        buffer_send=sendto(sockfd, buffer, buffer_size, 0,(struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (buffer_send == -1) {
                    perror("傳送失敗");
                    break;
        }
        //printf("%lld %d\n",t_s_llong,delay_time);
        transmitted_bytes += buffer_send;
        if((t_e.tv_sec - start.tv_sec)%2==0){
			if(c==1){
				c=0;
				if(mode==1)
					BITRATE=prob_jump(MbytesPerSecond,40,jitter);
				else if(jitter>0)
					BITRATE=brownian(BITRATE,MbytesPerSecond,jitter,3);
				fprintf(file_Bytesrate,"%d\n",BITRATE);	
				printf("%d\n",BITRATE);
            }
		}
		else if(c==0)
			c=1;
        wait_time = (long)(buffer_send / BITRATE)*1024;
        // 在此處執行傳輸，例如將buffer內容寫入網路套接字
        delay_time= tstoll(t_s)-tstoll(t_e);
        
        memcpy(buffer+100,&(delay_time),sizeof(long));
        //t_s_llong=tstoll(t_s)+wait_time;
        do{clock_gettime(CLOCK_REALTIME, &t_e);}
        while(tstoll(t_e)<tstoll(t_s)+wait_time);
        memcpy(&t_e, &t_s, sizeof(struct timespec));
        /*if(delay_time>0)
            delayns(delay_time);
        else
            delay_time=0;*/
    }while (t_e.tv_sec - start.tv_sec <= duration);
    double end_time = (t_e.tv_sec - start.tv_sec)+1e-6 * (t_e.tv_nsec/1000 - start.tv_usec);
    printf("%d %lld %.3f %.3f\n",priority, transmitted_bytes, end_time, transmitted_bytes/end_time);
    //printf("%d",count);
    for (int i=0;i<10;i++){
        sendto(sockfd, buffer, 0, 0,(struct sockaddr*)&serverAddr, sizeof(serverAddr));
        sleep(1);
    }
    close(sockfd);
}

int main(int argc, char *argv[]) {
    srand(time(NULL)); // 初始化随机数种子
    char ip[15] ="192.168.88.";
    strcpy(ip+strlen(ip),argv[1]);
    const int port = atoi(argv[2]);
    const int priority = atoi(argv[2])%10;
    const int duration = atoi(argv[3]);
    const int MbytesPerSecond = atoi(argv[4]); // 傳輸速率（每秒 bytes）
    const int mode = atoi(argv[5]);
    const int jitter = atoi(argv[6]);
    transfer(ip,port,priority,duration, MbytesPerSecond,mode,jitter);
    
    return 0;
}
