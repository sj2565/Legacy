#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

// GPIO
#define TRIG 17
#define ECHO 27
#define LED_RED 23
#define LED_GREEN 24
#define TEMP 25
#define MAX_TIMINGS 90 // DHT11센서는 LOW <-> HIGH 전환이 약85번 발생함
#define FILE_PATH "/home/seojoon/nodetest/sensor_data.csv"

// 40비트 데이터 (5바이트)
int data[5] = {0, 0, 0, 0, 0};

// GPIO Reset
void CleanUp(int signum)
{
    printf("GPIO 핀번호 리셋 \n");
    digitalWrite(TRIG, LOW);

    pinMode(TRIG, INPUT);
    pinMode(ECHO, INPUT);
    pinMode(LED_RED, INPUT);
    pinMode(LED_GREEN, INPUT);
    pinMode(TEMP, INPUT);

    printf("센서 안전하게 종료! \n");
    exit(0);
}

void SetUp()
{
    if (wiringPiSetupGpio() == -1)
    {
        fprintf(stderr, "wiringPi 초기화 실패 \n");
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

float GetTemperature()
{
    int last_state = HIGH;
    int counter = 0;
    int j = 0;
    float temperature;
    int retry_count = 0;
	
	static float last_valid_temp = 25.0; // 마지막 정상 값을 저장할 변수
	
    data[0] = data[1] = data[2] = data[3] = data[4] = 0;
	
	// 요청 신호 보내기
	pinMode(TEMP, OUTPUT);
	digitalWrite(TEMP, LOW); 
    delay(20);	// 센서를 사용하기 위해 20ms 동안 LOW신호 보냄
    digitalWrite(TEMP, HIGH);
	delayMicroseconds(40);  // HIGH 상태를 40마이크로초 동안 유지
    
    // 데이터 수신 준비 (INPUT모드로 변경)
    pinMode(TEMP, INPUT);

    for (int i = 0; i < MAX_TIMINGS; i++)
    {
        counter = 0;
        while (digitalRead(TEMP) == last_state) // 신호가 바뀔 때까지 대기
        {
            counter++; // 신호가 변경되지 않는 동안 카운트 증가
            delayMicroseconds(1);
            if (counter == 255)
            {
                break; // 255 마이크로초 이상 기다리면 타임아웃
            }
        }
        last_state = digitalRead(TEMP); // 현재 상태 업데이트

		// HIGH 신호의 길이로 0/1을 구분
        if ((i >= 4) && (i % 2 == 0))
        {
            data[j / 8] <<= 1; // 기존 데이터 왼쪽으로 쉬프트
            if (counter > 50) // HIGH 신호가 50마이크로 이상이면 1, 아니면 0
            {
                data[j / 8] |= 1;
            }
            j++;
        }
    }
    // 패리티 체크
    if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)))
    {
        temperature = data[2] + data[3] * 0.1;
        
        // 비정상적인 값 필터링
        if (temperature > 20 && temperature < 40)
        {
			last_valid_temp = temperature;
		}
		else
		{
			return last_valid_temp;
        }
        return temperature;
    }
}
void SaveData(float distance, float temperature){
	FILE *file = fopen(FILE_PATH, "a");
	if (file == NULL) {
		fprintf(stderr, "파일 열기 실패! \n");
		return;
	}
	//fprintf(stderr, "SaveData() 호출! : %.2f, %.2f\n" ,distance ,temperature);
	fprintf(file, "%.2f, %.2f\n" ,distance ,temperature);
	fclose(file);
}

int main()
{
    SetUp();
    while (1)
    {
		struct timeval start_time, end_time;
		gettimeofday(&start_time, NULL); // 시작 시간 측정
		
        float distance = GetDistance();
        float temperature = GetTemperature();
       
        SaveData(distance, temperature); // 파일 저장 
        
		// 디버깅 메세지로 출력, 버퍼로 인해 printf문으로 사용할 시 같이 Node로 넘어가기에 오류가 발생함
		// 터미널에서 stderr 로그 출력
        fprintf(stderr, "Distance : %.2f cm | Temperature : %.1f \n", distance, temperature);
        
        // 센서 데이터값 0과1로 표현
        int seat_data = (distance < 20.0 && temperature > 30.0) ? 1 : 0;
        
        // Node에서 실시간으로 읽을 수 있게 설정
        printf("{\"seat\" : %d}\n", seat_data); // JSON
        fflush(stdout);     // 버퍼를 즉시 비우면서 출력을 강제로 수행 -> printf문 전부 출력

        if (seat_data == 1)
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
