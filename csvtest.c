#include <stdio.h>

#define FILE_PATH "/home/seojoon/nodetest/sensor_data.csv"

int main(){
	FILE *file = fopen(FILE_PATH, "a");  
    if (file == NULL) {
        printf("파일을 열 수 없음: %s\n", FILE_PATH);
        return 1;
    }

    printf("파일이 정상적으로 열림!\n");

    fprintf(file, "10.0,25.5\n");  // 
    fclose(file);

    printf("데이터가 저장되었음: %s\n", FILE_PATH);

    return 0;
}
