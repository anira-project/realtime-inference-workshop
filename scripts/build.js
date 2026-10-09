#!/usr/bin/env node

const fs = require('fs').promises;
const path = require('path');
const QRCode = require('qrcode');

async function combineSlides() {
  const slidesDir = path.join(__dirname, '../slides');
  const slideFiles = await fs.readdir(slidesDir);
  
  const sortedFiles = slideFiles
    .filter(file => file.endsWith('.md') || (file.endsWith('.html') && file !== 'index.html'))
    .sort();
  
  let slides = [];
  
  for (const file of sortedFiles) {
    const filePath = path.join(slidesDir, file);
    const content = await fs.readFile(filePath, 'utf-8');
    
    slides.push({
      filename: file,
      content: content,
      isHtml: file.endsWith('.html')
    });
  }
  
  return slides;
}

async function generateHTML(isDev = false) {
  const slidesTemplatePath = path.join(__dirname, '../slides/templates/slides.html');
  const slidesTemplate = await fs.readFile(slidesTemplatePath, 'utf-8');
  
  const slides = await combineSlides();
  const distDir = path.join(__dirname, '../dist');
  await fs.mkdir(distDir, { recursive: true });
  
  // Every slide file is one chapter: a vertical stack in a single deck, so the
  // overview (Esc) shows one column per chapter
  const chapters = [];
  let part = null;
  for (const slide of slides) {
    const partMatch = slide.content.match(/<!--\s*part:\s*(.+?)\s*-->/);
    if (partMatch) {
      part = partMatch[1];
    }
    // exercise, demo or talk, or several ("demo, exercise") — shown as badges in the agenda
    const kindMatch = slide.content.match(/<!--\s*kind:\s*([\w,\s]+?)\s*-->/);
    chapters.push({
      ...slide,
      id: slide.filename.replace(/\.(md|html)$/, '').replace(/^\d+-/, ''),
      title: extractTitle(slide.content, slide.isHtml),
      part: part,
      kinds: kindMatch ? kindMatch[1].split(',').map(k => k.trim()) : []
    });
  }
  
  const agenda = buildAgenda(chapters);
  const integrationTemplate = await fs.readFile(
    path.join(__dirname, '../slides/templates/integration.html'), 'utf-8');
  let slidesContent = '';
  for (const chapter of chapters) {
    let content = chapter.content.replace(/\{\{AGENDA\}\}/g, agenda);
    content = renderIntegration(content, integrationTemplate);
    content = renderStreams(renderIcons(await renderWaveforms(content)));
    content = await renderQrCodes(content);
    slidesContent += `<section id="${chapter.id}">
      ${await buildSlidesContent([{ ...chapter, content }])}</section>
      `;
  }
  
  const html = renderTemplate(slidesTemplate, {
    SLIDES_CONTENT: slidesContent,
    PRESENTATION_TITLE: 'Real-Time Neural Inference Workshop',
    HOT_RELOAD_SCRIPT: isDev ? '<script src="js/hot_reload.js"></script>\n  <script src="js/overflow_check.js"></script>' : ''
  });
  
  await fs.writeFile(path.join(distDir, 'workshop.html'), html);
  console.log('✅ Generated workshop.html');
  
  // Generate index.html landing page
  await generateIndexPage(chapters, distDir, isDev);
  
  console.log('✅ All HTML files generated successfully!');
}

// {{INTEGRATION:train|export|implement}} → the train/export/implement diagram,
// with a box around that one step
function renderIntegration(content, template) {
  return content.replace(/\{\{INTEGRATION:(\w+)\}\}/g, (match, step) =>
    template.replace(/\{\{(TRAIN|EXPORT|IMPLEMENT)\}\}/g, (m, name) =>
      name.toLowerCase() === step ? 'highlight-step' : ''));
}

