const WebSocket = require('ws');
const {spawn} = require('child_process');
const express = require("express");
const http = require("http");
const path = require("path");

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({server});






//const aiScriptPath = path.resolve(__dirname, "SubwayAiTest.py");
//console.log(`실행할 AI모델 경로 : ${aiScriptPath}`);



const sensorProcess = spawn("./SubwaySensor"); // C언어 
const aiProcess = spawn("python3" ,["SubwayAiTest.py"], {encoding : "utf8"}); // AI 모델 실행

//const sensorData = JSON.stringify({distance : 15, tmperature : 30});
//console.log("AI 모델에 보낼 데이터 : " ,sensorData);
//console.log("데이터 타입 : ", typeof sensorData);
//aiProcess.stdin.write(sensorData + "\n");
aiProcess.stdin.end();


sensorProcess.stdout.on("data", (data) => {
	const messages = data.toString().trim().split("\n"); // 여러 개의 메세지가 한번에 들어올 경우 처리
	messages.forEach((message) => {
		//console.log("C에서 넘어온 데이터 : " ,message);
	
		try {
			const seatData = JSON.parse(message);
			console.log('Node에서 받은 데이터 : ' ,seatData);
			
			aiProcess.stdin.write(JSON.stringify(seatData) + "\n"); // AI Process로 데이터 전달
			
			wss.clients.forEach(client => {
				if (client.readyState == WebSocket.OPEN) {
					client.send(JSON.stringify(seatData));
				}
			});
		} catch (error) {
			console.error("JSON 파싱 오류 발생 : " ,error);
		}
	});
});

aiProcess.stdout.on("data", (data) => {
	console.error("AI 모델 오류 로그: " ,data.toString().trim()); 
	const messages = data.toString().trim().split("\n");
	messages.forEach((message) => {
		console.log("AI 분석 결과 (처리 전) : " ,message);
		
		try {
			const result = JSON.parse(message);
			console.log("AI 분석 결과 (처리 후) : " ,result);
			
			// 웹 클라이언트로 전송
			wss.clients.forEach(client => {
				if (client.readyState == WebSocket.OPEN) {
					client.send(JSON.stringify(result));
				}
			});
		} catch (error) {
			console.error("AI 결과 JSON 파싱 오류 발생 : " ,error);
			console.error("AI 프로세스 출력 원본 데이터 : " ,message);
		}
	});
});

/* stderr 출력 (디버깅용, WebSocket으로는 전송 안 함)
sensorProcess.stderr.on("data", (data) => {
	console.log("C 프로그램 오류 (디버깅) : ", data.toString().trim());
});*/

sensorProcess.on("close", (code) => {
	console.log('C 프로그램 종료 (코드: ${code})');
});

// GET 요청을 처리하는 Express 서버의 라우트 설정
app.get("/", (req, res) => res.sendFile(__dirname + '/SubwayWeb.html'));
server.listen(3000, () => console.log('웹 서버 실행 중 : http://localhost:3000'));
