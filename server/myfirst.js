const fs = require('fs');
const path = require('path');
const net = require('net');
const express = require('express');
const http = require('http');
const socketIo = require('socket.io');

const WEB_SOCKET_PORT = 3001;
const TCP_PORT = 3002;
const HOST = '192.168.11.4';

let isRecording = false;
let csvFiles = {};

function getTime(){
    var date = new Date();
    let time_now = date.getFullYear()  //現在時刻
        + '/' + ('0' + (date.getMonth() + 1)).slice(-2) 
        + '/' +('0' + date.getDate()).slice(-2) 
        + ' ' +  ('0' + date.getHours()).slice(-2) 
        + ':' + ('0' + date.getMinutes()).slice(-2) 
        + ':' + ('0' + date.getSeconds()).slice(-2) 
        + '.' + date.getMilliseconds();
    return time_now;
}

function setupWebSocketServer() {
    const app = express();
    const server = http.createServer(app);
    const io = socketIo(server, {
        cors: {
            origin: "*",
            methods: ["GET", "POST"]
        }
    });

    server.listen(WEB_SOCKET_PORT, () => {
        console.log(`WebSocket server listening on port ${WEB_SOCKET_PORT}`);
    });

    io.on('connection', (socket) => {
        console.log('WebSocket client connected');
        socket.on('toggleRecording', (newIsRecording) => {
            if (newIsRecording !== isRecording) {
                isRecording = newIsRecording;
                console.log(`Recording is now ${isRecording ? 'enabled' : 'disabled'}.`);

                if (!isRecording) {
                    // Stop recording: Close all files
                    Object.keys(csvFiles).forEach(id => {
                        if (csvFiles[id]) {
                            csvFiles[id].end();
                            console.log(`CSV file for device ${id} closed.`);
                            delete csvFiles[id]; // Properly delete the file reference
                        }
                    });
                }
            }
        });

        socket.on('disconnect', () => {
            console.log('WebSocket client disconnected');
        });
    });

    return io;
}

function setupTcpServer(io) {
    const tcpServer = net.createServer(socket => {
        console.log('TCP client connected');
        socket.on('data', buffer => {
            if (buffer.length < 16) {
                console.error('Received data is too short.');
                return;
            }
            const receivedId = buffer.readUInt32LE(0);
            //const receivedAccX = buffer.readFloatLE(4);
            //const receivedAccY = buffer.readFloatLE(8);
            //const receivedAccZ = buffer.readFloatLE(12);
            const normAcc = buffer.readFloatLE(4);
            const m5Time = buffer.readFloatLE(8);
            const lat = buffer.readFloatLE(12);
            const lng = buffer.readFloatLE(16);

            //const data = { time: getTime(), m5time: m5Time,id: receivedId, normacc: normAcc, accX: receivedAccX, accY: receivedAccY, accZ: receivedAccZ, lat: lat, lng: lng };
            const data = { time: getTime(), m5time: m5Time, id: receivedId, normacc: normAcc, lat: lat, lng: lng }

            //io.emit('data', data);  // Send data to WebSocket clients

            if (isRecording) {
                const dirPath = path.join(__dirname, "csv_data", `Device_${receivedId}`);
                if (!csvFiles[receivedId]) {
                    if (!fs.existsSync(dirPath)){
                        fs.mkdirSync(dirPath, { recursive: true });
                    }
                    const timeStr = data["time"];
                    const timeData = new Date(timeStr);
                    const dateStr = timeData.toISOString().split('T')[0];
                    const csvFilename = `${dateStr}_${receivedId}_${Date.now()}.csv`;
                    const filepath = path.join(dirPath, csvFilename);
                    csvFiles[receivedId] = fs.createWriteStream(filepath, { flags: 'a' });
                    csvFiles[receivedId].write("Time,m5time,accNorm,lat,lng\n");
                    console.log(`New CSV file created: ${filepath}`);
                }

                const csvLine = `${data.time},${data.m5time},${data.normacc},${data.lat},${data.lng}\n`;
                csvFiles[receivedId].write(csvLine);
            }
        });

        socket.on('close', () => {
            console.log('TCP client disconnected');
        });

        socket.on('error', error => {
            console.error(`Error from TCP client: ${error.message}`);
        });
    });

    tcpServer.listen(TCP_PORT, HOST, () => {
        console.log(`TCP server listening on port ${TCP_PORT}`);
    });
}

const io = setupWebSocketServer();
setupTcpServer(io);