// {{WAVEFORM:steps/common/assets/test_signal.h|blocks=2048|height=200|from=2|to=4}} → an SVG of the
// float array in that header as one line, a divider every `blocks` samples
async function renderWaveforms(content) {
  const matches = [...content.matchAll(/\{\{WAVEFORM:([^}|]+)((?:\|[^}|]+)*)\}\}/g)];
  for (const match of matches) {
    const options = Object.fromEntries(match[2].split('|').filter(Boolean).map(o => o.split('=')));
    const header = await fs.readFile(path.join(__dirname, '..', match[1].trim()), 'utf-8');
    const body = header.slice(header.indexOf('= {') + 3, header.lastIndexOf('};'));
    let samples = (body.match(/-?\d[\d.]*(?:e[-+]?\d+)?/gi) || []).map(Number);
    // |from=2|to=4 → only blocks 2 and 3; fewer periods on the slide read as a smooth curve
    if (options.blocks && (options.from || options.to)) {
      const block = Number(options.blocks);
      samples = samples.slice(Number(options.from || 0) * block, (options.to ? Number(options.to) : samples.length / block) * block);
    }

    const width = 1600;
    const height = Number(options.height || 200);
    // One continuous line through the samples (about two points per unit of width),
    // so a sine reads as a smooth curve rather than a comb of min/max bars
    const step = Math.max(1, Math.floor(samples.length / (2 * width)));
    // Full scale is ±1, so signals drawn side by side are comparable
    const y = v => ((1 - Math.max(-1, Math.min(1, v))) / 2 * height).toFixed(1);
    const points = [];
    for (let i = 0; i < samples.length; i += step) {
      points.push(`${(i / (samples.length - 1) * width).toFixed(1)},${y(samples[i])}`);
    }
    const columns = ['M' + points.join('L')];
    let dividers = '';
    if (options.blocks) {
      for (let i = Number(options.blocks); i < samples.length; i += Number(options.blocks)) {
        const x = (i / samples.length * width).toFixed(1);
        dividers += `<line x1="${x}" y1="0" x2="${x}" y2="${height}"/>`;
      }
    }
    const svg = `<svg class="waveform" viewBox="0 0 ${width} ${height}" preserveAspectRatio="none">` +
      `<g class="waveform-dividers">${dividers}</g><path d="${columns.join('')}"/></svg>`;
    content = content.replace(match[0], svg);
  }
  return content;
}

// {{STREAM:host=64|model=2048|length=4096}} → host callbacks above, model blocks below, on one
// sample axis. A playhead lights the callbacks one by one; the callback that completes a
// model block is drawn dark, and that block flashes: that is where the model runs.
// model=0 leaves the model row out.
function renderStreams(content) {
  return content.replace(/\{\{STREAM:([^}]+)\}\}/g, (match, args) => {
    const o = Object.fromEntries(args.split('|').map(a => a.split('=')));
    const host = Number(o.host);
    const model = Number(o.model || 0);
    // Whole callbacks only, so the playhead steps land on callback boundaries
    const count = Math.ceil(Number(o.length || 4096) / host);
    const length = count * host;
    const cycle = Number(o.seconds || 6);
    const cells = lit => Array.from({ length: count }, (_, i) => {
      const start = i * host;
      const end = Math.min(start + host, length);
      // A model block is complete inside this callback
      const fires = model && Math.floor(end / model) > Math.floor(start / model);
      return `<i class="${lit && fires ? 'fires' : ''}" style="left:${(start / length * 100).toFixed(3)}%;` +
        `width:${((end - start) / length * 100).toFixed(3)}%"></i>`;
    }).join('');
    let blocks = '';
    for (let b = 0; model && b < Math.floor(length / model); b++) {
      const done = (b + 1) * model;
      // When the playhead reaches the end of the callback that completes this block
      const cell = Math.ceil(done / host);
      blocks += `<b style="left:${(b * model / length * 100).toFixed(3)}%;width:${(model / length * 100).toFixed(3)}%;` +
        `animation-delay:${(cycle * 0.85 * Math.min(cell, count) / count).toFixed(3)}s"><span>${model}</span></b>`;
    }
    return `<div class="stream" style="--cells:${count};--cycle:${cycle}s">` +
      `<div class="stream-label">host<br><strong>${host}</strong></div>` +
      `<div class="stream-host"><div class="stream-cells">${cells(false)}</div>` +
      `<div class="stream-cells stream-lit">${cells(true)}</div></div>` +
      (model ? `<div class="stream-label">model</div><div class="stream-model">${blocks}</div>` : '') +
      `</div>`;
  });
}

