import os
import sys
import json
import pandas as pd
import numpy as np
from sklearn.ensemble import IsolationForest

print("AI 모델 실행됨! (CSV 기반 초기 학습 + 실시간 분석 시작)", file=sys.stderr)

# CSV 파일 로드
file_path = "sensor_data.csv"
df = pd.read_csv(file_path, names=["distance", "temperature"], header=None)
training_data = df.to_numpy()

# AI 모델 학습
model = IsolationForest(n_estimators=200,	# 트리 개수 증가
        contamination=0.05,					# 이상값 비율 낮춤 (더 민감)
        max_samples=256,					# 트리 샘플 크기 설정
        max_features=2,						# 특징 수(거리, 온도)
        random_state=42)					# 재현 가능성 유지
model.fit(training_data)

print("AI 모델 학습 완료!", file = sys.stderr)

'''
# CSV 파일의 데이터가 정상인지 이상값인지 판별
predictions = model.predict(training_data)

# 결과 출력
df["prediction"] = predictions
df["status"] = df["prediction"].apply(lambda x: "normal" if x == 1 else "anomaly")

# 이상값 확인
anomalies = df[df["status"] == "anomaly"]
print(anomalies)
'''

# 실시간 데이터 분석 (stdin을 통해 데이터 입력받기)
while True:
    line = sys.stdin.readline().strip()
    if not line:
        continue  # 빈 줄은 무시하고 계속 실행

    try:
        data = json.loads(line)
        distance = data["distance"]
        temperature = data["temperature"]

        new_data = np.array([[distance, temperature]])
        prediction = model.predict(new_data)
        result = "normal" if prediction[0] == 1 else "anomaly"

        output = json.dumps({
            "status": result,
            "distance": distance,
            "temperature": temperature
        })
        print(output)
        sys.stdout.flush()

    except Exception as e:
        print(json.dumps({"error": str(e)}))
        sys.stdout.flush()
