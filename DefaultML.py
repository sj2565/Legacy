import os
import sys
import json
import pandas as pd
import numpy as np
from sklearn.ensemble import IsolationForest

print("AI 모델 실행됨! (CSV 기반 초기 학습 + 실시간 분석 시작)", file=sys.stderr)

file_path = "sensor_data.csv"
df = pd.read_csv(file_path, names=["distance", "temperature"], header=None)
training_data = df.to_numpy()

model = IsolationForest(n_estimators=200,	
        contamination=0.05,					
        max_samples=256,					
        max_features=2,						
        random_state=42)					
model.fit(training_data)
'''
print("AI 모델 학습 완료!", file = sys.stderr)

predictions = model.predict(training_data)

df["prediction"] = predictions
df["status"] = df["prediction"].apply(lambda x: "normal" if x == 1 else "anomaly")

anomalies = df[df["status"] == "anomaly"]
print(anomalies)
'''

while True:
    line = sys.stdin.readline().strip()
    if not line:
        continue  

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