// Colour of the badge on a file icon, by extension
const FILE_COLOURS = { pt: '#ee4c2c', onnx: '#7f7f7f', h: '#8e6bd8', cpp: '#00599c' };
// The logo of what reads the file, on the page: PyTorch for .pt, ONNX, C++ for headers and sources
// (path data from Simple Icons, CC0)
const FILE_LOGOS = {
  pt: { colour: '#ee4c2c', path: 'M12.005 0L4.952 7.053a9.865 9.865 0 000 14.022 9.866 9.866 0 0014.022 0c3.984-3.9 3.986-10.205.085-14.023l-1.744 1.743c2.904 2.905 2.904 7.634 0 10.538s-7.634 2.904-10.538 0-2.904-7.634 0-10.538l4.647-4.646.582-.665zm3.568 3.899a1.327 1.327 0 00-1.327 1.327 1.327 1.327 0 001.327 1.328A1.327 1.327 0 0016.9 5.226 1.327 1.327 0 0015.573 3.9z' },
  onnx: { colour: '#5a5a5a', path: 'M23.0325 11.2963c-.0503 0-.1006 0-.1508.0126l-4.021-7.4387c.0754-.1383.1131-.289.1131-.4524 0-.5403-.4398-.9675-.9675-.9675-.2765 0-.5278.113-.7037.3015L9.286 1.156C9.2357.6785 8.821.3016 8.3184.3016c-.5277 0-.9675.4398-.9675.9675 0 .1634.0377.3141.113.4524l-6.245 8.9591c-.0753-.0251-.1633-.0377-.2513-.0377-.5403 0-.9675.4398-.9675.9676 0 .5403.4398.9675.9675.9675h.0377l3.3676 8.3309c-.0503.1257-.088.2639-.088.402 0 .5404.4398.9676.9676.9676.2764 0 .5277-.113.7036-.3015l10.1152.9926c.1005.4273.49.7288.9424.7288.5403 0 .9676-.4398.9676-.9675 0-.2388-.088-.465-.2262-.6283l5.1141-8.8712c.0503.0126.1005.0126.1634.0126.5403 0 .9675-.4398.9675-.9676 0-.5403-.4272-.98-.9675-.98zM17.2272 4.021c.1131.1508.2765.264.4524.3267l-1.533 11.5728c-.1005.0252-.1885.0503-.2764.1005L7.4513 8.708c.0251-.0754.0377-.1634.0377-.2514 0-.0628-.0126-.1256-.0126-.1884zm4.8754 8.5068l-5.177 3.556a1.105 1.105 0 0 0-.1256-.0753L18.3455 4.335h.0126l3.9456 7.288c-.1508.1759-.2388.3895-.2388.6408 0 .1005.0126.1885.0377.2638zM6.3832 7.5016c-.4649.0754-.8293.4775-.8293.955v.0628l-3.4555 2.0481 5.378-7.7026zm.3519 1.91c.1256-.0252.2513-.088.3518-.1634l8.356 7.2628c-.0377.113-.0628.2262-.0628.3518v.0503l-9.311 3.845c-.1382-.201-.3518-.3518-.6031-.402zm8.8963 8.1172c.1257.1382.3016.2513.5026.289l.465 4.046c-.201.1006-.3519.264-.4524.4524l-9.8136-.955zm1.1435.2136c.3267-.1633.5403-.49.5403-.867 0-.088-.0126-.1634-.0377-.2513l4.7372-3.2545-4.8 8.331zm.2513-14.3497l-9.889 4.31-.1131-.0755 1.2565-5.3906h.0377c.3393 0 .6409-.1759.8168-.4397l7.891 1.5706zM1.935 11.6105c0-.0629-.0126-.1257-.0126-.1885l3.9079-2.2995c.0754.0754.1633.1508.2638.201L4.8252 20.243l-3.2043-7.9036c.1885-.176.3142-.4398.3142-.7288Z' },
  h: { colour: '#00599c', path: 'M22.394 6c-.167-.29-.398-.543-.652-.69L12.926.22c-.509-.294-1.34-.294-1.848 0L2.26 5.31c-.508.293-.923 1.013-.923 1.6v10.18c0 .294.104.62.271.91.167.29.398.543.652.69l8.816 5.09c.508.293 1.34.293 1.848 0l8.816-5.09c.254-.147.485-.4.652-.69.167-.29.27-.616.27-.91V6.91c.003-.294-.1-.62-.268-.91zM12 19.11c-3.92 0-7.109-3.19-7.109-7.11 0-3.92 3.19-7.11 7.11-7.11a7.133 7.133 0 016.156 3.553l-3.076 1.78a3.567 3.567 0 00-3.08-1.78A3.56 3.56 0 008.444 12 3.56 3.56 0 0012 15.555a3.57 3.57 0 003.08-1.778l3.078 1.78A7.135 7.135 0 0112 19.11zm7.11-6.715h-.79v.79h-.79v-.79h-.79v-.79h.79v-.79h.79v.79h.79zm2.962 0h-.79v.79h-.79v-.79h-.79v-.79h.79v-.79h.79v.79h.79z' },
  cpp: { colour: '#00599c', path: 'M22.394 6c-.167-.29-.398-.543-.652-.69L12.926.22c-.509-.294-1.34-.294-1.848 0L2.26 5.31c-.508.293-.923 1.013-.923 1.6v10.18c0 .294.104.62.271.91.167.29.398.543.652.69l8.816 5.09c.508.293 1.34.293 1.848 0l8.816-5.09c.254-.147.485-.4.652-.69.167-.29.27-.616.27-.91V6.91c.003-.294-.1-.62-.268-.91zM12 19.11c-3.92 0-7.109-3.19-7.109-7.11 0-3.92 3.19-7.11 7.11-7.11a7.133 7.133 0 016.156 3.553l-3.076 1.78a3.567 3.567 0 00-3.08-1.78A3.56 3.56 0 008.444 12 3.56 3.56 0 0012 15.555a3.57 3.57 0 003.08-1.778l3.078 1.78A7.135 7.135 0 0112 19.11zm7.11-6.715h-.79v.79h-.79v-.79h-.79v-.79h.79v-.79h.79v.79h.79zm2.962 0h-.79v.79h-.79v-.79h-.79v-.79h.79v-.79h.79v.79h.79z' },
};

