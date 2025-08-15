const express = require('express')
const { SerialPort, ReadlineParser } = require("serialport");
const Readline = require("@serialport/parser-readline");
var bodyParser = require('body-parser');
const app = express();
const port = 3000;

// Arduino port detection
const ARDUINO_VIDS = new Set(['2341', '2A03']); // Arduino LLC / Genuino

async function findMkrPort() {
  // IDs for "Arduino MKR WAN 1310"
  const VID = '2341';
  const PID = '8059';
  const PNP_RE = /VID_2341.*PID_8059/i;
  const NAME_HINT = /arduino mkr wan 1310/i;

  const ports = await SerialPort.list();

  const match = ports.find(p => {
    const vid = String(p.vendorId || '').toUpperCase();
    const pid = String(p.productId || '').toUpperCase();
    const pnp = p.pnpId || '';
    const name = [p.friendlyName, p.manufacturer].filter(Boolean).join(' ');

    return (vid === VID && pid === PID)      // direct VID/PID match
        || PNP_RE.test(pnp)                  // Windows PnP ID match
        || NAME_HINT.test(name);             // friendly name match
  });

  return match?.path || null;
}
// ---------------------------------------

// Define serial port (now opened after detection)
let sPort;

// Read serial port data
let serialData = "";

app.use('/', express.static('public/html'));

app.use(bodyParser.urlencoded({ extended: true }))
app.use(bodyParser.json())

// Handle client-sent events
app.post('/client-sent-events', function(req, res) {
    const clientData = req.body.data;

    if (clientData) {
        console.log(`Received data from client: ${clientData}`);
        // Write data to serial port
        sPort.write(clientData, (err) => {
            if (err) {
                return res.status(500).send({ message: 'Failed to write to serial port' });
            }
            res.send({ message: 'Data sent to serial port successfully' });
        });
    } else {
        res.status(400).send({ message: 'No data received' });
    }
});

app.get('/server-sent-events', function(req, res) {

    res.writeHead(200, {
        'Content-Type': 'text/event-stream',
        'Cache-Control': 'no-cache',
        'Connection': 'keep-alive'
    });

    var interval = setInterval(function(){
        res.write("data: " + serialData + "\n\n");
    }, 1500);

    // close
    res.on('close', () => {
        clearInterval(interval);
        res.end();
    });
});

// open serial, wire parser, then start server
(async () => {
  try {
    const explicit =
      (process.argv.find(a => a.startsWith('--port=')) || '').split('=')[1] ||
      process.env.SERIAL_PORT;

    const path = explicit || await findMkrPort();
    if (!path) {
      console.error('No Arduino MKR port found. Pass --port=COM7 or set SERIAL_PORT=COM7.');
      process.exit(1);
    }

    sPort = new SerialPort({ path, baudRate: 115200 }, (err) => {
      if (err) {
        console.error(`Failed to open ${path}:`, err.message);
        process.exit(1);
      } else {
        console.log(`Opened ${path} @ 115200 baud`);
      }
    });

    const parser = new ReadlineParser();
    sPort.pipe(parser);

    parser.on('data', (data) => {
        console.log(data);
        serialData = data;
    });

    app.listen(port, () => {
      console.log(`Listening at http://localhost:${port}`)
    })
  } catch (e) {
    console.error('Error during serial setup:', e);
    process.exit(1);
  }
})();