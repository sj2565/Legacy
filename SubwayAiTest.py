import sys
import json
import numpy as np
from sklearn.ensemble import IsolationForest
import os
import pandas as pd

# 센서 데이터 불러오기
file_path = "/home/seojoon/nodetest/sensor_data.csv"

#if os.path.exists(file_path):
#	print("ok")
#else :
#	print("X")	

try:
	df = pd.read_csv(file_path, names=["distance", "temperature"] ,header=None)
	training_data = df.to_numpy() # 배열로 변환
	print("CSV 파일이 정상적으로 로드됨!" ,file = sys.stderr)
	print(df.head(), file = sys.stderr)

except:
	print("센서 데이터 파일이 없습니다! 기본값 사용")
	training_data = np.array([
	[10, 25], [11, 26], [12, 24], [13, 31], [9, 33], [10, 34]])

# 튜닝 Isolation Forest 모델 학습
model = IsolationForest(
    n_estimators = 200,      # 트리 개수 증가
    contamination = 0.05,    # 이상값 비율 낮춤 (더 민감)
    max_samples = 256,       # 트리 샘플 크기 설정
    max_features = 2,        # 특징 수(거리, 온도)
    random_state = 42        # 재현 가능성 유지
)
model.fit(training_data)

print("AI 모델 실행됨", file=sys.stderr)

# Node.js에서 실시간 데이터 입력 받기 
for line in sys.stdin:
    try:
        print(f" 받은 원본  데이터 : {repr(line.strip())}", file=sys.stderr)
        print(f" 받은 데이터 타입 : {type(line.strip())}", file=sys.stderr)
        # JSON 데이터 읽기 
        data = json.loads(line.strip())
        print(f" jSON 파싱된 데이터 : {data}", file=sys.stderr)  # JSON 정상 파싱 확인
        
        if not isinstance(data, dict):
            raise ValueError(" 데이터가 JSON 객체가 아님")
	
        if "distance" not in data or "temperature" not in data:
            raise ValueError("키가 존재하지 않음")

        new_data = np.array([[data["distance"], data["temperature"]]])

        # 모델
        prediction = model.predict(new_data)
        result = "normal" if prediction[0] == 1 else "anomaly"

        # 최종 결과
        print(json.dumps({
            "status": result,  # 튜닝된 모델 결과를 웹에 표시
            "distance": data["distance"],
            "temperature": data["temperature"]
        }))
        sys.stdout.flush() # 버퍼 비우기

    except Exception as e:
        print(json.dumps({"error": str(e)}))
        sys.stdout.flush()
