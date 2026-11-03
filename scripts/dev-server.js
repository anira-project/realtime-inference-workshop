#!/usr/bin/env node

const fs = require('fs').promises;
const path = require('path');
const http = require('http');
const { build } = require('./build.js');
const WebSocket = require('ws');

let server = null;
let wss = null;

const SLIDES_DIR = path.join(__dirname, '../slides');

// The notes of one slide in a chapter file, as Reveal splits it: slides at lines
// of ---, the notes after the first "Note:". The editable part ends where a
// commented-out slide (<!-- at the start of a line) follows the notes.
// Returns null for an index the file does not have.
function locateNotes(content, index) {
  const parts = content.split(/(^---[ \t]*$)/m);  // slides at even, separators at odd positions
  if (index < 0 || 2 * index >= parts.length) return null;
  const slide = parts[2 * index];

  const marker = /^[ \t]*notes?:[ \t]*$/im.exec(slide);
  let start, end;
  if (marker) {
    start = marker.index + marker[0].length;
    const comment = /^<!--/m.exec(slide.slice(start));
    end = comment ? start + comment.index : slide.length;
  } else {
    start = end = slide.replace(/\s*$/, '').length;
  }
  return { parts, slide, marker, start, end };
}

// The lines without the indentation they all share, trimmed
function dedent(text) {
  const lines = text.replace(/^\s*\n/, '').replace(/\s*$/, '').split('\n');
  const indent = Math.min(...lines.filter(line => line.trim()).map(line => /^[ \t]*/.exec(line)[0].length));
  return lines.map(line => line.slice(Number.isFinite(indent) ? indent : 0)).join('\n').trim();
}

// GET /api/notes?file=00-introduction.md&index=2 -> { notes }
// POST /api/notes { file, index, notes }          -> writes the file; the watcher rebuilds
async function handleNotes(req, res, url) {
  const reply = (status, body) => {
    res.writeHead(status, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify(body));
  };
  try {
    let file, index, notes;
    if (req.method === 'POST') {
      let body = '';
      for await (const chunk of req) body += chunk;
      ({ file, index, notes } = JSON.parse(body));
    } else {
      file = url.searchParams.get('file');
      index = Number(url.searchParams.get('index'));
    }
    // A chapter file of slides/, nothing else
    if (typeof file !== 'string' || !/^[\w.-]+\.md$/.test(file) || !Number.isInteger(index)) {
      return reply(400, { error: 'file and index required' });
    }
    const filePath = path.join(SLIDES_DIR, file);
    const content = await fs.readFile(filePath, 'utf-8');
    const found = locateNotes(content, index);
    if (!found) return reply(404, { error: `${file} has no slide ${index}` });
    const { parts, slide, marker, start, end } = found;

    // The notes as the editor shows them: without the indentation some notes carry
    const current = marker ? dedent(slide.slice(start, end)) : '';
    if (req.method !== 'POST') {
      return reply(200, { notes: current });
    }

    if (typeof notes !== 'string') return reply(400, { error: 'notes required' });
    const text = notes.replace(/\r\n/g, '\n').trim();
    // Saved unchanged: the file keeps its own formatting
    if (text === current) return reply(200, { saved: false });
    // Keep the whitespace after the notes and whatever follows them (the blank
    // lines before the next slide, a commented-out slide)
    const whitespace = /\s*$/.exec(slide.slice(start, end))[0] || '\n';
    parts[2 * index] = marker
      ? slide.slice(0, start) + '\n' + text + whitespace + slide.slice(end)
      : slide.slice(0, start) + '\n\nNote:\n' + text + slide.slice(end);

    // Written as a whole and renamed, so the watcher never builds a half-written file
    const temporary = `${filePath}.notes-editor`;
    await fs.writeFile(temporary, parts.join(''));
    await fs.rename(temporary, filePath);
    console.log(`📝 Notes saved: ${file}, slide ${index + 1}`);
    return reply(200, { saved: true });
  } catch (error) {
    return reply(500, { error: String(error) });
  }
}

