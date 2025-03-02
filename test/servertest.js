const WebSocket = require('ws');

const wss = new WebSocket.Server({port: 8080});

wss.on('connection', (ws) => {
	console.log('클라이언트가 연결됨');
	setInterval(() => {
		ws.send("Hello");
	}, 1000);
});
