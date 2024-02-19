#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#define MAX_BUFFER_SIZE 8192
void delay(int milliseconds) {
    clock_t start_time = clock();
    while (clock() < start_time + milliseconds)
        ;
}
void transferFile(const char* ip, int port, int priority, const char* filename,const int MbytesPerSecond) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        printf("無法開啟檔案：%s\n", filename);
        return;
    }
    //printf("\n%d\n",CLOCKS_PER_SEC);
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    int buffer_size = MAX_BUFFER_SIZE;
    char* buffer = (char*)malloc(buffer_size * sizeof(char));

    int transmitted_bytes = 0;
    
        
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("無法建立套接字");
        return;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority)) < 0){
        perror("so_priority");
        return;
    }
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("連線失敗");
        return;
    }
    long target_time = buffer_size / MbytesPerSecond;
    struct timespec start, t_s, t_e, end;
    clock_gettime(CLOCK_REALTIME, &start);
    while (transmitted_bytes < file_size) {
        int read_bytes = fread(buffer, sizeof(char), buffer_size, file);
        transmitted_bytes += read_bytes;
        clock_gettime(CLOCK_REALTIME, &t_s);
        // 在此處執行傳輸，例如將buffer內容寫入網路套接字
        if (send(sockfd, buffer, read_bytes, 0) == -1) {
                    perror("傳送失敗");
                    break;
        }
        clock_gettime(CLOCK_REALTIME, &t_e);
        long elapsed_time = (t_e.tv_sec - t_s.tv_sec)*1e6+(t_e.tv_nsec - t_s.tv_nsec)*1e-3;
        long delay_time = (target_time - elapsed_time)-55;
        //printf("%d\t%d\n",target_time,elapsed_time);
        // 執行延遲以控制傳輸速率
        if (delay_time > 0)
            usleep(delay_time);
    }
    clock_gettime(CLOCK_REALTIME, &end);
    double end_time = (end.tv_sec - start.tv_sec)+1e-9 * (end.tv_nsec - start.tv_nsec);
    printf("%.3f %.3f\n",end_time, transmitted_bytes/end_time);
    fclose(file);
    
}

int main(int argc, char *argv[]) {
    const char* ip = argv[1];
    const int port = atoi(argv[2]);
    const int priority = atoi(argv[3]);
    const int MbytesPerSecond = atoi(argv[4]); // 傳輸速率（每秒 bytes）
    const char* filename = argv[5];
    //printf("%d\n",MbytesPerSecond);
    transferFile(ip,port,priority,filename, MbytesPerSecond);
    
    return 0;
}