async function startDevServer() {
  console.log('🛑 Starting development server...');
  
  await build(true);
  
  // Create HTTP server
  server = http.createServer(async (req, res) => {
    // Handle CORS preflight requests
    if (req.method === 'OPTIONS') {
      res.setHeader('Access-Control-Allow-Origin', '*');
      res.setHeader('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS');
      res.setHeader('Access-Control-Allow-Headers', 'Content-Type, Authorization');
      res.writeHead(200);
      res.end();
      return;
    }
    
    // Parse URL to handle query parameters (needed for speaker notes)
    const url = new URL(req.url, `http://${req.headers.host}`);
    let requestPath = url.pathname;

    if (requestPath === '/api/notes') {
      await handleNotes(req, res, url);
      return;
    }

    let filePath = path.join(__dirname, '../dist', requestPath === '/' ? 'index.html' : requestPath);
    
    try {
      const stats = await fs.stat(filePath);
      if (stats.isDirectory()) {
        filePath = path.join(filePath, 'index.html');
      }
      
      const data = await fs.readFile(filePath);
      const ext = path.extname(filePath);
      
      let contentType = 'text/html';
      if (ext === '.js') contentType = 'application/javascript';
      else if (ext === '.css') contentType = 'text/css';
      else if (ext === '.json') contentType = 'application/json';
      else if (ext === '.png') contentType = 'image/png';
      else if (ext === '.jpg' || ext === '.jpeg') contentType = 'image/jpeg';
      else if (ext === '.gif') contentType = 'image/gif';
      else if (ext === '.svg') contentType = 'image/svg+xml';
      
      res.setHeader('Content-Type', contentType);
      res.setHeader('Cache-Control', 'no-cache');
      
      // Add CORS headers for speaker notes to work
      res.setHeader('Access-Control-Allow-Origin', '*');
      res.setHeader('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS');
      res.setHeader('Access-Control-Allow-Headers', 'Content-Type, Authorization');
      
      res.writeHead(200);
      res.end(data);
    } catch (err) {
      res.writeHead(404);
      res.end('Not found');
    }
  });
  
  // Create WebSocket server attached to HTTP server
  wss = new WebSocket.Server({ server });
  
  const PORT = 8080;
  server.listen(PORT, () => {
    console.log(`✅ Development server started at http://localhost:${PORT}`);
    console.log('🔌 Hot reload WebSocket server running on same port');
    console.log('⚡ Watching for changes...');
  });
}

async function watchFiles() {
  const watchDirs = [
    path.join(__dirname, '../slides')
  ];
  
  // Editors that save by writing a temporary file and renaming it report
  // 'rename', not 'change', and one save can fire several events: rebuild on
  // either, once the events have settled.
  let pending = null;
  const rebuild = async () => {
    console.log('🔄 Rebuilding...');
    await build(true);
    console.log('✅ Rebuild completed!');

    // Notify all connected browsers to reload
    if (wss) {
      wss.clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
          client.send('reload');
        }
      });
    }
  };

  for (const dir of watchDirs) {
    try {
      const watcher = require('fs').watch(dir, { recursive: true }, (eventType, filename) => {
        console.log(`📝 File changed: ${filename}`);
        clearTimeout(pending);
        pending = setTimeout(rebuild, 150);
      });
    } catch (error) {
      console.log(`⚠️  Could not watch directory: ${dir}`);
    }
  }
}

async function startDevelopment() {
  try {
    await startDevServer();
    await watchFiles();
  } catch (error) {
    console.error('❌ Development server failed:', error.message);
    process.exit(1);
  }
}

process.on('SIGINT', () => {
  console.log('\n🛑 Shutting down development server...');
  if (server) {
    server.close();
  }
  if (wss) {
    wss.close();
  }
  process.exit(0);
});

if (require.main === module) {
  startDevelopment();
}