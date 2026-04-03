'use strict';

const express   = require('express');
const cors      = require('cors');
const mysql     = require('mysql2/promise');
const mongoose  = require('mongoose');

const app  = express();
const PORT = process.env.PORT || 3000;

app.use(cors());
app.use(express.json());

// ─── MySQL connection pool ───────────────────────────────────────────────────
let db;
async function initMySQL() {
  db = await mysql.createPool({
    host:     process.env.MYSQL_HOST     || 'localhost',
    port:     parseInt(process.env.MYSQL_PORT || '3306'),
    user:     process.env.MYSQL_USER     || 'ismuser',
    password: process.env.MYSQL_PASS     || 'ismpass',
    database: process.env.MYSQL_DB       || 'ism_pictures',
    waitForConnections: true,
    connectionLimit:    10,
  });
  console.log('[C05] MySQL connected');
}

// ─── MongoDB + SNMP schema (optional) ───────────────────────────────────────
const snmpSchema = new mongoose.Schema({
  node:      String,
  osName:    String,
  cpuPct:    Number,
  ramMb:     Number,
  timestamp: { type: Date, default: Date.now },
});
const SnmpMetric = mongoose.model('SnmpMetric', snmpSchema);

async function initMongo() {
  try {
    await mongoose.connect(process.env.MONGO_URI || 'mongodb://localhost:27017/ism_snmp');
    console.log('[C05] MongoDB connected');
  } catch (e) {
    console.warn('[C05] MongoDB unavailable (optional):', e.message);
  }
}

// ─── Routes ─────────────────────────────────────────────────────────────────

/**
 * POST /picture
 * Body: raw BMP bytes (application/octet-stream)
 * Headers: X-Filename, X-Operation, X-Mode
 * Returns: picture id (plain text)
 */
app.post('/picture', express.raw({ type: 'application/octet-stream', limit: '100mb' }), async (req, res) => {
  try {
    const filename  = req.headers['x-filename']  || 'unknown.bmp';
    const operation = req.headers['x-operation'] || 'encrypt';
    const mode      = req.headers['x-mode']      || 'CBC';
    const data      = req.body;

    const [result] = await db.execute(
      'INSERT INTO pictures (filename, operation, mode, data) VALUES (?, ?, ?, ?)',
      [filename, operation, mode, data]
    );
    const id = result.insertId;
    console.log(`[C05] Stored picture id=${id} (${data.length} bytes)`);
    res.status(201).send(String(id));
  } catch (e) {
    console.error('[C05] Store error:', e);
    res.status(500).json({ error: e.message });
  }
});

/**
 * GET /picture/:id
 * Returns the BMP file as a download
 */
app.get('/picture/:id', async (req, res) => {
  try {
    const [rows] = await db.execute(
      'SELECT filename, operation, mode, data FROM pictures WHERE id = ?',
      [req.params.id]
    );
    if (!rows.length) return res.status(404).json({ error: 'Not found' });

    const { filename, operation, data } = rows[0];
    const dlName = `${operation}_${filename}`;
    res.setHeader('Content-Type', 'image/bmp');
    res.setHeader('Content-Disposition', `attachment; filename="${dlName}"`);
    res.send(data);
  } catch (e) {
    res.status(500).json({ error: e.message });
  }
});

/**
 * GET /pictures
 * Returns list of all stored pictures (metadata only, no blobs)
 */
app.get('/pictures', async (req, res) => {
  const [rows] = await db.execute(
    'SELECT id, filename, operation, mode, created_at FROM pictures ORDER BY id DESC'
  );
  res.json(rows);
});

/**
 * GET /view/:id  — Web Access: HTML page rendering the picture inline
 */
