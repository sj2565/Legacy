const WebSocket = require('ws');
const {spawn} = require('child_process');

const wss = new WebSocket.Server({port: 8080});

const sensorProcess = spawn('./SubwayTest');

wss.on('connection', (ws) => {
	console.log("클라이언트 연결성공");

	sensorProcess.stdout.on('data', (data) => {
		ws.send(data.toString().trim());
	});

	ws.on('close', () => {
		console.log('클라이언트 연결해제');
	});
});

console.log('WebSocket server running on ws://localhost:8080');
