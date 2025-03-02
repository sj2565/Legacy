const WebSocket = require('ws');

const wss = new WebSocket.Server({ port: 8080 });

console.log("WebSocket 서버가 시작(포트 8080)");

wss.on('connection', (ws) => {
	console.log('클라이언트 연결 성공!!!');
	
	ws.on('message', (message) => {
		console.log("index 메세지 : ", message);
	});

	setInterval(() => {
		let msg = "server message";
		ws.send(msg);
		console.log("서버에서 보낸 메세지 전송 : ", msg);
	}, 2000);
});

