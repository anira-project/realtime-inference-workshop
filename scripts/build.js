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
    // exercise, demo or talk — shown as a badge in the agenda
    const kindMatch = slide.content.match(/<!--\s*kind:\s*(\w+)\s*-->/);
    chapters.push({
      ...slide,
      id: slide.filename.replace(/\.(md|html)$/, '').replace(/^\d+-/, ''),
      title: extractTitle(slide.content, slide.isHtml),
      part: part,
      kind: kindMatch ? kindMatch[1] : null
    });
  }
  
  const agenda = buildAgenda(chapters);
  const integrationTemplate = await fs.readFile(
    path.join(__dirname, '../slides/templates/integration.html'), 'utf-8');
  let slidesContent = '';
  for (const chapter of chapters) {
    const content = await renderQrCodes(renderIntegration(
      chapter.content.replace(/\{\{AGENDA\}\}/g, agenda), integrationTemplate));
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

// {{QR:https://...}} → an inline SVG QR code for that URL
async function renderQrCodes(content) {
  const matches = [...content.matchAll(/\{\{QR:([^}]+)\}\}/g)];
  for (const match of matches) {
    const svg = await QRCode.toString(match[1].trim(), { type: 'svg', margin: 0 });
    content = content.replace(match[0], `<div class="qr-code">${svg}</div>`);
  }
  return content;
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
      const kind = chapter.kind ? `<span class="agenda-kind ${chapter.kind}">${KIND_LABELS[chapter.kind] ?? chapter.kind}</span>` : '';
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
  data = { THEME_ICONS, ...data };
  
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