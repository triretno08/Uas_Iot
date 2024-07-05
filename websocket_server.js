const http = require('http');
const express = require('express');
const WebSocket = require('ws');
const mqtt = require('mqtt');
const mysql = require('mysql');
const path = require('path');

// Express setup
const app = express();
const port = 8777;

app.use(express.static(path.join(__dirname, 'public'))); // Serve static files from 'public' folder

// HTTP server
const server = http.createServer(app);

// WebSocket server
const wss = new WebSocket.Server({ server });

// Connect to the MQTT broker
const mqttClient = mqtt.connect('mqtt://usm.revolusi-it.com', {
  username: 'usm',
  password: 'usmjaya24'
});

// Create a connection to the MySQL database
const dbConnection = mysql.createConnection({
  host: 'localhost',
  user: 'root', // Ganti dengan username MySQL Anda
  password: '', // Ganti dengan password MySQL Anda
  database: 'sensor_data',
  port:'3306'
});

// Connect to the database
dbConnection.connect((err) => {
  if (err) {
    console.error('Error connecting to MySQL:', err.stack);
    return;
  }
  console.log('Connected to MySQL as id', dbConnection.threadId);
});

// Subscribe to the MQTT topic
mqttClient.on('connect', () => {
  mqttClient.subscribe('test/test', (err) => {
    if (!err) {
      console.log('Subscribed to test/test');
    } else {
      console.error('Failed to subscribe:', err);
    }
  });
});

// Handle incoming MQTT messages
mqttClient.on('message', (topic, message) => {
  if (topic === 'test/test') {
    let data;
    try {
      data = JSON.parse(message.toString());
    } catch (e) {
      console.error('Failed to parse MQTT message:', e);
      return;
    }

    const { nim, suhu, kelembapan } = data;

    // Only save data to MySQL if NIM is G.231.21.0199 and suhu/kelembapan are valid numbers
    if (nim === 'G.231.21.0182' && !isNaN(suhu) && !isNaN(kelembapan)) {
      const query = 'INSERT INTO sensor_readings (nim, suhu, kelembapan) VALUES (?, ?, ?)';
      dbConnection.query(query, [nim, suhu, kelembapan], (err, results) => {
        if (err) {
          console.error('Failed to insert data into MySQL:', err.stack);
          return;
        }
        console.log('Data inserted into MySQL:', results.insertId);
      });
    } else {
      console.log('Invalid data or NIM not allowed to insert into database:', nim, suhu, kelembapan);
    }

    // Forward the data to all connected WebSocket clients
    wss.clients.forEach((client) => {
      if (client.readyState === WebSocket.OPEN) {
        client.send(JSON.stringify(data));
      }
    });
  }
});

// Handle WebSocket server events
wss.on('connection', (ws) => {
  console.log('New WebSocket connection established');
  
  ws.on('message', (message) => {
    const data = JSON.parse(message);
    
    if (data.type === 'control') {
      const controlMessage = `D${data.pin}=${data.state}`;
      
      // Publish the control message to MQTT
      mqttClient.publish('test/test', JSON.stringify({
        nim: 'G.231.21.0182',
        messages: controlMessage
      }));
      
      console.log(`Published control message to MQTT: ${controlMessage}`);
    }
  });
  
  ws.on('close', () => {
    console.log('WebSocket connection closed');
  });
});

// Serve the HTML file for the main page from the 'public' folder
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

// Start the server
server.listen(port, () => {
  console.log(`HTTP server and WebSocket server are running on http://localhost:${port}`);
});