// Line icons for cards, 24-unit grid, stroked with currentColor
const LINE_ICONS = {
  model: '<circle cx="5" cy="6" r="2"/><circle cx="5" cy="18" r="2"/><circle cx="12" cy="12" r="2"/><circle cx="19" cy="6" r="2"/><circle cx="19" cy="18" r="2"/><path d="M7 6.8l3.2 4M7 17.2l3.2-4M13.8 11.2L17 6.8M13.8 12.8l3.2 4.4"/>',
  chip: '<rect x="6" y="6" width="12" height="12" rx="1"/><path d="M9 2v4M15 2v4M9 18v4M15 18v4M2 9h4M2 15h4M18 9h4M18 15h4"/>',
  shield: '<path d="M12 3l8 3v6c0 4.5-3.4 8-8 9-4.6-1-8-4.5-8-9V6z"/><path d="M8.5 12l2.5 2.5 4.5-5"/>',
  plug: '<path d="M9 3v5M15 3v5M6 8h12v3a6 6 0 0 1-12 0zM12 17v4"/>',
  slides: '<rect x="3" y="4" width="18" height="12" rx="1"/><path d="M12 16v4M8 20h8"/>',
  code: '<path d="M8 7l-5 5 5 5M16 7l5 5-5 5M14 4l-4 16"/>',
  folder: '<path d="M3 6a1 1 0 0 1 1-1h5l2 2h9a1 1 0 0 1 1 1v10a1 1 0 0 1-1 1H4a1 1 0 0 1-1-1z"/><path d="M8 13l2 2 4-4"/>',
  clock: '<circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 2"/>',
  wave: '<path d="M2 12c1.5-6 3-6 4.5 0s3 6 4.5 0 3-6 4.5 0 3 6 4.5 0 1.5-3 2-3"/>',
  sliders: '<path d="M6 3v18M12 3v18M18 3v18"/><rect x="4" y="13" width="4" height="3" rx="1"/><rect x="10" y="6" width="4" height="3" rx="1"/><rect x="16" y="10" width="4" height="3" rx="1"/>',
  people: '<circle cx="9" cy="8" r="3"/><path d="M3 20c0-3.3 2.7-6 6-6s6 2.7 6 6M16 5a3 3 0 0 1 0 6M18 14c2 .8 3 2.9 3 6"/>',
};

