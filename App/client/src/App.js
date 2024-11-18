import React, { useEffect, useState } from 'react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend } from 'recharts';
import io from 'socket.io-client';
import './App.css';

const socket = io('http://192.168.1.4:3001');

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

const DeviceChart = ({ deviceId, deviceData }) => (
  <div className="chart-container">
    <h2>Device {deviceId}</h2>
    <RealTimeChart data={deviceData} color="#8884d8" />
  </div>
);

const MultiDeviceChart = () => {
  const [isRecording, setIsRecording] = useState(JSON.parse(localStorage.getItem('isRecording')) || false);
  const [devicesData, setDevicesData] = useState({});

  useEffect(() => {
    socket.on('connect', () => console.log("Connected to WebSocket server"));
    socket.on('connect_error', (error) => console.error("Connection Error:", error));
    socket.on('data', (newData) => {
      console.log("Received data:", newData);
      const { id, time, accX, accY, accZ } = newData;
      const formattedData = { time: new Date(time).toLocaleTimeString(), accX, accY, accZ };
      setDevicesData(prev => ({
        ...prev,
        [id]: [...(prev[id] || []).slice(-100), formattedData],
      }));
    });

    return () => {
      socket.off('connect');
      socket.off('connect_error');
      socket.off('data');
    };
  }, []);

  const toggleRecording = () => {
    const newIsRecording = !isRecording;
    setIsRecording(newIsRecording);
    localStorage.setItem('isRecording', JSON.stringify(newIsRecording));
    socket.emit('toggleRecording', newIsRecording);
    console.log(`Recording ${newIsRecording ? 'started' : 'stopped'}.`);
  };

  return (
    <div>
      <button onClick={toggleRecording}>
        {isRecording ? 'Stop Recording' : 'Start Recording'}
      </button>
      <div className="chart-grid">
        {Object.entries(devicesData).map(([deviceId, deviceData]) => (
          <DeviceChart key={deviceId} deviceId={deviceId} deviceData={deviceData} />
        ))}
      </div>
    </div>
  );
};

function App() {
  return <MultiDeviceChart />;
}

export default App;