app.get('/view/:id', async (req, res) => {
  try {
    const [rows] = await db.execute(
      'SELECT id, filename, operation, mode, created_at FROM pictures WHERE id = ?',
      [req.params.id]
    );
    if (!rows.length) return res.status(404).send('<h2>Picture not found</h2>');
    const { id, filename, operation, mode, created_at } = rows[0];
    res.setHeader('Content-Type', 'text/html');
    res.send(`<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8"/>
<title>Picture #${id}</title>
<style>
  body{font-family:system-ui,sans-serif;background:#f4f4f5;display:flex;flex-direction:column;align-items:center;padding:2rem}
  .card{background:#fff;border-radius:12px;padding:1.5rem;box-shadow:0 2px 12px rgba(0,0,0,.08);max-width:900px;width:100%}
  h2{margin-bottom:.5rem}
  p{color:#52525b;font-size:.9rem;margin-bottom:1rem}
  img{max-width:100%;border-radius:8px;border:1px solid #e4e4e7}
  a{display:inline-block;margin-top:1rem;color:#2563eb;text-decoration:underline}
</style></head><body>
<div class="card">
  <h2>Picture #${id} — ${operation.toUpperCase()} / ${mode}</h2>
  <p>File: ${filename} &nbsp;|&nbsp; Stored: ${new Date(created_at).toLocaleString()}</p>
  <img src="/picture/${id}" alt="picture ${id}"/>
  <br/><a href="/picture/${id}" download="${operation}_${filename}">Download</a>
  &nbsp;&nbsp;<a href="/">Back to gallery</a>
</div>
</body></html>`);
  } catch (e) {
    res.status(500).send('<h2>Error: ' + e.message + '</h2>');
  }
});

/**
 * GET /  — Web Access: gallery of all stored pictures
 */
app.get('/', async (req, res) => {
  try {
    const [rows] = await db.execute(
      'SELECT id, filename, operation, mode, created_at FROM pictures ORDER BY id DESC'
    );
    const items = rows.map(r => `
      <div class="card">
        <a href="/view/${r.id}">
          <img src="/picture/${r.id}" alt="pic ${r.id}"/>
        </a>
        <p><strong>#${r.id}</strong> ${r.operation.toUpperCase()} / ${r.mode}<br/>
           <span>${r.filename}</span><br/>
           <span>${new Date(r.created_at).toLocaleString()}</span></p>
        <a href="/view/${r.id}">View</a> &nbsp;
        <a href="/picture/${r.id}" download="${r.operation}_${r.filename}">Download</a>
      </div>`).join('');
    res.setHeader('Content-Type', 'text/html');
    res.send(`<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8"/>
<title>ISM — Picture Gallery</title>
<style>
  body{font-family:system-ui,sans-serif;background:#f4f4f5;padding:2rem}
  h1{margin-bottom:1.5rem}
  .grid{display:flex;flex-wrap:wrap;gap:1rem}
  .card{background:#fff;border-radius:12px;padding:1rem;box-shadow:0 2px 8px rgba(0,0,0,.08);width:260px}
  .card img{width:100%;border-radius:8px;border:1px solid #e4e4e7;display:block}
  .card p{font-size:.8rem;color:#52525b;margin:.5rem 0}
  .card span{color:#71717a}
  a{color:#2563eb;text-decoration:underline;font-size:.85rem}
</style></head><body>
<h1>ISM — Picture Gallery</h1>
<div class="grid">${items || '<p>No pictures stored yet.</p>'}</div>
</body></html>`);
  } catch (e) {
    res.status(500).send('<h2>Error: ' + e.message + '</h2>');
  }
});

// ─── SNMP endpoints (optional) ───────────────────────────────────────────────

/**
 * POST /snmp
 * Body: { node, osName, cpuPct, ramMb }
 */
app.post('/snmp', async (req, res) => {
  try {
    const metric = await SnmpMetric.create(req.body);
    res.status(201).json(metric);
  } catch (e) {
    res.status(500).json({ error: e.message });
  }
});

/**
 * GET /snmp
 * Returns last 100 SNMP readings
 */
app.get('/snmp', async (req, res) => {
  try {
    const metrics = await SnmpMetric.find().sort({ timestamp: -1 }).limit(100);
    res.json(metrics);
  } catch (e) {
    res.status(500).json({ error: e.message });
  }
});

/** GET /health — used by docker-compose depends_on checks */
app.get('/health', (_, res) => res.json({ status: 'ok' }));

// ─── Boot ────────────────────────────────────────────────────────────────────
async function boot() {
  await initMySQL();
  await initMongo();
  app.listen(PORT, () => console.log(`[C05] Listening on :${PORT}`));
}

boot().catch(e => { console.error(e); process.exit(1); });