// {{FILE:forward_stateful.pt}} → a file icon with its extension on a badge, name below
// {{ICON:cpp}} → the C++ hexagon
function renderIcons(content) {
  // {{FILE:ring_buffer.h|new}} marks a file that is new in this step; a name without an
  // extension is a library fetched by CMake
  content = content.replace(/\{\{FILE:([^}|]+)(\|new)?\}\}/g, (match, name, isNew) => {
    const extension = name.includes('.') ? name.split('.').pop() : '';
    const colour = FILE_COLOURS[extension] || '#7f7f7f';
    return `<div class="file${isNew ? ' new' : ''}"><svg class="file-icon" viewBox="0 0 100 124" aria-hidden="true">` +
      `<path class="file-page" d="M8 4h58l26 26v86a4 4 0 0 1-4 4H8a4 4 0 0 1-4-4V8a4 4 0 0 1 4-4z"/>` +
      `<path class="file-fold" d="M66 4v22a4 4 0 0 0 4 4h22"/>` +
      // Signal headers hold audio, so they get a waveform rather than the C++ logo
      (/signal/.test(name)
        ? `<path transform="translate(22 14) scale(2)" fill="none" stroke="#8e6bd8" stroke-width="1.6" stroke-linecap="round" d="M1 12c1.5-6 3-6 4.5 0s3 6 4.5 0 3-6 4.5 0 3 6 4.5 0"/>`
        : FILE_LOGOS[extension]
          ? `<path transform="translate(25 14) scale(1.85)" fill="${FILE_LOGOS[extension].colour}" d="${FILE_LOGOS[extension].path}"/>`
          : '') +
      `<rect x="0" y="66" width="74" height="30" rx="5" fill="${colour}"/>` +
      `<text x="37" y="87" text-anchor="middle">${extension ? '.' + extension : 'lib'}</text></svg>` +
      `<div class="file-name">${name}</div></div>`;
  });
  content = content.replace(/\{\{ICON:(\w+)\}\}/g, (match, name) =>
    LINE_ICONS[name] ? `<svg class="card-icon" viewBox="0 0 24 24" aria-hidden="true">${LINE_ICONS[name]}</svg>` : match);
  return content.replace(/\{\{ICON:cpp\}\}/g,
    `<svg class="cpp-icon" viewBox="0 0 100 112" aria-hidden="true">` +
    `<path d="M50 2l46 26v56l-46 26L4 84V28z" fill="#00599c"/>` +
    `<path d="M50 2l46 26L50 56 4 28z" fill="#659ad2"/>` +
    `<path d="M96 28v56l-46 26V56z" fill="#004482"/>` +
    `<text x="50" y="68" text-anchor="middle">C++</text></svg>`);
}

// QR code as SVG, tuned to scan from the back of a room: square modules,
// rounded finder patterns, low error correction (a screen does not get
// scratched) and a 4-module quiet zone
function qrSvg(text) {
  const { modules } = QRCode.create(text, { errorCorrectionLevel: 'L' });
  const size = modules.size;
  const quiet = 4;
  const inFinder = (r, c) => (r < 7 || r >= size - 7) && (c < 7 || c >= size - 7) && !(r >= size - 7 && c >= size - 7);

  // One path for all modules, so neighbours merge without anti-aliasing seams
  let squares = '';
  for (let r = 0; r < size; ++r) {
    for (let c = 0; c < size; ++c) {
      if (modules.get(r, c) && !inFinder(r, c)) squares += `M${c} ${r}h1v1h-1z`;
    }
  }

  let finders = '';
  for (const [r, c] of [[0, 0], [0, size - 7], [size - 7, 0]]) {
    finders += `<rect x="${c + 0.5}" y="${r + 0.5}" width="6" height="6" rx="1.5" fill="none" stroke="currentColor" stroke-width="1"/>` +
      `<rect x="${c + 2}" y="${r + 2}" width="3" height="3" rx="0.75"/>`;
  }

  const box = size + 2 * quiet;
  return `<svg viewBox="${-quiet} ${-quiet} ${box} ${box}" fill="currentColor" aria-hidden="true">${finders}<path d="${squares}"/></svg>`;
}

