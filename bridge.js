#!/usr/bin/env node
// AirPiano bridge: Wokwi serial output  ->  browser (Server-Sent Events)
// No dependencies. Needs Node 14+.
//
//   node bridge.js                      read Wokwi for VS Code over rfc2217://localhost:4000
//   node bridge.js --rfc2217 host:port  same, custom address
//   wokwi-cli . | node bridge.js --stdin
//   node bridge.js --port 8787          change the web port
//
// Then open http://localhost:8787

const http = require('http');
const net = require('net');
const fs = require('fs');
const path = require('path');

const args = process.argv.slice(2);
const opt = (name, def) => { const i = args.indexOf(name); return i >= 0 && args[i + 1] ? args[i + 1] : def; };
const PORT = +opt('--port', 8787);
const USE_STDIN = args.includes('--stdin');
const [RHOST, RPORT] = opt('--rfc2217', 'localhost:4000').split(':');

const clients = new Set();
let lastStatus = null;

function send(res, event, data) {
  res.write((event ? `event: ${event}\n` : '') + `data: ${data}\n\n`);
}
function broadcast(event, data) { for (const res of clients) send(res, event, data); }
function status(text) { lastStatus = text; console.log('[bridge]', text.replace(/^!/, '')); broadcast('status', text); }

// ---- turn a byte stream into lines ----
let buf = '';
function feed(text) {
  buf += text;
  let i;
  while ((i = buf.indexOf('\n')) >= 0) {
    const line = buf.slice(0, i).replace(/\r$/, '');
    buf = buf.slice(i + 1);
    if (line.length) broadcast(null, line);
  }
}

// ---- HTTP server: page + event stream ----
const server = http.createServer((req, res) => {
  const url = req.url.split('?')[0];
  if (url === '/events') {
    res.writeHead(200, {
      'Content-Type': 'text/event-stream', 'Cache-Control': 'no-cache', 'Connection': 'keep-alive',
      'Access-Control-Allow-Origin': '*',
    });
    res.write('retry: 1500\n\n');
    clients.add(res);
    if (lastStatus) send(res, 'status', lastStatus);
    req.on('close', () => clients.delete(res));
  } else if (url === '/ping') {
    res.writeHead(200, { 'Access-Control-Allow-Origin': '*' }); res.end('ok');
  } else if (url === '/' || url === '/index.html') {
    fs.readFile(path.join(__dirname, 'index.html'), (err, data) => {
      if (err) { res.writeHead(500); return res.end('index.html not found next to bridge.js'); }
      res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' }); res.end(data);
    });
  } else { res.writeHead(404); res.end(); }
});
setInterval(() => { for (const res of clients) res.write(': keep-alive\n\n'); }, 15000);
server.listen(PORT, () => console.log(`AirPiano bridge ready → http://localhost:${PORT}`));

// ---- source 1: stdin (e.g. wokwi-cli) ----
if (USE_STDIN) {
  status('Reading Wokwi CLI output');
  process.stdin.setEncoding('utf8');
  process.stdin.on('data', d => { process.stdout.write(d); feed(d); });
  process.stdin.on('end', () => status('!Wokwi CLI output ended'));
}

// ---- source 2: Wokwi for VS Code RFC2217 serial server (plain TCP + telnet bytes) ----
else {
  let st = 0, cmd = 0;               // tiny telnet filter state machine
  const IAC = 255;
  function telnet(sock, data) {
    const out = [];
    for (const b of data) {
      switch (st) {
        case 0: if (b === IAC) st = 1; else out.push(b); break;
        case 1:
          if (b === IAC) { out.push(IAC); st = 0; }
          else if (b >= 251 && b <= 254) { cmd = b; st = 2; }
          else if (b === 250) st = 3;
          else st = 0;
          break;
        case 2:                       // politely refuse every option: WILL→DONT, DO→WONT
          if (cmd === 251) sock.write(Buffer.from([IAC, 254, b]));
          else if (cmd === 253) sock.write(Buffer.from([IAC, 252, b]));
          st = 0; break;
        case 3: if (b === IAC) st = 4; break;
        case 4: st = b === 240 ? 0 : 3; break;
      }
    }
    return Buffer.from(out).toString('utf8');
  }
  function connect() {
    status(`Waiting for Wokwi on ${RHOST}:${RPORT}…`);
    const sock = net.connect(+RPORT, RHOST);
    sock.on('connect', () => { st = 0; status('Wokwi connected'); });
    sock.on('data', d => { const t = telnet(sock, d); process.stdout.write(t); feed(t); });
    sock.on('error', () => {});
    sock.on('close', () => { status(`!Wokwi not running — start the simulation (port ${RPORT})`); setTimeout(connect, 2000); });
  }
  connect();
}
