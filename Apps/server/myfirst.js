const fs = require('fs');
const path = require('path');
const net = require('net');
const express = require('express');
const http = require('http');
const socketIo = require('socket.io');

const WEB_SOCKET_PORT = 3001;
const TCP_PORT = 3002;
const HOST = '192.168.1.4'; //IPアドレス

let isRecording = false;
let csvFiles = {};

function formatDate(date, includeTime = false) {
    let formattedDate = date.getFullYear()  //現在時刻
        + '-' + ('0' + (date.getMonth() + 1)).slice(-2) 
        + '-' + ('0' + date.getDate()).slice(-2);
    
    if (includeTime) {
        formattedDate += ' ' + ('0' + date.getHours()).slice(-2) 
            + ':' + ('0' + date.getMinutes()).slice(-2) 
            + ':' + ('0' + date.getSeconds()).slice(-2) 
            + '.' + ('00' + date.getMilliseconds()).slice(-3);
    }
    
    return formattedDate;
}

function getTime() {
    return formatDate(new Date(), true);
}

function getCSVTime() {
    return formatDate(new Date(), false);
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
        console.log('TCP client connected',getTime());
        socket.on('data', buffer => {
            if (buffer.length < 24) {
                console.error('Received data is too short.');
                return;
            }
            const receivedId = buffer.readUInt32LE(0);
            const receivedAccX = buffer.readFloatLE(4);
            const receivedAccY = buffer.readFloatLE(8);
            const receivedAccZ = buffer.readFloatLE(12);
            const normAcc = buffer.readFloatLE(16);
            const m5Time = buffer.readFloatLE(20);

            const data = { time: getTime(), m5time: m5Time,id: receivedId, normacc: normAcc, accX: receivedAccX, accY: receivedAccY, accZ: receivedAccZ };
            
            //console.log(data);
            if (receivedId < 101) {
                io.emit('data', data);  // Send data to WebSocket clients

                if (isRecording) {
                    const dirPath = path.join(__dirname, ".." ,"csv_data", `${getCSVTime()}` , `Device_${receivedId}`);
                    if (!csvFiles[receivedId]) {
                        if (!fs.existsSync(dirPath)){
                            fs.mkdirSync(dirPath, { recursive: true });
                        }
                        var date = new Date();
                        const timeStr = data["time"];
                        const timeData = new Date(timeStr);
                        const dateStr = date.getFullYear() + '-' + ('0' + (date.getMonth() + 1)).slice(-2) + '-' +('0' + date.getDate()).slice(-2);
                        const csvFilename = `${dateStr}_${receivedId}_${Date.now()}.csv`;
                        const filepath = path.join(dirPath, csvFilename);
                        csvFiles[receivedId] = fs.createWriteStream(filepath, { flags: 'a' });
                        csvFiles[receivedId].write("Time,m5time,accNorm,accX,accY,accZ\n");
                        console.log(`New CSV file created: ${filepath}`);
                    }

                    const csvLine = `${data.time},${data.m5time},${data.normacc},${data.accX},${data.accY},${data.accZ}\n`;
                    csvFiles[receivedId].write(csvLine);
                }
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
