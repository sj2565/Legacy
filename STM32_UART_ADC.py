import serial
import json
import time
import websocket
import re # 정규 표현식 라이브러리

# UART 통신 초기화
try :
    out = serial.Serial('/dev/ttyAMA0', 115200, timeout=1)
    print("연결 성공")
except Exception as e:
    print("연결 실패 : {e}")
    exit(1)

# WebSocket 주소 (Node.js)
WS_URL = "ws://localhost:3000"

# WebSocket 연결
ws = websocket.WebSocket()
ws.connect(WS_URL)

try:
    while True:
        data = out.readline().decode().strip()
        if data:
            print(f"UART 수신 데이터 : {data}")
            try:
                match = re.search(r"Pressure:\s*([\d.]+)", data) # ADC값 제외하고 Pressure : 숫자만 출력
                if match:
                    pressure_value = float(match.group(1)) # 추출된 값
                    print(f"Pressure Data: {pressure_value} kg")
                    
                    if pressure_value == 0:
                        print("값이 0임")
                    
                    # WebSocket을 통해 Node.js 서버로 데이터 전송
                    ws.send(json.dumps({"pressure": pressure_value}))
                    print(f" WeboSocket 전송 완료 : {pressure_value} kg")
                    
            except Exception as e:
                print("데이터 처리 오류")
        time.sleep(1)
        
except KeyboardInterrupt:
    print("안전하게 종료")
    ws.close()
    out.close()

