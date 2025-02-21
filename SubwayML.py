import os
import sys
import json
import pandas as pd
import numpy as np
from scipy import stats
from sklearn.ensemble import IsolationForest
from sklearn.preprocessing import MinMaxScaler

#print("AI 모델 실행됨! (CSV 기반 초기 학습 시작, 튜닝 X)", file=sys.stderr)
print("AI 모델 실행됨! (실시간 이상 탐지 학습 시작, 튜닝 O)", file=sys.stderr)

# CSV 파일 로드
file_path = "sensor_data.csv"
df_raw = pd.read_csv(file_path, names=["distance", "temperature"], header=None, dtype={"distance": np.float32, "temperature": np.float32})
df = pd.read_csv(file_path, names=["distance", "temperature"], header=None, dtype={"distance": np.float32, "temperature": np.float32})

'''
# 원본 데이터로 진행한 모델 학습
training_data_raw = df_raw.to_numpy()

model_raw = IsolationForest(n_estimators=100,	# 트리 개수 증가
        contamination=0.05,					# 이상값 비율 낮춤 (더 민감)
        max_samples=256,					# 트리 샘플 크기 설정
        max_features=2,						# 특징 수(거리, 온도)
        random_state=42)					# 재현 가능성 유지
model_raw.fit(training_data_raw)

# 전처리 후 이상값 확인
predictions_raw = model_raw.predict(training_data_raw)
df_raw["prediction"] = predictions_raw
df_raw["status"] = df_raw["prediction"].apply(lambda x: "normal" if x == 1 else "anomaly")

# 이상값 확인
original_anomalies = df_raw[df_raw["status"] == "anomaly"]
print("\n 원본 데이터 기반 이상값 탐지 결과 :")
print(original_anomalies)
'''

# 데이터 전처리 과정 후 모델 학습
# 결측값 처리 (NaN값 보정)
df.interpolate(method="linear", inplace=True) # 선형 보간(데이터 흐름 유지)
df.fillna(df.mean(), inplace=True) # 평균값으로 NaN값 보정

# 이상값 제거 (Z-Score) 너무 많이 제거하면 학습 범위가 좁아짐 => 정상값도 이상인식 => 완화하거나 해제
#z_scores = np.abs(stats.zscore(df))
#df = df[(z_scores < 4).all(axis=1)] # Z-score가 4이상인 값 제거

# 데이터 정규화
scaler = MinMaxScaler()
scaler.fit(df[["distance", "temperature"]]) # 최소값과 최대값을 찾아서 transform에 적용 => 실시간 데이터는 CSV 데이터의 최소/최대 값을 기준으로 변환
df[["distance", "temperature"]] = scaler.transform(df[["distance", "temperature"]])

# 이동 평균(SMA) 추가로 인한 노이즈 완화)
#df["sma_distance"] = df["distance"].rolling(window=3).mean()
#df["sma_temp"] = df["temperature"].rolling(window=3).mean()

# Sliding Window 적용 (과거 데이터 반영)
df["prev_distance_1"] = df["distance"].shift(1)
df["prev_temp_1"] = df["temperature"].shift(1)
df.dropna(inplace=True)  # NaN값 제거

# 최종 데이터 
training_data = df.to_numpy() # (n_sample, 4) => distance, temperature, prev_dis, prev_temp
#print(training_data)

# AI 모델 학습
model = IsolationForest(n_estimators=200,	# 트리 개수 증가
        contamination=0.01,					# 이상값 비율 낮춤 (더 민감)
        max_samples="auto",					# 트리 샘플 크기 설정
        max_features=4,						# 특징 수(거리, 온도, 이전 거리, 이전 온도)
        random_state=42)					# 재현 가능성 유지
model.fit(training_data)

print("CSV파일 기반 AI 모델 학습 완료!", file = sys.stderr)
'''
# 전처리 후 이상값 확인
predictions = model.predict(training_data)
df["prediction"] = predictions
df["status"] = df["prediction"].apply(lambda x: "normal" if x == 1 else "anomaly")

# 이상값 확인
anomalies = df[df["status"] == "anomaly"]
print("\n 전처리 후 AI 모델이 탐지한 이상값 :")
print(anomalies)
'''

# 실시간 데이터 분석 (stdin을 통해 데이터 입력받기)
while True:
    line = sys.stdin.readline().strip()
    if not line:
        continue  # 빈 줄은 무시하고 계속 실행

    try:
        data = json.loads(line)
        distance = float(data.get("distance", np.nan)) # 겉측값 방지
        temperature = float(data.get("temperature", np.nan))
       
        # 결측값 처리 (NaN -> 이전 값으로 대체)
        if np.isnan(distance):
            distance = prev_distance if prev_distance is not None else df["distance"].mean()
        if np.isnan(temperature):
            temperature = prev_temperature if prev_temperature is not None else df["temperature"].mean()
            	
        # 이상값 제거 (Z-Score, (데이터 - 평균) / 표준편차)
        std_distance = df["distance"].std()
        std_temperature = df["temperature"].std()
                
        z_scores_distance = (distance - df["distance"].mean()) / std_distance
        z_scores_temperature = (temperature - df["temperature"].mean()) / std_temperature
       
        if abs(z_scores_distance) > 4.0 or abs(z_scores_temperature) > 4.0:
            anomaly_flag = True
        else:
            anomaly_flag = False
		
        # 정규화 작업 실시, 위에 데이터 전처리 과정이랑 일관성 갖기, scaler.fit() 재호출 X              
        new_data_2d = np.array([[distance, temperature]])
        new_data_2d = scaler.transform(new_data_2d)
		
        #  Sliding Window 적용, 학습 시와 실시간 시 동일 차원으로 맞추기 
        if "prev_distance" not in locals():
            prev_distance, prev_temperature = distance, temperature # 초기값 설정
        new_data_4d= np.array([[new_data_2d[0, 0], new_data_2d[0, 1], prev_distance, prev_temperature]])
        prev_distance, prev_temperature = new_data_2d[0, 0], new_data_2d[0, 1] # 이전 값 업데이트

        # 모델 예측 수행
        prediction = model.predict(new_data_4d)
        result = "normal" if prediction[0] == 1 else "anomaly"
		
        output = json.dumps({
            "status": result,
            "distance": float(distance),
            "temperature": float(temperature)
        })
        print(output)
        sys.stdout.flush()

    except Exception as e:
        print(json.dumps({"error": str(e)}))
        sys.stdout.flush()