// {{QR:https://...}} → an inline SVG QR code for that URL
async function renderQrCodes(content) {
  return content.replace(/\{\{QR:([^}]+)\}\}/g, (_, url) => `<div class="qr-code">${qrSvg(url.trim())}</div>`);
}

const KIND_LABELS = { exercise: 'hands-on', demo: 'demo', talk: 'talk' };

// "Step 3 Benchmarking and the real-time budget" -> { number: 3, name: "Benchmarking ..." }
function splitStepTitle(title) {
  const match = title.match(/^Step\s+(\d+)\s+(.*)$/);
  return match ? { number: match[1], name: match[2] } : { number: null, name: title };
}

// One column per part, separated by a break; chapters without a part are left out
function buildAgenda(chapters) {
  const parts = [];
  for (const chapter of chapters) {
    if (!chapter.part) continue;
    if (parts.length === 0 || parts[parts.length - 1].title !== chapter.part) {
      parts.push({ title: chapter.part, chapters: [] });
    }
    parts[parts.length - 1].chapters.push(chapter);
  }
  
  const columns = parts.map((p, i) => {
    // A part that is a single chapter of the same name links its heading instead
    const first = p.chapters[0];
    if (p.chapters.length === 1 && splitStepTitle(first.title).name === p.title) {
      return `<div class="agenda-part"><div class="agenda-part-label">Part ${i + 1}</div><h3><a href="#/${first.id}">${p.title}</a></h3></div>`;
    }
    const items = p.chapters.map(chapter => {
      const { number, name } = splitStepTitle(chapter.title);
      const label = number ? `<span class="agenda-number">${number}</span>` : '<span class="agenda-number"></span>';
      const kind = chapter.kinds.map(k => `<span class="agenda-kind ${k}">${KIND_LABELS[k] ?? k}</span>`).join('');
      return `<li><a href="#/${chapter.id}">${label}${name}</a>${kind}</li>`;
    }).join('');
    return `<div class="agenda-part"><div class="agenda-part-label">Part ${i + 1}</div><h3>${p.title}</h3><ol>${items}</ol></div>`;
  });
  
  return `<div class="agenda">${columns.join('<div class="agenda-break">Break</div>')}</div>`;
}

