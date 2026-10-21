// WebPage.h - the web app served at http://192.168.4.1/ (self-contained: no external files,
// fonts, scripts or network access needed). Edited as plain HTML/CSS/JS inside a raw string.
#ifndef CALIBRATOR_WEB_PAGE_H
#define CALIBRATOR_WEB_PAGE_H

#include <pgmspace.h>

static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Ojo del Sol Calibrator</title>
<style>
:root{
  --bg:#f3f4f6; --card:#ffffff; --ink:#1a1f26; --muted:#58626e; --line:#d9dee4;
  --accent:#1d4ed8; --accent-ink:#ffffff; --cart:#1465a8; --uv:#c25a00;
  --good:#15803d; --warn:#a15c00; --bad:#b42318; --seg:#e3e7ec; --seg-on:#1465a8;
  --shadow:0 1px 2px rgba(16,24,40,.06);
}
@media (prefers-color-scheme: dark){
  :root{
    --bg:#0f1317; --card:#171c22; --ink:#e7eaee; --muted:#98a3af; --line:#2a323b;
    --accent:#7aa7ff; --accent-ink:#0b1220; --cart:#5eb0f0; --uv:#ffa24f;
    --good:#4cc47a; --warn:#f0b24a; --bad:#ff8085; --seg:#262d36; --seg-on:#5eb0f0;
    --shadow:none;
  }
}
*{box-sizing:border-box}
html{-webkit-text-size-adjust:100%}
body{margin:0;background:var(--bg);color:var(--ink);font:15px/1.45 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
header{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:14px 16px 8px;max-width:980px;margin:0 auto}
h1{font-size:18px;margin:0;font-weight:650;letter-spacing:.1px}
h2{font-size:14px;margin:0 0 10px;font-weight:650;color:var(--muted);text-transform:uppercase;letter-spacing:.6px}
.pill{font-size:12px;padding:3px 10px;border-radius:99px;border:1px solid var(--line);background:var(--card);color:var(--muted);white-space:nowrap}
.pill.ok{color:var(--good);border-color:var(--good)}
.pill.bad{color:var(--bad);border-color:var(--bad)}
nav{display:flex;gap:4px;padding:0 16px;max-width:980px;margin:0 auto;border-bottom:1px solid var(--line)}
nav button{flex:1;appearance:none;background:none;border:0;border-bottom:3px solid transparent;padding:10px 4px;font:inherit;font-weight:600;color:var(--muted);cursor:pointer}
nav button[aria-selected="true"]{color:var(--ink);border-bottom-color:var(--accent)}
main{max-width:980px;margin:0 auto;padding:14px 16px 60px}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:14px;margin-bottom:14px;box-shadow:var(--shadow)}
.row{display:flex;flex-wrap:wrap;gap:10px;align-items:center}
.grid2{display:grid;grid-template-columns:1fr 1fr;gap:14px}
.readout{display:flex;flex-direction:column;gap:2px;min-width:0}
.readout .lab{font-size:12px;color:var(--muted);text-transform:uppercase;letter-spacing:.5px}
.readout .big{font-size:42px;line-height:1.05;font-weight:700;font-variant-numeric:tabular-nums}
.readout .sub{font-size:13px;color:var(--muted)}
.readout.cart .big{color:var(--cart)} .readout.uv .big{color:var(--uv)}
.gauge{display:grid;grid-template-columns:118px 1fr 48px;gap:10px;align-items:center;margin:7px 0}
.gauge .gl{font-size:13px;color:var(--muted)}
.gauge .gv{font-size:13px;font-variant-numeric:tabular-nums;text-align:right}
.segs{display:flex;gap:3px}
.segs i{flex:1;height:14px;border-radius:3px;background:var(--seg)}
.segs i.on{background:var(--seg-on)}
canvas{width:100%;display:block}
.chart{height:230px} .chart.tall{height:260px}
.legend{display:flex;gap:14px;flex-wrap:wrap;font-size:12.5px;color:var(--muted);margin:6px 0 0}
.legend b{display:inline-block;width:14px;height:3px;border-radius:2px;vertical-align:middle;margin-right:5px}
button.btn,a.btn,label.btn{appearance:none;display:inline-block;font:inherit;font-weight:600;font-size:14px;padding:8px 14px;border-radius:9px;border:1px solid var(--line);background:var(--card);color:var(--ink);cursor:pointer;text-decoration:none;text-align:center}
.btn.primary{background:var(--accent);border-color:var(--accent);color:var(--accent-ink)}
.btn.danger{color:var(--bad);border-color:var(--bad)}
.btn.small{padding:5px 10px;font-size:13px}
.btn:disabled,.btn[aria-disabled="true"]{opacity:.45;cursor:not-allowed}
.seg-btns{display:inline-flex;border:1px solid var(--line);border-radius:9px;overflow:hidden}
.seg-btns button{appearance:none;border:0;background:var(--card);color:var(--muted);font:inherit;font-size:13px;font-weight:600;padding:6px 12px;cursor:pointer;border-right:1px solid var(--line)}
.seg-btns button:last-child{border-right:0}
.seg-btns button.on{background:var(--accent);color:var(--accent-ink)}
select,input[type=number],input[type=text]{font:inherit;padding:7px 9px;border-radius:8px;border:1px solid var(--line);background:var(--card);color:var(--ink);min-width:0}
input[type=range]{width:100%}
table{width:100%;border-collapse:collapse;font-size:14px}
th{font-size:12px;text-align:left;color:var(--muted);text-transform:uppercase;letter-spacing:.4px;padding:6px 8px;border-bottom:1px solid var(--line)}
td{padding:9px 8px;border-bottom:1px solid var(--line);vertical-align:middle}
td.num,th.num{text-align:right;font-variant-numeric:tabular-nums}
tr:last-child td{border-bottom:0}
.banner{border:1px solid var(--warn);border-radius:10px;padding:10px 12px;margin-bottom:14px;font-size:14px;background:var(--card)}
.banner.bad{border-color:var(--bad)}
.file{display:flex;flex-wrap:wrap;align-items:center;gap:6px 12px;padding:10px 0;border-bottom:1px solid var(--line)}
.file:last-child{border-bottom:0}
.file .fn{flex:1 1 130px;min-width:0}
.file .acts{display:flex;gap:6px;flex-wrap:wrap}
.tag{font-size:11px;font-weight:700;padding:2px 7px;border-radius:99px;background:var(--seg);color:var(--muted)}
.tag.rec{background:var(--good);color:#fff}
.meter{height:12px;border-radius:99px;background:var(--seg);overflow:hidden}
.meter i{display:block;height:100%;background:var(--accent)}
.meter.warn i{background:var(--warn)} .meter.bad i{background:var(--bad)}
.muted{color:var(--muted)} .small{font-size:13px}
.note{font-size:13px;color:var(--muted);margin:8px 0 0}
.err{color:var(--bad)}
.hidden{display:none !important}
.stats{display:grid;grid-template-columns:repeat(auto-fit,minmax(130px,1fr));gap:8px 14px;margin-top:10px}
.stats div{font-size:13px;color:var(--muted)} .stats b{display:block;font-size:16px;color:var(--ink);font-variant-numeric:tabular-nums}
.toast{position:fixed;left:50%;bottom:18px;transform:translate(-50%,20px);background:var(--ink);color:var(--bg);padding:10px 16px;border-radius:10px;font-size:14px;opacity:0;pointer-events:none;transition:.2s;max-width:90vw;z-index:30}
.toast.show{opacity:1;transform:translate(-50%,0)} .toast.bad{background:var(--bad);color:#fff}
.modal{position:fixed;inset:0;background:rgba(0,0,0,.5);display:none;align-items:center;justify-content:center;padding:16px;z-index:40}
.modal.show{display:flex}
.modal .box{background:var(--card);color:var(--ink);border-radius:14px;padding:18px;max-width:420px;width:100%}
.modal h3{margin:0 0 8px;font-size:17px} .modal p{margin:0 0 12px;color:var(--muted)}
.modal input{width:100%;margin-bottom:12px}
.modal .row{justify-content:flex-end}
@media (max-width:520px){
  .readout .big{font-size:34px}
  .gauge{grid-template-columns:96px 1fr 42px}
  .grid2{gap:10px}
}
</style>
</head>
<body>
<header>
  <h1>Ojo del Sol Calibrator</h1>
  <span id="conn" class="pill">connecting…</span>
</header>
<nav role="tablist">
  <button role="tab" data-tab="live" aria-selected="true">Live</button>
  <button role="tab" data-tab="history" aria-selected="false">History</button>
  <button role="tab" data-tab="storage" aria-selected="false">Storage</button>
</nav>
<noscript><main><div class="card">This page needs JavaScript.</div></main></noscript>
<main>
<div id="banner" class="banner hidden" role="alert"></div>

<!-- ================= LIVE ================= -->
<section id="tab-live">
  <div class="card">
    <div class="grid2">
      <div class="readout cart"><span class="lab">Cartridge level</span><span class="big" id="cartLevel">—</span><span class="sub" id="cartRaw">&nbsp;</span></div>
      <div class="readout uv"><span class="lab">LTR390 UV index</span><span class="big" id="uvi">—</span><span class="sub" id="ltrRaw">&nbsp;</span></div>
    </div>
    <p class="note err" id="liveNote"></p>
  </div>

  <div class="card">
    <h2>Bars the game would show</h2>
    <div id="gauges"></div>
    <div class="row" style="margin-top:12px">
      <label class="small muted" for="calIn">Calibration</label>
      <input id="calIn" type="number" min="0" max="255" step="1" style="width:84px">
      <button class="btn small" id="calFromNow" title="Cover the cartridge's sensor first">Use current reading − 2</button>
    </div>
    <p class="note">Level = calibration − raw cartridge value, limited to 0–140. The game sets its calibration from the raw value at its sensor setup, minus 2 (cover the sensor, then press the button). The default 230 is only a starting guess.</p>
  </div>

  <div class="card">
    <div class="row" style="justify-content:space-between">
      <h2 style="margin:0">Recent readings</h2>
      <div class="seg-btns" id="liveWin">
        <button data-w="120000">2 min</button><button data-w="600000" class="on">10 min</button><button data-w="0">All</button>
      </div>
    </div>
    <canvas id="liveChart" class="chart"></canvas>
    <div class="legend"><span><b style="background:var(--cart)"></b>Cartridge level (left)</span><span><b style="background:var(--uv)"></b>UV index (right)</span></div>
  </div>
</section>

<!-- ================= HISTORY ================= -->
<section id="tab-history" class="hidden">
  <div class="card">
    <div class="row">
      <select id="hSession" style="flex:1;min-width:150px"></select>
      <button class="btn primary" id="hLoad">Load</button>
      <a class="btn" id="hDownload" aria-disabled="true">Download CSV</a>
    </div>
    <p class="note" id="hInfo">Choose a session and press Load.</p>
  </div>
  <div id="hBody" class="hidden">
    <div class="card">
      <div class="row" style="justify-content:space-between">
        <h2 style="margin:0">Over time</h2>
        <span class="small muted" id="hRange"></span>
      </div>
      <canvas id="hChart" class="chart tall"></canvas>
      <div class="legend"><span><b style="background:var(--cart)"></b>Cartridge level (left)</span><span><b style="background:var(--uv)"></b>UV index (right)</span></div>
      <div style="margin-top:10px">
        <input type="range" id="zFrom" min="0" max="1000" value="0" aria-label="Zoom start">
        <input type="range" id="zTo" min="0" max="1000" value="1000" aria-label="Zoom end">
      </div>
      <div class="stats" id="hStats"></div>
    </div>
    <div class="card">
      <div class="row" style="justify-content:space-between">
        <h2 style="margin:0">UV index vs cartridge</h2>
        <select id="hTable" aria-label="Bar thresholds"></select>
      </div>
      <canvas id="hScatter" class="chart tall"></canvas>
      <p class="note">Each dot is one sample (in the zoomed range). Dashed lines are the game's bar thresholds; the solid line is the average cartridge level at each UV index.</p>
      <h2 style="margin-top:14px">UV index where the cartridge reaches each bar (rough estimate)</h2>
      <div id="hEst" class="small"></div>
    </div>
  </div>
</section>

<!-- ================= STORAGE ================= -->
<section id="tab-storage" class="hidden">
  <div class="card">
    <h2>Flash storage</h2>
    <div class="meter" id="fsMeter"><i style="width:0"></i></div>
    <p class="note" id="fsText">…</p>
  </div>

  <div class="card">
    <h2>Sessions</h2>
    <div id="fileList" class="small muted">Loading…</div>
    <p class="note">A new session starts every time the device powers up. Downloads or uploads not working? Captive-portal pop-ups often block them: open <b id="hostHint">http://192.168.4.1</b> in your normal browser instead.</p>
  </div>

  <div class="card">
    <h2>Backup &amp; restore</h2>
    <div class="row">
      <button class="btn primary" id="bkAll">Download backup of all sessions</button>
    </div>
    <p class="note" id="bkStatus">Saves every session into one .tar file (opens with 7-Zip, Windows 11 Explorer, macOS and Linux).</p>
    <hr style="border:0;border-top:1px solid var(--line);margin:14px 0">
    <div class="row">
      <label class="btn" for="rsFile">Restore from file…</label>
      <input id="rsFile" type="file" accept=".tar,.csv" multiple class="hidden">
      <label class="small"><input type="checkbox" id="rsOver"> Replace sessions that already exist</label>
    </div>
    <p class="note" id="rsStatus">Accepts a backup .tar or individual cal_NNNN.csv files.</p>
  </div>

  <div class="card">
    <h2>Recording</h2>
    <div class="row">
      <label class="small" for="ivSel">Sample every</label>
      <select id="ivSel"></select>
      <button class="btn small" id="ivApply">Apply</button>
      <span class="small muted" id="ivNote"></span>
    </div>
    <p class="note">A sample takes about 1 second, so shorter intervals do not help. The setting is remembered across restarts.</p>
    <div class="row" style="margin-top:12px">
      <button class="btn" id="newSess">Start a new session</button>
      <button class="btn" id="syncClock">Sync clock from this device</button>
      <span class="small muted" id="clockNote"></span>
    </div>
  </div>

  <div class="card">
    <h2>Danger zone</h2>
    <div class="row">
      <button class="btn danger" id="delAll">Delete all sessions except the one recording</button>
      <button class="btn danger" id="fmt">Format storage…</button>
    </div>
    <p class="note">Deleting cannot be undone. Download a backup first.</p>
  </div>
</section>

</main>
<div id="toast" class="toast" role="status"></div>
<div id="modal" class="modal" role="dialog" aria-modal="true">
  <div class="box">
    <h3 id="mTitle"></h3><p id="mBody"></p>
    <input id="mInput" type="text" autocomplete="off" autocapitalize="characters">
    <div class="row"><button class="btn" id="mCancel">Cancel</button><button class="btn primary" id="mOk">Confirm</button></div>
  </div>
</div>

<script>
(function(){
'use strict';
const $=(s,r)=>(r||document).querySelector(s);
const $$=(s,r)=>Array.from((r||document).querySelectorAll(s));
const esc=s=>String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const pad2=n=>String(n).padStart(2,'0');
const fmtBytes=n=>n>=1048576?(n/1048576).toFixed(2)+' MB':n>=1024?(n/1024).toFixed(1)+' KB':n+' B';
const fmtDur=ms=>{let s=Math.max(0,Math.round(ms/1000));const h=Math.floor(s/3600);s-=h*3600;const m=Math.floor(s/60);s-=m*60;return h?h+'h '+pad2(m)+'m':m?m+'m '+pad2(s)+'s':s+'s';};
const fmtClock=ep=>{const d=new Date(ep);return pad2(d.getHours())+':'+pad2(d.getMinutes());};
const fmtDateTime=ep=>new Date(ep).toLocaleString([],{month:'short',day:'numeric',hour:'2-digit',minute:'2-digit'});
const css=n=>getComputedStyle(document.documentElement).getPropertyValue(n).trim();

// Bar thresholds (first calibrated level at which each bar starts) from Raphi's sensor graph,
// plus the Boktai 1 (E) table found by disassembling that ROM.
const TABLES={
  b23:{name:'Boktai 2 / 3',th:[1,6,13,23,35,50,67,87,110,140]},
  b1:{name:'Boktai 1 (J, U)',th:[1,7,16,28,44,67,98,140]},
  b1e:{name:'Boktai 1 (E)',th:[1,7,15,25,38,56,98,140]}
};
const barsFor=(lvl,th)=>th.reduce((n,t)=>n+(lvl>=t?1:0),0);

const store={
  get(k,d){try{const v=localStorage.getItem('ods_'+k);return v===null?d:JSON.parse(v);}catch(e){return d;}},
  set(k,v){try{localStorage.setItem('ods_'+k,JSON.stringify(v));}catch(e){}}
};
let cal=Number(store.get('cal',230)); if(!(cal>=0&&cal<=255)) cal=230;
let tableKey=store.get('table','b23'); if(!TABLES[tableKey]) tableKey='b23';
const level=raw=>(raw==null||raw<0||isNaN(raw))?null:Math.max(0,Math.min(140,cal-raw));

// ------------------------------------------------------------------ helpers
let toastTimer;
function toast(msg,bad){const t=$('#toast');t.textContent=msg;t.className='toast show'+(bad?' bad':'');clearTimeout(toastTimer);toastTimer=setTimeout(()=>{t.className='toast';},4200);}
function askConfirm(title,body,o){
  o=o||{};
  return new Promise(res=>{
    const m=$('#modal'),inp=$('#mInput'),ok=$('#mOk'),cancel=$('#mCancel');
    $('#mTitle').textContent=title;$('#mBody').textContent=body;
    inp.classList.toggle('hidden',!o.typed);inp.value='';inp.placeholder=o.typed?'Type '+o.typed+' to confirm':'';
    ok.textContent=o.ok||'Confirm';ok.className=o.danger?'btn danger':'btn primary';
    const done=v=>{m.classList.remove('show');ok.onclick=cancel.onclick=null;res(v);};
    ok.onclick=()=>{if(o.typed&&inp.value.trim().toUpperCase()!==o.typed){inp.focus();return;}done(true);};
    cancel.onclick=()=>done(false);
    m.classList.add('show');if(o.typed)inp.focus();
  });
}
async function api(path,opts,timeout){
  const ctl=new AbortController();const t=setTimeout(()=>ctl.abort(),timeout||8000);
  try{
    const r=await fetch(path,Object.assign({signal:ctl.signal,cache:'no-store'},opts||{}));
    const ct=r.headers.get('content-type')||'';
    const body=ct.indexOf('json')>=0?await r.json():await r.text();
    if(!r.ok)throw new Error((body&&body.error)||('HTTP '+r.status));
    return body;
  }finally{clearTimeout(t);}
}
const post=(p)=>api(p,{method:'POST'});

// ------------------------------------------------------------------ charts
function niceStep(range,count){
  const raw=range/count,mag=Math.pow(10,Math.floor(Math.log10(raw||1))),n=raw/mag;
  return (n<1.5?1:n<3?2:n<7?5:10)*mag;
}
function niceMax(v){if(!(v>0))return 1;const s=niceStep(v,5);return Math.ceil(v/s)*s;}
function setupCanvas(cv){
  const dpr=window.devicePixelRatio||1,w=cv.clientWidth,h=cv.clientHeight;
  if(!w||!h)return null;
  cv.width=Math.round(w*dpr);cv.height=Math.round(h*dpr);
  const g=cv.getContext('2d');g.setTransform(dpr,0,0,dpr,0,0);
  return {g,w,h};
}
// o: {x0,x1,xTicks:[{x,label}],left:{max,ticks,fmt},right:{max,ticks,fmt},series:[{pts:[[x,y]],color,axis,gap}]}
function drawChart(cv,o){
  const c=setupCanvas(cv);if(!c)return;
  const {g,w,h}=c,ink=css('--muted'),line=css('--line');
  const L=40,R=o.right?44:10,T=10,B=24,pw=w-L-R,ph=h-T-B;
  g.clearRect(0,0,w,h);g.font='11px system-ui,sans-serif';g.lineWidth=1;
  const X=x=>L+(x-o.x0)/(o.x1-o.x0||1)*pw;
  const Yl=y=>T+ph-(y/o.left.max)*ph;
  const Yr=o.right?(y=>T+ph-(y/o.right.max)*ph):null;
  g.strokeStyle=line;g.fillStyle=ink;g.textAlign='right';g.textBaseline='middle';
  o.left.ticks.forEach(t=>{const y=Yl(t);g.beginPath();g.moveTo(L,y+.5);g.lineTo(L+pw,y+.5);g.stroke();g.fillText(o.left.fmt(t),L-6,y);});
  if(o.right){g.textAlign='left';o.right.ticks.forEach(t=>{g.fillText(o.right.fmt(t),L+pw+6,Yr(t));});}
  g.textAlign='center';g.textBaseline='top';
  let lastEnd=-1e9;
  o.xTicks.slice().sort((a,b)=>a.x-b.x).forEach(t=>{
    const x=X(t.x);if(x<L-1||x>L+pw+1)return;
    g.beginPath();g.moveTo(x+.5,T+ph);g.lineTo(x+.5,T+ph+4);g.stroke();
    const tw=g.measureText(t.label).width;
    if(x-tw/2<lastEnd+6||x-tw/2<0||x+tw/2>w)return;   // skip labels that would collide or run off
    g.fillText(t.label,x,T+ph+6);lastEnd=x+tw/2;
  });
  g.save();g.beginPath();g.rect(L,T-2,pw,ph+4);g.clip();
  o.series.forEach(s=>{
    const Y=s.axis==='r'?Yr:Yl;if(!Y)return;
    const nb=Math.max(1,Math.floor(pw)),sum=new Float64Array(nb),tsum=new Float64Array(nb),cnt=new Uint32Array(nb);
    for(const p of s.pts){if(p[0]<o.x0||p[0]>o.x1)continue;let b=Math.floor((p[0]-o.x0)/(o.x1-o.x0||1)*nb);if(b>=nb)b=nb-1;sum[b]+=p[1];tsum[b]+=p[0];cnt[b]++;}
    g.strokeStyle=s.color;g.lineWidth=2;g.lineJoin='round';g.beginPath();
    // a break in the line = a real hole in the data, not just several samples sharing a pixel
    const gapMs=Math.max(s.gap,2.5*(o.x1-o.x0)/nb);
    let prevT=null,pen=false;
    for(let b=0;b<nb;b++){
      if(!cnt[b])continue;
      const t=tsum[b]/cnt[b],x=L+b+.5,y=Y(sum[b]/cnt[b]);
      if(pen&&prevT!==null&&t-prevT<=gapMs)g.lineTo(x,y);else{g.moveTo(x,y);g.lineTo(x+.01,y);}
      pen=true;prevT=t;
    }
    g.stroke();
  });
  g.restore();
  g.strokeStyle=line;g.strokeRect(L+.5,T+.5,pw,ph);
}
function medianGap(ts){
  const d=[];for(let i=1;i<ts.length&&d.length<400;i++)d.push(ts[i]-ts[i-1]);
  if(!d.length)return 5000;d.sort((a,b)=>a-b);return Math.max(d[d.length>>1]*4,1500);
}
// Ticks anchored at the right end (live chart: "now") or at the left end (history: the start).
function timeTicks(x0,x1,labeler,fromRight){
  const span=x1-x0,steps=[1,2,5,10,15,30,60,120,300,600,900,1800,3600,7200,14400,21600,43200,86400].map(s=>s*1000);
  let step=steps[steps.length-1];for(const s of steps){if(span/s<=5){step=s;break;}}
  const out=[];
  if(fromRight){for(let x=x1;x>=x0;x-=step)out.push({x,label:labeler(x)});}
  else{for(let x=x0;x<=x1;x+=step)out.push({x,label:labeler(x)});}
  return out;
}

// ------------------------------------------------------------------ LIVE tab
const live={seq:0,pts:[],win:600000,nowUp:0,statusAt:0};
const MAX_LIVE=600;
function buildGauges(){
  const box=$('#gauges');box.innerHTML='';
  Object.keys(TABLES).forEach(k=>{
    const t=TABLES[k],d=document.createElement('div');d.className='gauge';d.dataset.k=k;
    d.innerHTML='<span class="gl">'+esc(t.name)+'</span><div class="segs"></div><span class="gv"></span>';
    const segs=d.querySelector('.segs');for(let i=0;i<t.th.length;i++)segs.appendChild(document.createElement('i'));
    box.appendChild(d);
  });
}
function renderLive(){
  const p=live.pts[live.pts.length-1];
  const lv=p?level(p.cart):null;
  $('#cartLevel').textContent=lv==null?'—':Math.round(lv);
  $('#cartRaw').textContent=p?(p.cart<0?'no reading (timeout)':'raw '+p.cart):' ';
  const uvi=p&&p.ltr>=0?p.ltr/2300:null;
  $('#uvi').textContent=uvi==null?'—':uvi.toFixed(2);
  $('#ltrRaw').textContent=p?(p.ltr<0?(status&&status.ltrReady===false?'sensor not found':'no fresh reading'):p.ltr+' counts'):' ';
  $$('#gauges .gauge').forEach(d=>{
    const t=TABLES[d.dataset.k],n=lv==null?0:barsFor(lv,t.th);
    $$('.segs i',d).forEach((s,i)=>s.classList.toggle('on',i<n));
    $('.gv',d).textContent=lv==null?'—':n+'/'+t.th.length;
  });
  const notes=[];
  if(p&&p.cart<0)notes.push('Cartridge: no answer. Check its wiring and that it is fully seated in the socket.');
  else if(p&&p.bus===0)notes.push('Cartridge: it answered, but the bus read-back check failed, so this reading may be noise. Check the wiring.');
  if(status&&status.ltrReady===false)notes.push('LTR390: not found. Check its wiring (SDA GPIO4, SCL GPIO5, 3V3, GND). The device retries every 30 seconds.');
  $('#liveNote').innerHTML=notes.map(esc).join('<br>');
  drawLive();
}
function drawLive(){
  const cv=$('#liveChart');if(!cv.clientWidth)return;
  const pts=live.pts;if(!pts.length){drawChart(cv,{x0:0,x1:1,xTicks:[],left:{max:140,ticks:[0,35,70,105,140],fmt:v=>v},right:{max:1,ticks:[0,.5,1],fmt:v=>v.toFixed(1)},series:[]});return;}
  const x1=pts[pts.length-1].up,x0=live.win?Math.max(pts[0].up,x1-live.win):pts[0].up;
  const cart=[],uv=[];let maxU=0;
  for(const p of pts){if(p.up<x0)continue;const l=level(p.cart);if(l!=null)cart.push([p.up,l]);if(p.ltr>=0){const u=p.ltr/2300;uv.push([p.up,u]);if(u>maxU)maxU=u;}}
  const rm=niceMax(Math.max(maxU*1.1,1)),rs=rm/4;
  const gap=medianGap(pts.map(p=>p.up));
  drawChart(cv,{x0,x1:Math.max(x1,x0+1000),
    xTicks:timeTicks(x0,x1,x=>{const s=Math.round((x-x1)/1000);return s===0?'now':'-'+(Math.abs(s)>=60?Math.round(Math.abs(s)/60)+'m':Math.abs(s)+'s');},true),
    left:{max:140,ticks:[0,35,70,105,140],fmt:v=>v},
    right:{max:rm,ticks:[0,rs,rs*2,rs*3,rm],fmt:v=>v.toFixed(rm<4?1:0)},
    series:[{pts:cart,color:css('--cart'),axis:'l',gap},{pts:uv,color:css('--uv'),axis:'r',gap}]});
}
function setConn(ok,text){const c=$('#conn');c.textContent=text||(ok?'connected':'offline');c.className='pill '+(ok?'ok':'bad');}
async function pollLive(){
  try{
    const d=await api('/api/live?since='+live.seq,null,4000);
    if(d.seq<live.seq||d.up<live.nowUp){live.pts=[];}
    live.seq=d.seq;live.nowUp=d.up;
    for(const s of d.s)live.pts.push({seq:s[0],up:s[1],cart:s[2],ltr:s[3],bus:s[4]});
    if(live.pts.length>MAX_LIVE)live.pts.splice(0,live.pts.length-MAX_LIVE);
    setConn(true);renderLive();
  }catch(e){setConn(false);}
}
let status=null,clockSent=false;
function updateBanner(){
  const b=$('#banner');let msg='',bad=false;
  if(status){
    if(status.storage==='unmounted'){
      bad=true;
      msg=status.flashBytes&&status.flashBytes<16*1048576
        ?'This board reports only '+Math.round(status.flashBytes/1048576)+' MB of flash, but partitions.csv assumes 16 MB, so the log storage cannot mount. See the README (edit or delete partitions.csv).'
        :'Storage has not been set up yet. This is normal on a first run: open the Storage tab and press "Format storage…". Nothing is recorded until then.';
    }else if(status.storage==='full'){bad=true;msg='Storage is full and recording has stopped. Download what you need, then delete old sessions to resume.';}
    else if(status.storage==='error'){bad=true;msg='The log file could not be written. The device keeps retrying; if this persists, check the Storage tab.';}
  }
  b.textContent=msg;b.classList.toggle('hidden',!msg);b.classList.toggle('bad',bad);
}
async function pollStatus(){
  try{
    status=await api('/api/status',null,4000);
    setConn(true);updateBanner();renderLive();
    if(!clockSent){clockSent=true;api('/api/time?ms='+Date.now(),{method:'POST'}).then(()=>{status.timeSynced=true;renderStorageStatus();}).catch(()=>{clockSent=false;});}
    renderStorageStatus();
  }catch(e){setConn(false);}
}

// ------------------------------------------------------------------ HISTORY tab
const hist={n:0,up:null,cart:null,uvi:null,syncs:[],x0:0,x1:1,view:[0,1]};
function parseCsv(text){
  const lines=text.split('\n'),up=[],cart=[],uvi=[],syncs=[];let timeouts=0,stale=0;
  for(let i=0;i<lines.length;i++){
    const l=lines[i];if(!l)continue;
    if(l.charCodeAt(0)===35){const m=/^# time_sync,(\d+),(\d+)/.exec(l);if(m)syncs.push([+m[1],+m[2]]);continue;}
    if(l.startsWith('uptime_ms'))continue;
    const f=l.replace(/\r$/,'').split(',');if(f.length<8)continue;
    const t=+f[0];if(!isFinite(t))continue;
    up.push(t);
    if(f[1]==='ok'&&f[2]!=='')cart.push(+f[2]);else{cart.push(-1);timeouts++;}
    if(f[5]==='ok'&&f[7]!=='')uvi.push(+f[7]);else{uvi.push(NaN);stale++;}
  }
  return {up:Float64Array.from(up),cart:Int16Array.from(cart),uvi:Float32Array.from(uvi),syncs,timeouts,stale};
}
function epochFor(t){
  const s=hist.syncs;if(!s.length)return null;
  let best=s[0];for(const x of s){if(x[0]<=t)best=x;else break;}
  return best[1]+(t-best[0]);
}
async function loadSession(n){
  $('#hInfo').textContent='Loading session '+n+'…';
  try{
    const r=await fetch('/api/file?n='+n,{cache:'no-store'});
    if(!r.ok)throw new Error('HTTP '+r.status);
    const text=await r.text(),d=parseCsv(text);
    Object.assign(hist,d,{n,view:[0,1]});
    if(!d.up.length){$('#hBody').classList.add('hidden');$('#hInfo').textContent='Session '+n+' has no samples yet.';return;}
    $('#zFrom').value=0;$('#zTo').value=1000;
    $('#hBody').classList.remove('hidden');
    const a=$('#hDownload');a.href='/api/file?n='+n;a.download='cal_'+String(n).padStart(4,'0')+'.csv';a.removeAttribute('aria-disabled');
    const t0=d.up[0],t1=d.up[d.up.length-1],e0=epochFor(t0);
    $('#hInfo').textContent='Session '+n+': '+d.up.length.toLocaleString()+' samples over '+fmtDur(t1-t0)+(e0?', started '+fmtDateTime(e0):' (no wall-clock time recorded: open the app while recording to sync it)');
    renderHistory();
  }catch(e){$('#hInfo').textContent='Could not load session '+n+': '+e.message;toast('Could not load session '+n,true);}
}
function histRange(){
  const t0=hist.up[0],t1=hist.up[hist.up.length-1],span=t1-t0||1;
  const f=Math.min($('#zFrom').value,$('#zTo').value)/1000,t=Math.max($('#zFrom').value,$('#zTo').value)/1000;
  return [t0+span*f,t0+span*Math.max(t,f+.002)];
}
function renderHistory(){
  if(!hist.up||!hist.up.length)return;
  const [x0,x1]=histRange(),th=TABLES[tableKey].th;
  const cart=[],uv=[],sc=[];let maxU=0,sumL=0,nL=0,maxL=0,sumU=0,nU=0,maxUv=0;
  for(let i=0;i<hist.up.length;i++){
    const t=hist.up[i];if(t<x0||t>x1)continue;
    const l=level(hist.cart[i]);if(l!=null){cart.push([t,l]);sumL+=l;nL++;if(l>maxL)maxL=l;}
    const u=hist.uvi[i];if(!isNaN(u)){uv.push([t,u]);if(u>maxU)maxU=u;sumU+=u;nU++;if(u>maxUv)maxUv=u;}
    if(l!=null&&!isNaN(u))sc.push([u,l]);
  }
  const e0=epochFor(x0),labeler=e0!=null?(x=>fmtClock(e0+(x-x0))):(x=>fmtDur(x));
  const rm=niceMax(Math.max(maxU*1.1,1)),rs=rm/4,gap=medianGap(Array.from(hist.up.subarray(0,400)));
  $('#hRange').textContent=(e0!=null?fmtDateTime(e0)+' · ':'')+fmtDur(x1-x0);
  drawChart($('#hChart'),{x0,x1,xTicks:timeTicks(x0,x1,labeler),
    left:{max:140,ticks:[0,35,70,105,140],fmt:v=>v},right:{max:rm,ticks:[0,rs,rs*2,rs*3,rm],fmt:v=>v.toFixed(rm<4?1:0)},
    series:[{pts:cart,color:css('--cart'),axis:'l',gap},{pts:uv,color:css('--uv'),axis:'r',gap}]});
  $('#hStats').innerHTML=
    '<div>Samples<b>'+cart.length.toLocaleString()+'</b></div>'+
    '<div>Cartridge level (avg / max)<b>'+(nL?(sumL/nL).toFixed(1)+' / '+maxL.toFixed(0):'—')+'</b></div>'+
    '<div>UV index (avg / max)<b>'+(nU?(sumU/nU).toFixed(2)+' / '+maxUv.toFixed(2):'—')+'</b></div>'+
    '<div>Cartridge timeouts<b>'+hist.timeouts+'</b></div><div>LTR390 missing<b>'+hist.stale+'</b></div>';
  drawScatter(sc,th,rm);
}
function drawScatter(pts,th,xmax){
  const cv=$('#hScatter'),c=setupCanvas(cv);if(!c)return;
  const {g,w,h}=c,L=40,R=34,T=10,B=26,pw=w-L-R,ph=h-T-B,ink=css('--muted'),line=css('--line');
  const xm=pts.length?niceMax(Math.max.apply(null,pts.map(p=>p[0]))*1.05):xmax;
  const X=x=>L+x/xm*pw,Y=y=>T+ph-y/140*ph;
  g.clearRect(0,0,w,h);g.font='11px system-ui,sans-serif';g.fillStyle=ink;g.strokeStyle=line;
  g.textAlign='right';g.textBaseline='middle';
  [0,35,70,105,140].forEach(v=>{g.beginPath();g.moveTo(L,Y(v)+.5);g.lineTo(L+pw,Y(v)+.5);g.stroke();g.fillText(v,L-6,Y(v));});
  g.textAlign='center';g.textBaseline='top';
  const s=niceStep(xm,5);for(let v=0;v<=xm+1e-9;v+=s){g.fillText(v.toFixed(s<1?1:0),X(v),T+ph+6);}
  g.strokeRect(L+.5,T+.5,pw,ph);
  g.save();g.setLineDash([4,4]);g.strokeStyle=ink;g.textAlign='left';g.textBaseline='middle';
  th.forEach((t,i)=>{g.beginPath();g.moveTo(L,Y(t)+.5);g.lineTo(L+pw,Y(t)+.5);g.stroke();g.fillText(String(i+1),L+pw+6,Y(t));});
  g.restore();
  g.save();g.beginPath();g.rect(L,T,pw,ph);g.clip();
  g.globalAlpha=.35;g.fillStyle=css('--cart');
  for(const p of pts)g.fillRect(X(p[0])-1,Y(p[1])-1,2.5,2.5);
  g.restore();
  // average level per UV index bin
  const nb=40,sum=new Float64Array(nb),cnt=new Uint32Array(nb);
  for(const p of pts){const b=Math.min(nb-1,Math.floor(p[0]/xm*nb));sum[b]+=p[1];cnt[b]++;}
  const bins=[];for(let b=0;b<nb;b++)if(cnt[b]>=3)bins.push([(b+.5)/nb*xm,sum[b]/cnt[b]]);
  g.strokeStyle=css('--uv');g.lineWidth=2.5;g.beginPath();
  bins.forEach((p,i)=>{if(i)g.lineTo(X(p[0]),Y(p[1]));else g.moveTo(X(p[0]),Y(p[1]));});g.stroke();
  // UV index at which the average level first reaches each bar threshold
  let html='';
  if(bins.length<2)html='Not enough data in this range.';
  else{
    html='<table><tr><th>Bar</th><th class="num">Level ≥</th><th class="num">UV index ≈</th></tr>';
    th.forEach((t,i)=>{
      let uv=null;
      for(let k=1;k<bins.length;k++){
        if(bins[k-1][1]<t&&bins[k][1]>=t){const f=(t-bins[k-1][1])/((bins[k][1]-bins[k-1][1])||1);uv=bins[k-1][0]+f*(bins[k][0]-bins[k-1][0]);break;}
      }
      html+='<tr><td>'+(i+1)+'</td><td class="num">'+t+'</td><td class="num">'+(uv==null?'—':uv.toFixed(2))+'</td></tr>';
    });
    html+='</table><p class="note">Interpolated from the averaged line above. Treat it as a guide: it needs data spanning the whole range, and depends on the calibration value.</p>';
  }
  $('#hEst').innerHTML=html;
}

// ------------------------------------------------------------------ STORAGE tab
let files=[];
const ROW_BYTES=40;
function renderStorageStatus(){
  if(!status)return;
  const used=status.total-status.free,pct=status.total?Math.round(used/status.total*100):0;
  const m=$('#fsMeter');m.className='meter'+(status.storage==='full'?' bad':status.free<status.minFree*3?' warn':'');
  $('i',m).style.width=pct+'%';
  const perHour=ROW_BYTES*3600000/status.intervalMs,hrs=status.free/perHour;
  $('#fsText').innerHTML=fmtBytes(used)+' used of '+fmtBytes(status.total)+' ('+pct+'%), <b>'+fmtBytes(status.free)+'</b> free.<br>'+
    'Recording to session '+status.session+' ('+status.rows.toLocaleString()+' samples) — storage <b class="'+(status.storage==='ok'?'':'err')+'">'+status.storage+'</b>. '+
    'Roughly '+(hrs>=1?hrs.toFixed(hrs<10?1:0)+' hours':Math.round(hrs*60)+' minutes')+' left at one sample every '+(status.intervalMs/1000)+' s.';
  const sel=$('#ivSel');
  if(!sel.options.length){
    [[0,'Default ('+(status.defaultIntervalMs/1000)+' s)'],[1000,'1 s'],[2000,'2 s'],[5000,'5 s'],[10000,'10 s'],[30000,'30 s'],[60000,'1 min'],[300000,'5 min']].forEach(o=>{const e=document.createElement('option');e.value=o[0];e.textContent=o[1];sel.appendChild(e);});
    sel.value=String(status.intervalMs===status.defaultIntervalMs?0:status.intervalMs);
    if(sel.value===''||sel.selectedIndex<0)sel.value='0';
  }
  $('#ivNote').textContent='now: '+(status.intervalMs/1000)+' s';
  $('#clockNote').textContent=status.timeSynced?'clock set':'clock not set';
}
async function loadFiles(){
  try{
    const d=await api('/api/files');files=d.files.sort((a,b)=>b.n-a.n);
    renderFiles();fillSessionSelect();
  }catch(e){$('#fileList').textContent='Could not read the file list: '+e.message;}
}
function renderFiles(){
  if(!files.length){$('#fileList').textContent='No sessions on flash.';return;}
  let h='';
  files.forEach(f=>{
    const name='cal_'+String(f.n).padStart(4,'0')+'.csv';
    h+='<div class="file"><div class="fn"><b>Session '+f.n+'</b> '+(f.current?'<span class="tag rec">recording</span>':'')+
      '<div class="muted small">'+fmtBytes(f.size)+'</div></div><div class="acts">'+
      '<a class="btn small" href="#history/'+f.n+'">View</a>'+
      '<a class="btn small" href="/api/file?n='+f.n+'" download="'+name+'">Download</a>'+
      '<button class="btn small danger" data-del="'+f.n+'"'+(f.current?' disabled title="Still being written"':'')+'>Delete</button></div></div>';
  });
  $('#fileList').innerHTML=h;
  $$('[data-del]').forEach(b=>b.onclick=()=>deleteOne(+b.dataset.del));
}
function fillSessionSelect(){
  const sel=$('#hSession'),cur=sel.value;sel.innerHTML='';
  files.forEach(f=>{const o=document.createElement('option');o.value=f.n;o.textContent='Session '+f.n+' ('+fmtBytes(f.size)+')'+(f.current?' — recording':'');sel.appendChild(o);});
  if(cur&&files.some(f=>String(f.n)===cur))sel.value=cur;
}
async function deleteOne(n){
  if(!await askConfirm('Delete session '+n+'?','This permanently deletes its data from the device.',{ok:'Delete',danger:true}))return;
  try{await post('/api/delete?n='+n);toast('Deleted session '+n);}catch(e){toast(e.message,true);}
  refreshStorage();
}
async function refreshStorage(){await Promise.all([pollStatus(),loadFiles()]);}

// tar helpers (so a backup is a single file and restore can split it again)
function tarHeader(name,size,mtime){
  const h=new Uint8Array(512),enc=new TextEncoder();
  const put=(s,off,len)=>h.set(enc.encode(s).subarray(0,len),off);
  put(name,0,100);put('0000644\0',100,8);put('0000000\0',108,8);put('0000000\0',116,8);
  put(size.toString(8).padStart(11,'0')+'\0',124,12);put(Math.floor(mtime/1000).toString(8).padStart(11,'0')+'\0',136,12);
  put('        ',148,8);h[156]=48;put('ustar\0',257,6);put('00',263,2);
  let sum=0;for(let i=0;i<512;i++)sum+=h[i];
  put(sum.toString(8).padStart(6,'0')+'\0 ',148,8);
  return h;
}
function buildTar(entries){
  const parts=[];
  entries.forEach(e=>{parts.push(tarHeader(e.name,e.data.length,e.mtime||Date.now()),e.data);const pad=(512-e.data.length%512)%512;if(pad)parts.push(new Uint8Array(pad));});
  parts.push(new Uint8Array(1024));
  return new Blob(parts,{type:'application/x-tar'});
}
function parseTar(buf){
  const u=new Uint8Array(buf),dec=new TextDecoder(),out=[];let off=0;
  while(off+512<=u.length){
    if(u[off]===0)break;
    const name=dec.decode(u.subarray(off,off+100)).replace(/\0[\s\S]*$/,'');
    const size=parseInt(dec.decode(u.subarray(off+124,off+136)).replace(/\0[\s\S]*$/,'').trim(),8)||0,type=u[off+156];
    off+=512;
    if(type===48||type===0)out.push({name,data:u.subarray(off,off+size)});
    off+=Math.ceil(size/512)*512;
  }
  return out;
}
async function backupAll(){
  const st=$('#bkStatus'),btn=$('#bkAll');btn.disabled=true;
  try{
    await loadFiles();if(!files.length){st.textContent='Nothing to back up.';return;}
    const entries=[];let i=0;
    for(const f of files.slice().sort((a,b)=>a.n-b.n)){
      i++;st.textContent='Reading session '+f.n+' ('+i+' of '+files.length+')…';
      const r=await fetch('/api/file?n='+f.n,{cache:'no-store'});if(!r.ok)throw new Error('session '+f.n+': HTTP '+r.status);
      entries.push({name:'cal_'+String(f.n).padStart(4,'0')+'.csv',data:new Uint8Array(await r.arrayBuffer())});
    }
    const d=new Date(),stamp=d.getFullYear()+pad2(d.getMonth()+1)+pad2(d.getDate())+'-'+pad2(d.getHours())+pad2(d.getMinutes());
    const url=URL.createObjectURL(buildTar(entries)),a=document.createElement('a');
    a.href=url;a.download='ojo-calibrator-backup-'+stamp+'.tar';document.body.appendChild(a);a.click();a.remove();
    setTimeout(()=>URL.revokeObjectURL(url),30000);
    st.textContent='Backup of '+entries.length+' session(s) downloaded.';
  }catch(e){st.textContent='Backup failed: '+e.message;toast('Backup failed',true);}
  finally{btn.disabled=false;}
}
async function restoreFiles(list){
  const st=$('#rsStatus'),over=$('#rsOver').checked,items=[];
  st.textContent='Reading…';
  for(const f of list){
    try{
      const buf=await f.arrayBuffer();
      if(/\.tar$/i.test(f.name))parseTar(buf).forEach(e=>items.push(e));
      else items.push({name:f.name,data:new Uint8Array(buf)});
    }catch(e){items.push({name:f.name,error:e.message});}
  }
  const msgs=[];let okN=0;
  for(const it of items){
    const base=it.name.split(/[\\/]/).pop();
    if(!/^cal_\d{4,}\.csv$/.test(base)){msgs.push('Skipped '+esc(base)+' (not a session file)');continue;}
    if(it.error){msgs.push(esc(base)+': '+esc(it.error));continue;}
    st.textContent='Uploading '+base+'…';
    const fd=new FormData();fd.append('file',new Blob([it.data],{type:'text/csv'}),base);
    try{await api('/api/upload'+(over?'?overwrite=1':''),{method:'POST',body:fd},120000);msgs.push('Restored '+esc(base));okN++;}
    catch(e){msgs.push(esc(base)+': '+esc(e.message));}
  }
  st.innerHTML=(msgs.length?msgs.join('<br>'):'No session files found.');
  if(okN)toast('Restored '+okN+' session(s)');
  $('#rsFile').value='';refreshStorage();
}

// ------------------------------------------------------------------ wiring
function show(tab,arg){
  $$('nav button').forEach(b=>b.setAttribute('aria-selected',String(b.dataset.tab===tab)));
  ['live','history','storage'].forEach(t=>$('#tab-'+t).classList.toggle('hidden',t!==tab));
  if(tab==='live')renderLive();
  if(tab==='storage')refreshStorage();
  if(tab==='history'){
    loadFiles().then(()=>{
      const n=arg||$('#hSession').value;
      if(arg){$('#hSession').value=arg;}
      if(n&&(arg||!hist.up))loadSession(n);else if(hist.up)renderHistory();
    });
  }
}
function route(){
  const m=/^#(live|history|storage)(?:\/(\d+))?$/.exec(location.hash)||[null,'live'];
  show(m[1],m[2]?Number(m[2]):0);
}
$$('nav button').forEach(b=>b.onclick=()=>{location.hash='#'+b.dataset.tab;});
window.addEventListener('hashchange',route);
window.addEventListener('resize',()=>{renderLive();if(!$('#tab-history').classList.contains('hidden'))renderHistory();});

$('#liveWin').onclick=e=>{const b=e.target.closest('button');if(!b)return;live.win=Number(b.dataset.w);$$('#liveWin button').forEach(x=>x.classList.toggle('on',x===b));drawLive();};
$('#calIn').value=cal;
$('#calIn').onchange=()=>{const v=Number($('#calIn').value);if(v>=0&&v<=255){cal=Math.round(v);store.set('cal',cal);renderLive();if(hist.up)renderHistory();}else $('#calIn').value=cal;};
$('#calFromNow').onclick=()=>{const p=live.pts[live.pts.length-1];if(!p||p.cart<0){toast('No cartridge reading to use',true);return;}cal=Math.min(255,p.cart-2);store.set('cal',cal);$('#calIn').value=cal;renderLive();toast('Calibration set to '+cal);};

$('#hLoad').onclick=()=>{const n=$('#hSession').value;if(n){history.replaceState(null,'','#history/'+n);loadSession(n);}};
['zFrom','zTo'].forEach(id=>$('#'+id).oninput=()=>renderHistory());
Object.keys(TABLES).forEach(k=>{const o=document.createElement('option');o.value=k;o.textContent=TABLES[k].name+' thresholds';$('#hTable').appendChild(o);});
$('#hTable').value=tableKey;
$('#hTable').onchange=()=>{tableKey=$('#hTable').value;store.set('table',tableKey);renderHistory();};
$('#hDownload').onclick=e=>{if($('#hDownload').getAttribute('aria-disabled'))e.preventDefault();};

$('#bkAll').onclick=backupAll;
$('#rsFile').onchange=()=>{if($('#rsFile').files.length)restoreFiles(Array.from($('#rsFile').files));};
$('#ivApply').onclick=async()=>{try{await post('/api/interval?ms='+$('#ivSel').value);toast('Interval updated');}catch(e){toast(e.message,true);}pollStatus();};
$('#newSess').onclick=async()=>{if(!await askConfirm('Start a new session?','The current session is closed and recording continues in a new file.',{ok:'Start'}))return;try{await post('/api/newsession');toast('New session started');}catch(e){toast(e.message,true);}refreshStorage();};
$('#syncClock').onclick=async()=>{try{await post('/api/time?ms='+Date.now());toast('Clock set from this device');}catch(e){toast(e.message,true);}pollStatus();};
$('#delAll').onclick=async()=>{if(!await askConfirm('Delete all other sessions?','Every session except the one being recorded will be permanently deleted. Download a backup first if you need them.',{ok:'Delete all',danger:true}))return;try{const r=await post('/api/delete?all=1');toast('Deleted '+r.deleted+' session(s)');}catch(e){toast(e.message,true);}refreshStorage();};
$('#fmt').onclick=async()=>{if(!await askConfirm('Format storage?','This erases EVERYTHING on the device, including the session being recorded, and starts over.',{ok:'Erase everything',danger:true,typed:'ERASE'}))return;try{await post('/api/format?confirm=ERASE');toast('Storage formatted');}catch(e){toast(e.message,true);}live.pts=[];refreshStorage();};
$('#hostHint').textContent='http://'+location.host;

buildGauges();route();pollStatus();pollLive();
setInterval(pollLive,1500);
setInterval(()=>{pollStatus();if(!$('#tab-storage').classList.contains('hidden'))loadFiles();},6000);
})();
</script>
</body>
</html>
)HTML";

#endif
