# M5StickServer
- M5StickCPlusとNode.js、Reactを使って加速度を取得する。

### 初期設定
M5StickCPlus2を使う時にWiFi設定、自分のPCのIPアドレスを調べてコードに入れておく。device_idは1から100の間で設定して、複数台使う場合はかぶりがないようにする。
``` C++
const char* ssid = "";
const char* password = "";

const char* server_ip = "";
const int server_port = 3002;
const int device_id = 1;
```

Node.js、React側でもIPアドレスを入力する。
- Node.js
``` js
const fs = require('fs');
const path = require('path');
const net = require('net');
const express = require('express');
const http = require('http');
const socketIo = require('socket.io');

const WEB_SOCKET_PORT = 3001;
const TCP_PORT = 3002;
const HOST = '192.168.1.18'; //IPアドレス

let isRecording = false;
let csvFiles = {};
```

- React
``` js
import React, { useEffect, useState } from 'react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend } from 'recharts';
import io from 'socket.io-client';
import './App.css';

const socket = io('http://192.168.1.18:3001'); //IPアドレスとlocalhost

const RealTimeChart = ({ data, color }) => (
  <LineChart width={600} height={300} data={data} margin={{ top: 5, right: 30, left: 20, bottom: 5 }}>
    <CartesianGrid strokeDasharray="3 3" />
    <XAxis dataKey="time" />
    <YAxis />
    <Tooltip />
    <Legend />
    <Line type="monotone" dataKey="accX" stroke={color} name="accX" activeDot={{ r: 8 }} />
    <Line type="monotone" dataKey="accY" stroke="#ffc658" name="accY" activeDot={{ r: 8 }} />
    <Line type="monotone" dataKey="accZ" stroke="#82ca9d" name="accZ" activeDot={{ r: 8 }} />
  </LineChart>
);
```

### 起動
- コンソールにAppsフォルダのディレクトリでnpm startを入力すると起動する。
```
npm start
```

- 起動すると以下の画面が出てくる
![alt text](<images/Screenshot 2024-08-10 220850.png>)

- 画面左上のstart recordingを押すと加速度、加速度ノルムがcsvに記録される。
  押した後はstop recordingになるのでそれを押すと記録が停止するので任意の任意のタイミングで押す。
- M5Stickの機能にAボタン（大きいM5と書かれたボタン）を押すと画面を消してバッテリー消費を抑えるようにしている。