function extractTitle(content, isHtml) {
  let titleMatch = content.match(/<h1[^>]*>(.*?)<\/h1>/i);
  if (!titleMatch) {
    titleMatch = content.match(/^#\s+(.+)$/m);
  }
  const formatTitle = titleMatch ? titleMatch[1].replace(/<br\s*\/?>/gi, ' ').replace(/<[^>]*>/g, '').trim() : null;

  return formatTitle || 'Untitled Presentation';
}

async function generateIndexPage(chapters, distDir, isDev) {
  const indexTemplatePath = path.join(__dirname, '../slides/templates/index.html');
  const indexTemplate = await fs.readFile(indexTemplatePath, 'utf-8');
  
  const presentationsList = chapters.map(chapter => 
    `    <a href="workshop.html#/${chapter.id}" class="presentation-card">
      <h2>${chapter.title}</h2>
      <p class="filename">${chapter.part ?? 'Introduction'}</p>
    </a>`
  ).join('\n');
  
  const indexHTML = renderTemplate(indexTemplate, {
    PRESENTATIONS_LIST: presentationsList,
    HOT_RELOAD_SCRIPT: isDev ? '<script src="js/hot_reload.js"></script>' : ''
  });

  await fs.writeFile(path.join(distDir, 'index.html'), indexHTML);
  console.log('✅ Generated index.html (landing page)');
}

async function buildSlidesContent(slides) {
  let slidesContent = '';
  let markdownContent = '';
  let isInMarkdownSection = false;
  
  for (let i = 0; i < slides.length; i++) {
    const slide = slides[i];
    
    if (slide.isHtml) {
      // Close markdown section if we're in one
      if (isInMarkdownSection) {
        slidesContent += await createMarkdownSection(markdownContent);
        markdownContent = '';
        isInMarkdownSection = false;
      }
      // Add HTML slide
      slidesContent += slide.content + '\n      ';
    } else {
      // Add markdown content
      markdownContent += slide.content + '\n\n---\n\n';
      isInMarkdownSection = true;
    }
  }
  
  // Close final markdown section if needed
  if (isInMarkdownSection) {
    slidesContent += await createMarkdownSection(markdownContent);
  }
  
  return slidesContent;
}

async function createMarkdownSection(markdownContent) {
  // Process timeline imports before cleaning content
  const processedContent = await processTimelineImports(markdownContent);
  const cleanContent = processedContent.replace(/\n\n---\n\n$/, '');
  return `<section data-markdown data-separator="^---\\s*$">
        <textarea data-template>
${cleanContent}
        </textarea>
      </section>
      `;
}

async function processTimelineImports(content) {
  let processedContent = content;
  
  // Find all timeline divs with their placeholders
  // Match: <div class="timeline" ... data-timeline-fragments-select="...">{{TIMELINE:filename}}</div>
  const timelineDivPattern = /<div[^>]*class="[^"]*timeline[^"]*"[^>]*>(.*?\{\{TIMELINE:[^}]+\}\}.*?)<\/div>/gs;
  const allTimelineDivs = [...content.matchAll(timelineDivPattern)];
  
  for (const divMatch of allTimelineDivs) {
    const fullDiv = divMatch[0];
    const divInner = divMatch[1];
    
    // Extract the placeholder from within this div
    const placeholderMatch = divInner.match(/\{\{TIMELINE:([^}]+)\}\}/);
    if (!placeholderMatch) continue;
    
    const placeholder = placeholderMatch[0];
    const filename = placeholderMatch[1].trim();
    
    try {
      // Read the timeline HTML file
      const timelinePath = path.join(__dirname, '../slides/templates/timelines', `${filename}.html`);
      let timelineContent = await fs.readFile(timelinePath, 'utf-8');
      
      // Check for fragment attributes (select and color variants)
      const fragmentTypes = [
        { attr: 'data-timeline-fragments-select', className: 'select' },
        { attr: 'data-timeline-fragments-color-0', className: 'color-0' },
        { attr: 'data-timeline-fragments-color-1', className: 'color-1' },
        { attr: 'data-timeline-fragments-color-2', className: 'color-2' }
      ];
      
      let hasFragments = false;
      
      for (const { attr, className } of fragmentTypes) {
        const fragmentsMatch = fullDiv.match(new RegExp(`${attr}="([^"]*)"`, 'i'));
        
        if (fragmentsMatch) {
          hasFragments = true;
          const fragmentsAttr = fragmentsMatch[1];

          // Parse the fragments attribute: "year:index,year:index,..."
          const fragmentPairs = fragmentsAttr.split(',').map(s => s.trim());
          
          for (const pair of fragmentPairs) {
            const [year, index] = pair.split(':').map(s => s.trim());
            if (year && index) {
              // Replace each element type that matches this year
              timelineContent = timelineContent.replace(
                new RegExp(`<div class="timeline-dot" style="--year: ${year};">`, 'g'),
                `<div class="timeline-dot fragment custom ${className}" data-fragment-index="${index}" style="--year: ${year};">`
              );
              
              timelineContent = timelineContent.replace(
                new RegExp(`<div class="timeline-item" style="--year: ${year};">`, 'g'),
                `<div class="timeline-item fragment custom ${className}" data-fragment-index="${index}" style="--year: ${year};">`
              );

              if (attr !== 'data-timeline-fragments-select') {
                // Also apply to timeline-year within timeline-item
                timelineContent = timelineContent.replace(
                  new RegExp(`(<div class="timeline-item fragment custom ${className}" data-fragment-index="${index}" style="--year: ${year};">\\s*<div class="timeline-content">\\s*)<div class="timeline-year"`, 'g'),
                  `$1<div class="timeline-year fragment custom ${className}" data-fragment-index="${index}"`
                );
              }
            }
          }
        }
      }
      
      // Replace THIS SPECIFIC placeholder with the processed content
      // Use a more specific replacement that only targets this exact div
      const newDiv = fullDiv.replace(placeholder, timelineContent.trim());
      processedContent = processedContent.replace(fullDiv, newDiv);
    } catch (error) {
      console.error(`❌ Failed to import timeline ${filename}.html:`, error.message);
      // Keep the placeholder if import fails
    }
  }
  
  return processedContent;
}

