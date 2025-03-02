#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

// GPIO
#define TRIG 17
#define ECHO 27
#define LED_RED 23
#define LED_GREEN 24
#define SPI_CHANNEL 0  // SPI 채널 (CEO 사용)
#define SPI_SPEED 1000000  // 1MHz 속도
#define LM35_CHANNEL 0  // MCP3008의 0번 채널 사용
//#define FILE_PATH "/home/seojoon/nodetest/sensor_data.csv"

// GPIO Reset
void CleanUp(int signum)
{
    printf("GPIO 핀번호 리셋 \n");
    digitalWrite(TRIG, LOW);

    pinMode(TRIG, INPUT);
    pinMode(ECHO, INPUT);
    pinMode(LED_RED, INPUT);
    pinMode(LED_GREEN, INPUT);

    printf("센서 안전하게 종료! \n");
    exit(0);
}

// 핀출력 설정
void SetUp()
{
	if (wiringPiSetupGpio() == -1 || wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) == -1) {
        fprintf(stderr, "SPI 초기화  or wiringPi 초기화 실패!\n");
        exit(1);
    }
    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);

    digitalWrite(TRIG, LOW);
    fprintf(stderr, "초음파 센서 및 온도 센서 준비 완료 \n");

    signal(SIGINT, CleanUp);
}

float GetDistance()
{
    struct timeval start, stop;
    long elapsed_time;
    float distance;

    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);

    while (digitalRead(ECHO) == LOW);
    gettimeofday(&start, NULL);

    while (digitalRead(ECHO) == HIGH);
    gettimeofday(&stop, NULL);

    elapsed_time = (stop.tv_sec - start.tv_sec) * 1000000 + (stop.tv_usec - start.tv_usec);
    distance = (elapsed_time * 0.0343) / 2;

    return distance;
}

float GetTemperature(){
	uint8_t buffer[3];
    int adc_value;
    float voltage, temperature;
	
	buffer[0] = 1; // MCP3008 시작 비트
    buffer[1] = (8 + LM35_CHANNEL) << 4; // 채널 선택
    buffer[2] = 0;
    
    wiringPiSPIDataRW(SPI_CHANNEL, buffer, 3);
    
     // 10비트 ADC 값 변환
    adc_value = ((buffer[1] & 3) << 8) + buffer[2];

    // 전압 변환 (3.3V 기준)
    voltage = (adc_value * 3.3) / 1023.0;

    // 온도 변환 (LM35는 10mV/C)
    temperature = voltage * 100.0;

    return temperature;
}

// 데이터 파일 저장
/*
void SaveData(float distance, float temperature){
	FILE *file = fopen(FILE_PATH, "a");
	if (file == NULL) {
		fprintf(stderr, "파일 열기 실패! \n");
		return;
	}
	//fprintf(stderr, "SaveData() 호출! : %.2f, %.2f\n" ,distance ,temperature);
	fprintf(file, "%.2f, %.2f\n" ,distance ,temperature);
	fclose(file);
} */

int main()
{
    SetUp();
    while (1)
    {
		struct timeval start_time, end_time;
		gettimeofday(&start_time, NULL); // 시작 시간 측정
		
        float distance = GetDistance();
        float temperature = GetTemperature();
       
        //SaveData(distance, temperature); // 파일 저장 
        
		// 거리 및 온도 데이터 JSON 형식으로 출력
        printf("{\"distance\" : %.2f, \"temperature\" : %.1f}\n", distance, temperature);
        fflush(stdout);     // 버퍼를 즉시 비우면서 출력을 강제로 수행 -> printf문 전부 출력

        if (distance < 20.0 && temperature > 30.0)
        {
            digitalWrite(LED_RED, HIGH);
            digitalWrite(LED_GREEN, LOW);
        }
        else
        {
            digitalWrite(LED_RED, LOW);
            digitalWrite(LED_GREEN, HIGH);           
        }
        
        gettimeofday(&end_time, NULL); // 끝난 시간 측정
        
        // 수행 시간 계산 (마이크로초 -> 초 변환)
        long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000 + 
                            (end_time.tv_usec - start_time.tv_usec);
        long remaining_time = 5000000 - elapsed_time; // 5초 
        
        // 남은 시간이 있으면 sleep
        if (remaining_time > 0)
        {
			usleep(remaining_time); // 5초에서 실행 시간을 뺀 만큼 대기
        }
	}
    return 0;
}
