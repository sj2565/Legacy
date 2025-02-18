const WebSocket = require('ws');
const {spawn} = require('child_process');
const express = require("express");
const http = require("http");

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({server});

const sensorProcess = spawn("./SubwaySensor");

sensorProcess.stdout.on("data", (data) => {
	const message = data.toString().trim();
	console.log("C에서 넘어온 data : ", message);
	
	try {
		const seatData = JSON.parse(message);
		//console.log('받은 데이터 : ', seatData);
		
		wss.clients.forEach(client => {
			if (client.readyState == WebSocket.OPEN) {
				client.send(JSON.stringify(seatData));
			}
		});
	} catch (error) {
		console.error("JSON 파싱 오류 발생 : ", error);
	}
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
