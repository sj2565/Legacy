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
        printf("wiringPi 초기화 실패 \n");
        exit(1);
    }
    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);

    digitalWrite(TRIG, LOW);
    printf("초음파 센서 및 온도 센서 준비 완료 \n");

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

    while (digitalRead(ECHO) == LOW)
        ;
    gettimeofday(&start, NULL);

    while (digitalRead(ECHO) == HIGH)
        ;
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

    data[0] = data[1] = data[2] = data[3] = data[4] = 0;
	
	// 요청 신호 보내기
	pinMode(TEMP, OUTPUT);
	digitalWrite(TEMP, LOW); 
    delay(20);	// 센서를 사용하기 위해 18ms 동안 LOW신호 보냄
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
        return temperature;
    }
    /*else
    {
        printf("센서값 데이터 읽기 실패 \n");
        return -1.0;
    }*/
}

int main()
{
    SetUp();
    while (1)
    {
        float distance = GetDistance();
        float temperature = GetTemperature();

        printf("Distance : %.2f cm | Temperature : %.1f \n", distance, temperature);

        if (distance < 20.0 && temperature > 30.0)
        {
            digitalWrite(LED_RED, HIGH);
            digitalWrite(LED_GREEN, LOW);
            delay(500);
        }
        else
        {
            digitalWrite(LED_RED, LOW);
            digitalWrite(LED_GREEN, HIGH);
            delay(500);
        }
        sleep(1); // delay(1000);
    }
    return 0;
}