const THEME_ICONS = `<svg class="icon-moon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"/></svg><svg class="icon-sun" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M4.93 4.93l1.41 1.41M17.66 17.66l1.41 1.41M2 12h2M20 12h2M4.93 19.07l1.41-1.41M17.66 6.34l1.41-1.41"/></svg>`;

function renderTemplate(template, data) {
  let result = template;
  // Stamped onto every css/ and js/ link, so a new build is never served from a stale cache
  data = { THEME_ICONS, BUILD_ID: Date.now().toString(36), ...data };
  
  // Replace all placeholders
  for (const [key, value] of Object.entries(data)) {
    const placeholder = `{{${key}}}`;
    result = result.replace(new RegExp(placeholder, 'g'), value);
  }
  
  return result;
}

async function copyAllFolders() {
  const jsDir = path.join(__dirname, '../slides/js');
  const cssDir = path.join(__dirname, '../slides/css');
  const assetsDir = path.join(__dirname, '../slides/assets');
  const distDir = path.join(__dirname, '../dist');
  
  // Create dist directory
  await fs.mkdir(distDir, { recursive: true });
  
  // Copy all folders from slides/src to dist
  const entries = { jsDir, cssDir, assetsDir };
  for (const [key, srcDir] of Object.entries(entries)) {
    const destDir = path.join(distDir, key.replace('Dir', ''));
    try {
      await copyDirectory(srcDir, destDir);
      console.log(`📂 Copied ${key.replace('Dir', '')}/`);
    } catch (error) {
      console.error(`❌ Failed to copy ${key.replace('Dir', '')}:`, error.message);
    }
  }
  
  // Copy reveal.js from node_modules
  const nodeModulesReveal = path.join(__dirname, '../node_modules/reveal.js');
  const distReveal = path.join(distDir, 'reveal.js');
  
  try {
    // Copy reveal.js core files
    const revealDistDir = path.join(nodeModulesReveal, 'dist');
    const targetDistDir = path.join(distReveal, 'dist');
    await copyDirectory(revealDistDir, targetDistDir);
    
    // Copy plugins
    const pluginDir = path.join(nodeModulesReveal, 'plugin');
    const targetPluginDir = path.join(distReveal, 'plugin');
    await copyDirectory(pluginDir, targetPluginDir);
    
    console.log('📂 Copied reveal.js/');
  } catch (error) {
    console.error('❌ Failed to copy reveal.js files:', error.message);
  }
  
  // Copy KaTeX from node_modules
  const nodeModulesKatex = path.join(__dirname, '../node_modules/katex');
  const distKatex = path.join(distDir, 'katex');
  
  try {
    // Copy KaTeX dist files
    const katexDistDir = path.join(nodeModulesKatex, 'dist');
    const targetDistDir = path.join(distKatex, 'dist');
    await copyDirectory(katexDistDir, targetDistDir);

    console.log('📂 Copied katex/');
  } catch (error) {
    console.error('❌ Failed to copy katex files:', error.message);
  }
}

async function copyDirectory(src, dest) {
  await fs.mkdir(dest, { recursive: true });
  const entries = await fs.readdir(src, { withFileTypes: true });
  
  for (const entry of entries) {
    const srcPath = path.join(src, entry.name);
    const destPath = path.join(dest, entry.name);
    
    if (entry.isDirectory()) {
      await copyDirectory(srcPath, destPath);
    } else {
      await fs.copyFile(srcPath, destPath);
    }
  }
}

async function build(isDev = false) {
  try {
    console.log('🔨 Building presentation...');
    await generateHTML(isDev);
    await copyAllFolders();
    console.log('✅ Build process completed!');
  } catch (error) {
    console.error('❌ Build failed:', error.message);
    process.exit(1);
  }
}

if (require.main === module) {
  build();
}

module.exports = { build, combineSlides };