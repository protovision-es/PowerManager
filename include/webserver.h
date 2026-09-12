#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <ESPAsyncWebServer.h>
#include "config.h"
#include "storage.h"
#include "relay.h"
#include "wol.h"
#include "scheduler.h"
#include "ping.h"
#include "log.h"

static AsyncWebServer server(WEB_PORT);

static const char API_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="color-scheme" content="dark light">
<title>PowerManager - API</title>
<script>(function(){try{var t=localStorage.getItem('pm-theme');if(!t){t=matchMedia('(prefers-color-scheme: light)').matches?'light':'dark'}document.documentElement.setAttribute('data-theme',t)}catch(e){}})();</script>
<style>
*{margin:0;padding:0;box-sizing:border-box}
:root{
  --bg:#0b0d13;--surface:#161923;--surface2:#1e2230;
  --border:#2a2f42;--border2:#3a4058;
  --accent:#6366f1;--accent2:#818cf8;--accent-glow:rgba(99,102,241,.18);
  --on:#22c55e;--text:#eceef2;--text2:#9aa0b4;--text3:#666c82;
  --tag-get:#22c55e;--topbar-bg:rgba(11,13,19,.9);
  --shadow:0 8px 32px rgba(0,0,0,.45);
  color-scheme:dark;
}
:root[data-theme="light"]{
  --bg:#f4f5f9;--surface:#ffffff;--surface2:#eef0f6;
  --border:#e1e3ec;--border2:#cdd0e0;
  --accent:#5457e8;--accent2:#4547cf;--accent-glow:rgba(84,87,232,.12);
  --on:#15803d;--text:#181a24;--text2:#585e70;--text3:#868ca0;
  --tag-get:#15803d;--topbar-bg:rgba(244,245,249,.88);
  --shadow:0 8px 28px rgba(30,34,60,.1);
  color-scheme:light;
}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:var(--bg);color:var(--text);min-height:100vh;line-height:1.5;transition:background .25s,color .25s}
svg{display:block}
.icon{width:1em;height:1em;flex-shrink:0}
:focus-visible{outline:2px solid var(--accent);outline-offset:2px;border-radius:4px}
.topbar{position:sticky;top:0;z-index:50;background:var(--topbar-bg);backdrop-filter:blur(16px);-webkit-backdrop-filter:blur(16px);border-bottom:1px solid var(--border);padding:0 32px;height:56px;display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap}
.topbar .brand{display:flex;align-items:center;gap:12px}
.topbar .brand .logo{width:32px;height:32px;border-radius:8px;background:linear-gradient(135deg,var(--accent),#a78bfa);display:flex;align-items:center;justify-content:center;color:#fff}
.topbar .brand .logo .icon{width:16px;height:16px}
.topbar .brand h1{font-size:1rem;font-weight:700}
.topbar .nav{display:flex;gap:14px;align-items:center}
.topbar .nav a{color:var(--text2);text-decoration:none;font-size:.82rem;font-weight:600;transition:color .15s;display:flex;align-items:center;gap:6px}
.topbar .nav a:hover{color:var(--accent)}
.topbar .nav a .icon{width:14px;height:14px}
.iconbtn{width:32px;height:32px;border-radius:8px;border:1px solid var(--border);background:var(--surface);color:var(--text2);display:flex;align-items:center;justify-content:center;cursor:pointer;transition:all .15s}
.iconbtn:hover{border-color:var(--accent);color:var(--accent);background:var(--accent-glow)}
.iconbtn .icon{width:16px;height:16px}
.main{max-width:900px;margin:0 auto;padding:32px 24px 64px}
h2{font-size:.7rem;font-weight:700;text-transform:uppercase;letter-spacing:1.5px;color:var(--text3);margin:32px 0 14px;display:flex;align-items:center;gap:8px}
h2 .bar{width:3px;height:12px;border-radius:2px;background:var(--accent)}
.info-box{background:var(--surface);border:1px solid var(--border);border-radius:12px;padding:16px 20px;margin-bottom:24px;font-size:.82rem;color:var(--text2);line-height:1.7}
.info-box code{background:var(--surface2);padding:2px 8px;border-radius:4px;font-family:'SF Mono',Consolas,monospace;font-size:.78rem;color:var(--accent)}
.ep{background:var(--surface);border:1px solid var(--border);border-radius:12px;padding:16px 20px;margin-bottom:10px;transition:border-color .15s}
.ep:hover{border-color:var(--border2)}
.ep-top{display:flex;align-items:center;gap:12px;margin-bottom:8px;flex-wrap:wrap}
.ep-tag{padding:3px 10px;border-radius:6px;font-size:.65rem;font-weight:700;letter-spacing:.5px;font-family:'SF Mono',Consolas,monospace}
.ep-tag.get{background:rgba(34,197,94,.12);color:var(--tag-get);border:1px solid rgba(34,197,94,.25)}
.ep-tag.html{background:var(--accent-glow);color:var(--accent);border:1px solid var(--accent)}
.ep-desc{font-size:.82rem;color:var(--text2)}
.ep-url-wrap{display:flex;align-items:center;gap:8px;background:var(--surface2);border:1px solid var(--border);border-radius:8px;padding:8px 12px;margin-top:4px}
.ep-url{flex:1;font-family:'SF Mono',Consolas,monospace;font-size:.78rem;color:var(--text);word-break:break-all;user-select:all}
.ep-url .who{color:var(--text3)}
.ep-copy{background:var(--accent);color:#fff;border:none;border-radius:6px;padding:6px 14px;font-size:.7rem;font-weight:600;cursor:pointer;transition:all .15s;white-space:nowrap;font-family:inherit;flex-shrink:0}
.ep-copy:hover{background:var(--accent2)}
.ep-copy.copied{background:var(--on)}
.ep-params{margin-top:8px;font-size:.75rem;color:var(--text3)}
.ep-params code{color:var(--text2);background:var(--surface2);padding:1px 6px;border-radius:3px;font-family:'SF Mono',Consolas,monospace;font-size:.72rem}
.ep-example{margin-top:8px;background:var(--surface2);border:1px solid var(--border);border-radius:6px;padding:10px 14px;font-family:'SF Mono',Consolas,monospace;font-size:.72rem;color:var(--text2);white-space:pre-wrap;overflow-x:auto;line-height:1.6}
.ep-note{margin-top:8px;font-size:.75rem;color:var(--text3);line-height:1.5}
#toastWrap{position:fixed;bottom:24px;left:50%;transform:translateX(-50%);display:flex;flex-direction:column-reverse;gap:8px;z-index:200;align-items:center;pointer-events:none}
.toast{background:var(--surface2);color:var(--text);padding:10px 20px;border-radius:10px;font-size:.8rem;border:1px solid var(--border2);border-left:3px solid var(--accent);box-shadow:var(--shadow);font-weight:500;opacity:0;transform:translateY(12px);transition:opacity .25s,transform .25s;white-space:nowrap}
.toast.show{opacity:1;transform:translateY(0)}
.toast.ok{border-left-color:var(--on)}
</style>
</head>
<body>
<div class="topbar">
<div class="brand"><div class="logo"><svg class="icon" viewBox="0 0 24 24" fill="currentColor"><path d="M13 2 3 14h7l-1 8 10-12h-7l1-8Z"/></svg></div><h1>API Reference</h1></div>
<div class="nav">
<button class="iconbtn" id="themeBtn" title="Toggle theme" aria-label="Toggle theme"></button>
<a href="/"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M19 12H5M12 19l-7-7 7-7"/></svg>Dashboard</a>
</div>
</div>
<div class="main">
<div class="info-box">
<strong>Base URL:</strong> <code id="baseurl">http://...</code><br>
All endpoints are <code>GET</code>, designed to be called from a browser, a shortcut, an NFC/RFID tag, or a script — no authentication required.<br>
The ones returning <code>JSON</code> are for integrations; the ones marked <code>HTML</code> open a confirmation page meant for humans (and trigger the action as soon as they load).
</div>

<h2><span class="bar"></span>System status</h2>
<div class="ep">
<div class="ep-top"><span class="ep-tag get">JSON</span><span class="ep-desc">Full status: relays, WoL, ping, and the device clock.</span></div>
<div class="ep-url-wrap"><span class="ep-url" id="u-status"></span><button class="ep-copy" data-copy="u-status">Copy</button></div>
<div class="ep-example" id="ex-status">{
  "uptime": 3661,
  "datetime": "Mon 29 Jul 2026  14:30:00",
  "ip": "10.0.0.93",
  "wifi_rssi": -55,
  "relays": [{"id":0,"name":"Router","pin":16,"restartSec":10,"on":true,"restarting":false,"remaining":0}],
  "wol": [{"id":0,"name":"Office PC","mac":"F4:4D:30:18:58:95"}],
  "ping": {"host":"8.8.8.8","interval":300,"threshold":3,"relayTrigger":0,"failCount":0,"lastResult":true,"ms":24}
}</div>
</div>

<h2><span class="bar"></span>Relays</h2>
<div class="ep">
<div class="ep-top"><span class="ep-tag html">HTML</span><span class="ep-desc">Restarts a relay (turns it off for X seconds and back on) and shows a live countdown confirmation screen.</span></div>
<div id="restart-urls"></div>
<div class="ep-params">Parameter: <code>id</code> (0-3)</div>
<div class="ep-note">The action runs as soon as the URL is requested, without waiting for the page to render — it works whether opened in a browser or called with <code>fetch</code>/<code>curl</code>.</div>
</div>

<h2><span class="bar"></span>Wake on LAN</h2>
<div class="ep">
<div class="ep-top"><span class="ep-tag html">HTML</span><span class="ep-desc">Sends a Wake-on-LAN magic packet to power on a remote device.</span></div>
<div id="wol-urls"></div>
<div class="ep-params">Parameter: <code>id</code> (0-4)</div>
</div>

<h2><span class="bar"></span>Scheduled restarts</h2>
<div class="ep">
<div class="ep-top"><span class="ep-tag get">JSON</span><span class="ep-desc">List of configured schedules and the device's current time.</span></div>
<div class="ep-url-wrap"><span class="ep-url" id="u-schedule"></span><button class="ep-copy" data-copy="u-schedule">Copy</button></div>
<div class="ep-example">{
  "time": "14:30",
  "schedules": [{"id":0,"relay":0,"relayName":"Router","hour":4,"min":0,"enabled":true}]
}</div>
</div>

<h2><span class="bar"></span>Event log</h2>
<div class="ep">
<div class="ep-top"><span class="ep-tag get">JSON</span><span class="ep-desc">Latest system events (max 50, oldest first).</span></div>
<div class="ep-url-wrap"><span class="ep-url" id="u-log"></span><button class="ep-copy" data-copy="u-log">Copy</button></div>
<div class="ep-example">[
  {"t":"29/07 14:30:12","m":"Relay Router restarting (OFF 10s)"},
  {"t":"29/07 14:30:22","m":"Relay Router ON (restart complete)"}
]</div>
</div>
</div>
<div id="toastWrap"></div>
<script>
(function(){
'use strict';
function $(id){return document.getElementById(id)}

function currentTheme(){return document.documentElement.getAttribute('data-theme')||'dark'}
function themeIcon(){
  return currentTheme()==='light'
    ? '<svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 12.8A9 9 0 1 1 11.2 3 7 7 0 0 0 21 12.8Z"/></svg>'
    : '<svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4"/></svg>';
}
$('themeBtn').innerHTML=themeIcon();
$('themeBtn').addEventListener('click',function(){
  var next=currentTheme()==='light'?'dark':'light';
  document.documentElement.setAttribute('data-theme',next);
  try{localStorage.setItem('pm-theme',next)}catch(e){}
  $('themeBtn').innerHTML=themeIcon();
});

function toast(msg,kind){
  var wrap=$('toastWrap');
  var t=document.createElement('div');
  t.className='toast'+(kind?' '+kind:'');
  t.textContent=msg;
  wrap.appendChild(t);
  requestAnimationFrame(function(){t.classList.add('show')});
  setTimeout(function(){t.classList.remove('show');setTimeout(function(){t.remove()},300)},2000);
}

function copyText(text){
  if(navigator.clipboard&&window.isSecureContext){
    return navigator.clipboard.writeText(text);
  }
  return new Promise(function(resolve,reject){
    var ta=document.createElement('textarea');
    ta.value=text;
    ta.style.cssText='position:fixed;left:-9999px';
    document.body.appendChild(ta);
    ta.select();
    var ok=false;
    try{ok=document.execCommand('copy')}catch(e){}
    document.body.removeChild(ta);
    ok?resolve():reject(new Error('copy failed'));
  });
}

document.addEventListener('click',function(e){
  var btn=e.target.closest('[data-copy]');
  if(!btn)return;
  var el=$(btn.dataset.copy);
  if(!el)return;
  var url=el.getAttribute('data-url')||el.textContent;
  copyText(url).then(function(){
    toast('Copied!','ok');
    btn.classList.add('copied');
    setTimeout(function(){btn.classList.remove('copied')},1200);
  }).catch(function(){toast('Copy failed')});
});

function esc(s){return String(s==null?'':s).replace(/[&<>"']/g,function(c){return({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'})[c]})}

async function init(){
  var base='http://'+location.host;
  $('baseurl').textContent=base;
  $('u-status').textContent=base+'/api/status';
  $('u-status').setAttribute('data-url',base+'/api/status');
  $('u-schedule').textContent=base+'/api/schedule';
  $('u-schedule').setAttribute('data-url',base+'/api/schedule');
  $('u-log').textContent=base+'/api/log';
  $('u-log').setAttribute('data-url',base+'/api/log');

  var restartWrap=$('restart-urls');
  var relayNames=[null,null,null,null];
  try{
    var r=await fetch(base+'/api/status');
    var d=await r.json();
    (d.relays||[]).forEach(function(rl){relayNames[rl.id]=rl.name});
  }catch(e){/* offline: URLs are still shown, without a name */}

  restartWrap.innerHTML='';
  for(var i=0;i<4;i++){
    var u=base+'/api/relays/restart?id='+i;
    var label=relayNames[i]?('  <span class="who">&larr; '+esc(relayNames[i])+'</span>'):'';
    restartWrap.innerHTML+='<div class="ep-url-wrap"><span class="ep-url" id="u-restart'+i+'" data-url="'+u+'">'+u+label+'</span><button class="ep-copy" data-copy="u-restart'+i+'">Copy</button></div>';
  }

  var wolWrap=$('wol-urls');
  try{
    var r2=await fetch(base+'/api/status');
    var d2=await r2.json();
    var wol=d2.wol||[];
    if(wol.length){
      wolWrap.innerHTML=wol.map(function(w){
        var u=base+'/api/wol/wake?id='+w.id;
        return '<div class="ep-url-wrap"><span class="ep-url" id="u-wol'+w.id+'" data-url="'+u+'">'+u+'  <span class="who">&larr; '+esc(w.name)+'</span></span><button class="ep-copy" data-copy="u-wol'+w.id+'">Copy</button></div>';
      }).join('');
    }else{
      wolWrap.innerHTML='<div class="ep-params">No WoL devices configured</div>';
    }
  }catch(e){
    wolWrap.innerHTML='<div class="ep-params">Could not load configuration (no connection to the device)</div>';
  }
}
init();
})();
</script>
</body>
</html>
)rawliteral";

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="color-scheme" content="dark light">
<title>PowerManager</title>
<script>(function(){try{var t=localStorage.getItem('pm-theme');if(!t){t=matchMedia('(prefers-color-scheme: light)').matches?'light':'dark'}document.documentElement.setAttribute('data-theme',t)}catch(e){}})();</script>
<style>
*{margin:0;padding:0;box-sizing:border-box}
:root{
  --bg:#0b0d13;--surface:#161923;--surface2:#1e2230;--surface3:#272c3d;
  --border:#2a2f42;--border2:#3a4058;
  --accent:#6366f1;--accent2:#818cf8;--accent-glow:rgba(99,102,241,.18);
  --on:#22c55e;--on-glow:rgba(34,197,94,.14);--on-border:rgba(34,197,94,.32);
  --restart:#f59e0b;--restart-glow:rgba(245,158,11,.14);--restart-border:rgba(245,158,11,.32);
  --off:#ef4444;--off-glow:rgba(239,68,68,.12);--off-border:rgba(239,68,68,.32);
  --text:#eceef2;--text2:#9aa0b4;--text3:#666c82;
  --shadow:0 8px 32px rgba(0,0,0,.45);
  --backdrop:rgba(5,6,10,.6);
  --topbar-bg:rgba(11,13,19,.9);
  color-scheme:dark;
}
:root[data-theme="light"]{
  --bg:#f4f5f9;--surface:#ffffff;--surface2:#eef0f6;--surface3:#e4e7f0;
  --border:#e1e3ec;--border2:#cdd0e0;
  --accent:#5457e8;--accent2:#4547cf;--accent-glow:rgba(84,87,232,.12);
  --on:#15803d;--on-glow:rgba(21,128,61,.1);--on-border:rgba(21,128,61,.28);
  --restart:#c2660a;--restart-glow:rgba(194,102,10,.1);--restart-border:rgba(194,102,10,.28);
  --off:#dc2626;--off-glow:rgba(220,38,38,.1);--off-border:rgba(220,38,38,.28);
  --text:#181a24;--text2:#585e70;--text3:#868ca0;
  --shadow:0 8px 28px rgba(30,34,60,.1);
  --backdrop:rgba(30,32,44,.4);
  --topbar-bg:rgba(244,245,249,.88);
  color-scheme:light;
}
html{-webkit-tap-highlight-color:transparent}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:var(--bg);color:var(--text);min-height:100vh;line-height:1.5;transition:background .25s,color .25s}
svg{display:block}
.icon{width:1em;height:1em;flex-shrink:0}
button{font-family:inherit}
:focus-visible{outline:2px solid var(--accent);outline-offset:2px;border-radius:4px}

/* topbar */
.topbar{position:sticky;top:0;z-index:50;background:var(--topbar-bg);backdrop-filter:blur(16px);-webkit-backdrop-filter:blur(16px);border-bottom:1px solid var(--border);padding:10px 32px;display:flex;flex-wrap:wrap;row-gap:8px;align-items:center;justify-content:space-between;gap:16px}
.brand{display:flex;align-items:center;gap:12px}
.brand .logo{width:34px;height:34px;border-radius:9px;background:linear-gradient(135deg,var(--accent),#a78bfa);display:flex;align-items:center;justify-content:center;color:#fff;flex-shrink:0}
.brand .logo .icon{width:18px;height:18px}
.brand h1{font-size:1rem;font-weight:700;letter-spacing:-.3px;line-height:1.15}
.brand .ip-sub{font-size:.68rem;color:var(--text3);font-family:'SF Mono',Consolas,monospace}
.status-bar{display:flex;align-items:center;gap:14px;font-size:.78rem;color:var(--text2);flex-wrap:wrap;row-gap:8px}
.status-bar .sep{width:1px;height:18px;background:var(--border)}
.pill{display:flex;align-items:center;gap:6px;white-space:nowrap}
.pill .icon{width:14px;height:14px;color:var(--text3)}
.dot{width:7px;height:7px;border-radius:50%;background:var(--on);box-shadow:0 0 8px var(--on);flex-shrink:0}
.dot.live{animation:livepulse 2s ease infinite}
.dot.bad{background:var(--off);box-shadow:0 0 8px var(--off)}
@keyframes livepulse{0%,100%{opacity:1}50%{opacity:.45}}
@keyframes spin{to{transform:rotate(360deg)}}
@keyframes fadeUp{from{opacity:0;transform:translateY(10px)}to{opacity:1;transform:translateY(0)}}
@keyframes flashIn{from{background:var(--accent-glow)}to{background:transparent}}
.iconbtn{width:32px;height:32px;border-radius:8px;border:1px solid var(--border);background:var(--surface);color:var(--text2);display:flex;align-items:center;justify-content:center;cursor:pointer;transition:all .15s;flex-shrink:0}
.iconbtn:hover{border-color:var(--accent);color:var(--accent);background:var(--accent-glow)}
.iconbtn .icon{width:16px;height:16px}
.iconbtn.spinning .icon{animation:spin .7s linear infinite}
.docs-link{display:flex;align-items:center;gap:6px;color:var(--accent);text-decoration:none;font-size:.78rem;font-weight:600}
.docs-link:hover{text-decoration:underline}
.docs-link .icon{width:14px;height:14px}
.wifi-bars{display:flex;align-items:flex-end;gap:2px;height:12px}
.wifi-bars i{width:3px;display:block;background:var(--border2);border-radius:1px}
.wifi-bars i:nth-child(1){height:35%}
.wifi-bars i:nth-child(2){height:55%}
.wifi-bars i:nth-child(3){height:75%}
.wifi-bars i:nth-child(4){height:100%}
.wifi-bars i.on{background:var(--on)}

/* offline banner */
#offlineBanner{display:none;align-items:center;gap:10px;background:var(--off-glow);border-bottom:1px solid var(--off-border);color:var(--off);padding:8px 32px;font-size:.8rem;font-weight:600}
#offlineBanner.show{display:flex}
#offlineBanner .icon{width:15px;height:15px;flex-shrink:0}
#offlineBanner button{margin-left:auto;background:transparent;border:1px solid var(--off-border);color:var(--off);border-radius:8px;padding:4px 12px;font-size:.72rem;font-weight:700;cursor:pointer}
#offlineBanner button:hover{background:var(--off-glow)}

.main{max-width:1280px;margin:0 auto;padding:24px 32px 64px}
.section{margin-bottom:30px;animation:fadeUp .4s ease both}
.section-title{font-size:.7rem;font-weight:700;text-transform:uppercase;letter-spacing:1.5px;color:var(--text3);margin-bottom:14px;display:flex;align-items:center;gap:8px}
.section-title .icon{width:13px;height:13px;color:var(--accent)}
.grid-relays{display:grid;grid-template-columns:repeat(4,1fr);gap:16px}
@media(max-width:1100px){.grid-relays{grid-template-columns:repeat(2,1fr)}}
@media(max-width:600px){.grid-relays{grid-template-columns:1fr}.main{padding:16px 14px 48px}.topbar{padding:10px 16px}#offlineBanner{padding:8px 16px}}
.card{background:var(--surface);border:1px solid var(--border);border-radius:14px;padding:20px;transition:border-color .2s,box-shadow .2s,background .25s}
.card:hover{border-color:var(--border2)}

/* relay card */
.r-card{display:flex;flex-direction:column;align-items:center;text-align:center;padding:26px 16px 20px;position:relative}
.r-card .ring-wrap{width:76px;height:76px;position:relative;margin-bottom:12px;flex-shrink:0}
.r-card .ring-wrap svg{width:100%;height:100%;transform:rotate(-90deg)}
.r-card .ring-track{fill:none;stroke:var(--surface3);stroke-width:5}
.r-card .ring-prog{fill:none;stroke:var(--restart);stroke-width:5;stroke-linecap:round;transition:stroke-dashoffset .9s linear}
.r-card .ring-center{position:absolute;inset:0;display:flex;align-items:center;justify-content:center;color:var(--on)}
.r-card .ring-center .icon{width:26px;height:26px}
.r-card.restarting .ring-center{color:var(--restart)}
.r-card .rname{font-size:.92rem;font-weight:600;margin-bottom:2px}
.r-card .rconf{font-size:.68rem;color:var(--text3);margin-bottom:2px;font-family:'SF Mono',Consolas,monospace}
.r-card .rgpio{font-size:.64rem;color:var(--text3);margin-bottom:12px}
.r-card .badge{display:inline-flex;align-items:center;gap:5px;padding:4px 12px;border-radius:20px;font-size:.66rem;font-weight:700;letter-spacing:.3px;margin-bottom:14px}
.r-card .badge .icon{width:10px;height:10px}
.r-card .badge.ok{background:var(--on-glow);color:var(--on);border:1px solid var(--on-border)}
.r-card .badge.restarting{background:var(--restart-glow);color:var(--restart);border:1px solid var(--restart-border)}
.r-card .countdown{font-size:1.7rem;font-weight:700;font-variant-numeric:tabular-nums;color:var(--restart);line-height:1;margin-bottom:2px}
.r-card .clbl{font-size:.6rem;color:var(--text3);text-transform:uppercase;letter-spacing:1px;margin-bottom:14px}

.btn{display:inline-flex;align-items:center;justify-content:center;gap:6px;border:none;border-radius:10px;padding:9px 20px;font-size:.75rem;font-weight:600;cursor:pointer;transition:all .15s;letter-spacing:.2px;font-family:inherit}
.btn .icon{width:13px;height:13px}
.btn:active{transform:scale(.96)}
.btn:disabled{opacity:.5;cursor:not-allowed;transform:none}
.btn-primary{background:var(--accent);color:#fff}
.btn-primary:hover:not(:disabled){background:var(--accent2);box-shadow:0 0 20px var(--accent-glow)}
.btn-warn{background:linear-gradient(135deg,#f59e0b,#ef4444);color:#fff}
.btn-warn:hover:not(:disabled){box-shadow:0 0 20px var(--restart-glow)}
.btn-ghost{background:transparent;color:var(--accent);border:1px solid var(--border2);padding:7px 14px;font-size:.7rem}
.btn-ghost:hover{background:var(--accent-glow);border-color:var(--accent)}

.grid-bottom{display:grid;grid-template-columns:1fr 1fr;gap:16px}
@media(max-width:800px){.grid-bottom{grid-template-columns:1fr}}
.card .card-head{display:flex;align-items:center;gap:8px;margin-bottom:16px;font-size:.75rem;font-weight:700;color:var(--text2)}
.card .card-head .icon{width:14px;height:14px}
.card .card-head .ch-dot{width:8px;height:8px;border-radius:50%}
.wol-row{display:flex;align-items:center;justify-content:space-between;gap:10px;padding:10px 14px;min-height:56px;border-radius:10px;background:var(--surface2);margin-bottom:6px;border:1px solid var(--border);transition:border-color .15s}
.wol-row:hover{border-color:var(--accent)}
.wol-row .wol-name{font-size:.82rem;font-weight:600}
.wol-row .wol-mac{font-size:.66rem;color:var(--text3);font-family:'SF Mono',Consolas,monospace}
.sch-row{display:flex;align-items:center;gap:10px;padding:10px 14px;min-height:56px;border-radius:10px;background:var(--surface2);margin-bottom:6px;border:1px solid var(--border);font-size:.78rem;position:relative}
.sch-row.next{border-color:var(--accent)}
.sch-row .sch-relay{padding:2px 10px;border-radius:6px;font-size:.64rem;font-weight:700;white-space:nowrap}
.sch-row .sch-time{flex:1;font-family:'SF Mono',Consolas,monospace;font-weight:700}
.sch-row .sch-next-tag{font-size:.6rem;font-weight:700;color:var(--accent);background:var(--accent-glow);border:1px solid var(--accent);padding:2px 8px;border-radius:10px;letter-spacing:.3px}
.sch-row .sch-status{font-size:.6rem;padding:3px 10px;border-radius:20px;font-weight:700}
.sch-row .sch-status.on{background:var(--on-glow);color:var(--on);border:1px solid var(--on-border)}
.sch-row .sch-status.off{background:var(--surface3);color:var(--text3);border:1px solid var(--border)}
.empty-msg{color:var(--text3);font-size:.78rem;text-align:center;padding:24px;font-style:italic}
.skel{background:linear-gradient(90deg,var(--surface2) 25%,var(--surface3) 50%,var(--surface2) 75%);background-size:200% 100%;animation:shimmer 1.4s ease infinite;border-radius:8px}
@keyframes shimmer{0%{background-position:200% 0}100%{background-position:-200% 0}}

.ping-row{display:flex;align-items:center;gap:22px;flex-wrap:wrap;width:100%}
.ping-indicator{width:60px;height:60px;border-radius:16px;display:flex;align-items:center;justify-content:center;border:2px solid var(--on-border);background:var(--on-glow);color:var(--on);flex-shrink:0;transition:all .3s}
.ping-indicator .icon{width:24px;height:24px}
.ping-indicator.fail{border-color:var(--off-border);background:var(--off-glow);color:var(--off)}
.ping-metrics{display:flex;gap:26px;align-items:center;flex-wrap:wrap;flex:1;justify-content:space-between}
.ping-metric .pm-val{font-size:1.35rem;font-weight:700;font-variant-numeric:tabular-nums;line-height:1}
.ping-metric .pm-val.fast{color:var(--on)}
.ping-metric .pm-val.mid{color:var(--restart)}
.ping-metric .pm-val.slow{color:var(--off)}
.ping-metric .pm-lbl{font-size:.6rem;color:var(--text3);text-transform:uppercase;letter-spacing:1px;margin-top:4px}
.ping-desc{margin-top:14px;font-size:.72rem;color:var(--text3);line-height:1.6}
.ping-desc strong{color:var(--text)}

.log-wrap{max-height:320px;overflow-y:auto;scrollbar-width:thin;scrollbar-color:var(--border2) transparent}
.log-item{display:flex;gap:12px;padding:8px 12px;border-bottom:1px solid var(--border);font-size:.78rem;align-items:baseline;border-radius:6px}
.log-item:last-child{border-bottom:none}
.log-item.new{animation:flashIn 1.6s ease}
.log-item .log-t{color:var(--accent);font-family:'SF Mono',Consolas,monospace;font-size:.7rem;font-weight:600;white-space:nowrap;flex-shrink:0}
.log-item .log-m{color:var(--text2);line-height:1.4}

#toastWrap{position:fixed;bottom:24px;left:50%;transform:translateX(-50%);display:flex;flex-direction:column-reverse;gap:8px;z-index:200;align-items:center;pointer-events:none}
.toast{background:var(--surface2);color:var(--text);padding:10px 20px;border-radius:10px;font-size:.8rem;border:1px solid var(--border2);border-left:3px solid var(--accent);box-shadow:var(--shadow);font-weight:500;opacity:0;transform:translateY(12px);transition:opacity .25s,transform .25s;white-space:nowrap}
.toast.show{opacity:1;transform:translateY(0)}
.toast.ok{border-left-color:var(--on)}
.toast.err{border-left-color:var(--off)}

.modal-backdrop{position:fixed;inset:0;background:var(--backdrop);display:flex;align-items:center;justify-content:center;z-index:300;opacity:0;pointer-events:none;transition:opacity .2s;padding:20px}
.modal-backdrop.show{opacity:1;pointer-events:auto}
.modal{background:var(--surface);border:1px solid var(--border);border-radius:16px;padding:28px 24px;max-width:340px;width:100%;text-align:center;transform:scale(.94);transition:transform .2s;box-shadow:var(--shadow)}
.modal-backdrop.show .modal{transform:scale(1)}
.modal .modal-icon{width:52px;height:52px;border-radius:14px;background:var(--restart-glow);color:var(--restart);border:1px solid var(--restart-border);display:flex;align-items:center;justify-content:center;margin:0 auto 16px}
.modal .modal-icon .icon{width:24px;height:24px}
.modal h3{font-size:1rem;font-weight:700;margin-bottom:8px}
.modal p{font-size:.82rem;color:var(--text2);line-height:1.5;margin-bottom:20px}
.modal .modal-actions{display:flex;gap:10px;justify-content:center}
.modal .modal-actions .btn{flex:1}
</style>
</head>
<body>
<div class="topbar">
<div class="brand">
<div class="logo"><svg class="icon" viewBox="0 0 24 24" fill="currentColor"><path d="M13 2 3 14h7l-1 8 10-12h-7l1-8Z"/></svg></div>
<div><h1>PowerManager</h1><div class="ip-sub" id="ipsub">connecting…</div></div>
</div>
<div class="status-bar">
<button class="iconbtn" id="themeBtn" title="Toggle theme" aria-label="Toggle theme"></button>
<a class="docs-link" href="/api-doc"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2Z"/></svg><span>API</span></a>
<span class="sep"></span>
<span class="pill" id="connpill" role="status" aria-live="polite"><span class="dot live" id="conndot"></span><span id="connlabel">Live</span></span>
<span class="sep"></span>
<span class="pill" title="Device time"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 3"/></svg><span id="datetime">--</span></span>
<span class="sep"></span>
<span class="pill" title="Uptime"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 22a10 10 0 1 0 0-20 10 10 0 0 0 0 20Z"/><path d="M12 6v6l4 2"/></svg><span id="uptime">0s</span></span>
<span class="sep"></span>
<span class="pill" id="wifipill" title="WiFi signal"><span class="wifi-bars" id="wifibars"><i></i><i></i><i></i><i></i></span><span id="wifipct">--</span></span>
<button class="iconbtn" id="refreshBtn" title="Refresh now" aria-label="Refresh now"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 12a9 9 0 1 1-3-6.7"/><path d="M21 3v6h-6"/></svg></button>
</div>
</div>
<div id="offlineBanner"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 9v4M12 17h.01M10.3 3.9 1.8 18a2 2 0 0 0 1.7 3h17a2 2 0 0 0 1.7-3L13.7 3.9a2 2 0 0 0-3.4 0Z"/></svg><span>Lost connection to the device. Retrying…</span><button id="retryBtn">Retry now</button></div>

<div class="main">
<div class="section"><div class="section-title"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M13 2 3 14h7l-1 8 10-12h-7l1-8Z"/></svg>Relays</div><div class="grid-relays" id="relays">
<div class="card r-card"><div class="skel" style="width:76px;height:76px;border-radius:50%;margin-bottom:12px"></div><div class="skel" style="width:70%;height:14px;margin-bottom:8px"></div><div class="skel" style="width:50%;height:10px;margin-bottom:16px"></div><div class="skel" style="width:100%;height:32px;border-radius:10px"></div></div>
</div></div>

<div class="section"><div class="section-title"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 2 2 7l10 5 10-5-10-5Z"/><path d="M2 17l10 5 10-5M2 12l10 5 10-5"/></svg>Operations</div>
<div class="grid-bottom">
<div class="card"><div class="card-head"><span class="ch-dot" style="background:var(--accent)"></span>Wake on LAN</div><div id="wollist"></div></div>
<div class="card"><div class="card-head"><span class="ch-dot" style="background:var(--restart)"></span>Scheduled restarts</div><div id="schedlist"></div></div>
</div></div>

<div class="section"><div class="section-title"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 12h4l2 8 4-16 2 8h4"/></svg>Monitoring</div>
<div class="card">
<div class="ping-row">
<div class="ping-indicator" id="pingcircle"><svg class="icon" id="pingicon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M20 6 9 17l-5-5"/></svg></div>
<div class="ping-metrics">
<div class="ping-metric"><div class="pm-val fast" id="pingms">--</div><div class="pm-lbl">Latency</div></div>
<div class="ping-metric"><div class="pm-val" id="pingfail">0</div><div class="pm-lbl">Failures</div></div>
</div>
</div>
<div class="ping-desc">Host: <strong id="pinghost">-</strong> &mdash; if it fails to respond <strong id="pingthr">3</strong> times in a row, it restarts: <strong id="pingrelayname">-</strong></div>
</div>
</div>

<div class="section"><div class="section-title"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2Z"/></svg>Log</div>
<div class="card"><div class="log-wrap" id="loglist"><div class="empty-msg">No events</div></div></div>
</div>
</div>

<div class="modal-backdrop" id="modalBackdrop">
<div class="modal" role="dialog" aria-modal="true" aria-labelledby="modalTitle">
<div class="modal-icon"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 12a9 9 0 1 1-3-6.7"/><path d="M21 3v6h-6"/></svg></div>
<h3 id="modalTitle">Restart</h3>
<p id="modalMsg">Confirm?</p>
<div class="modal-actions">
<button class="btn btn-ghost" id="modalCancel">Cancel</button>
<button class="btn btn-warn" id="modalConfirm"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 12a9 9 0 1 1-3-6.7"/><path d="M21 3v6h-6"/></svg>Restart</button>
</div>
</div>
</div>

<div id="toastWrap"></div>

<script>
(function(){
'use strict';

var ICO={
  bolt:'<svg class="icon" viewBox="0 0 24 24" fill="currentColor"><path d="M13 2 3 14h7l-1 8 10-12h-7l1-8Z"/></svg>',
  check:'<svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M20 6 9 17l-5-5"/></svg>',
  x:'<svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M18 6 6 18M6 6l12 12"/></svg>',
  sun:'<svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4"/></svg>',
  moon:'<svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 12.8A9 9 0 1 1 11.2 3 7 7 0 0 0 21 12.8Z"/></svg>'
};

var S={};
var restartMap={};        // id -> {remaining, total}
var consecutiveFails=0;
var pollDelay=3000;
var pollTimer=null;
var lastLogKey='';
var fetching=false;
var confirmResolve=null;

/* ---------- helpers ---------- */
function $(id){return document.getElementById(id)}
function esc(s){return String(s==null?'':s).replace(/[&<>"']/g,function(c){return({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'})[c]})}
var RC=['#6366f1','#22c55e','#ec4899','#f59e0b'];

function toast(msg,kind){
  var wrap=$('toastWrap');
  var t=document.createElement('div');
  t.className='toast'+(kind?' '+kind:'');
  t.textContent=msg;
  wrap.appendChild(t);
  requestAnimationFrame(function(){t.classList.add('show')});
  setTimeout(function(){
    t.classList.remove('show');
    setTimeout(function(){t.remove()},300);
  },2600);
}

function fmtDur(s){
  s=Math.max(0,Math.floor(s));
  if(s<60)return s+'s';
  if(s<3600)return Math.floor(s/60)+'m '+Math.floor(s%60)+'s';
  var h=Math.floor(s/3600),m=Math.floor((s%3600)/60);
  return h+'h '+m+'m';
}

function wifiPct(rssi){return Math.min(100,Math.max(0,Math.round((rssi+100)*2)))}

/* ---------- theme ---------- */
function currentTheme(){return document.documentElement.getAttribute('data-theme')||'dark'}
function applyThemeIcon(){$('themeBtn').innerHTML=currentTheme()==='light'?ICO.moon:ICO.sun}
function toggleTheme(){
  var next=currentTheme()==='light'?'dark':'light';
  document.documentElement.setAttribute('data-theme',next);
  try{localStorage.setItem('pm-theme',next)}catch(e){}
  applyThemeIcon();
}
$('themeBtn').addEventListener('click',toggleTheme);
applyThemeIcon();

/* ---------- confirm modal ---------- */
function confirmModal(title,msg,confirmLabel){
  $('modalTitle').textContent=title;
  $('modalMsg').textContent=msg;
  $('modalConfirm').lastChild.textContent=confirmLabel||'Restart';
  var bd=$('modalBackdrop');
  bd.classList.add('show');
  return new Promise(function(resolve){
    confirmResolve=resolve;
    setTimeout(function(){$('modalConfirm').focus()},50);
  });
}
function closeModal(result){
  $('modalBackdrop').classList.remove('show');
  if(confirmResolve){confirmResolve(result);confirmResolve=null}
}
$('modalCancel').addEventListener('click',function(){closeModal(false)});
$('modalConfirm').addEventListener('click',function(){closeModal(true)});
$('modalBackdrop').addEventListener('click',function(e){if(e.target===$('modalBackdrop'))closeModal(false)});
document.addEventListener('keydown',function(e){if(e.key==='Escape'&&$('modalBackdrop').classList.contains('show'))closeModal(false)});

/* ---------- network ---------- */
function apiJson(path){
  return fetch(path).then(function(r){
    if(!r.ok)throw new Error('http '+r.status);
    return r.json();
  });
}
function apiFire(path){
  return fetch(path).then(function(r){return r.ok}).catch(function(){return false});
}

/* ---------- render: relays ---------- */
function ringOffset(remaining,total){
  var C=2*Math.PI*33;
  var frac=total>0?Math.max(0,Math.min(1,remaining/total)):0;
  return (C*(1-frac)).toFixed(1);
}
function renderRelays(list){
  var c=$('relays');
  var html='';
  list.forEach(function(r,i){
    var co=RC[i%4];
    if(r.restarting){
      restartMap[r.id]={remaining:r.remaining,total:r.restartSec};
      var C=2*Math.PI*33;
      html+='<div class="card r-card restarting" id="rcard'+r.id+'">'+
        '<div class="ring-wrap"><svg viewBox="0 0 76 76">'+
          '<circle class="ring-track" cx="38" cy="38" r="33"/>'+
          '<circle class="ring-prog" id="ring'+r.id+'" cx="38" cy="38" r="33" stroke-dasharray="'+C.toFixed(1)+'" stroke-dashoffset="'+ringOffset(r.remaining,r.restartSec)+'"/>'+
        '</svg><div class="ring-center"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="animation:spin 1.4s linear infinite"><path d="M21 12a9 9 0 1 1-3-6.7"/><path d="M21 3v6h-6"/></svg></div></div>'+
        '<div class="rname">'+esc(r.name)+'</div>'+
        '<div class="rconf">GPIO '+r.pin+' &middot; '+r.restartSec+'s</div>'+
        '<div class="badge restarting">'+ICO.x+'Restarting</div>'+
        '<div class="countdown" id="rem'+r.id+'">'+r.remaining+'s</div>'+
        '<div class="clbl">remaining</div>'+
      '</div>';
    }else{
      delete restartMap[r.id];
      html+='<div class="card r-card" id="rcard'+r.id+'">'+
        '<div class="ring-wrap"><div class="ring-center" style="color:'+co+'">'+ICO.bolt+'</div></div>'+
        '<div class="rname">'+esc(r.name)+'</div>'+
        '<div class="rconf">GPIO '+r.pin+' &middot; '+r.restartSec+'s</div>'+
        '<div class="badge ok">'+ICO.check+'Online</div>'+
        '<button class="btn btn-warn" data-action="restart" data-id="'+r.id+'" data-name="'+esc(r.name)+'"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 12a9 9 0 1 1-3-6.7"/><path d="M21 3v6h-6"/></svg>Restart</button>'+
      '</div>';
    }
  });
  c.innerHTML=html;
}
function tickRestartCountdowns(){
  Object.keys(restartMap).forEach(function(id){
    var e=restartMap[id];
    e.remaining=Math.max(0,e.remaining-1);
    var cd=$('rem'+id),ring=$('ring'+id);
    if(cd)cd.textContent=e.remaining+'s';
    if(ring)ring.setAttribute('stroke-dashoffset',ringOffset(e.remaining,e.total));
  });
}

/* ---------- render: wol ---------- */
function renderWol(list){
  var l=$('wollist');
  if(!list.length){l.innerHTML='<div class="empty-msg">No devices configured</div>';return}
  l.innerHTML=list.map(function(x){
    return '<div class="wol-row"><div><div class="wol-name">'+esc(x.name)+'</div><div class="wol-mac">'+esc(x.mac)+'</div></div>'+
      '<button class="btn btn-ghost" data-action="wake" data-id="'+x.id+'" data-name="'+esc(x.name)+'">'+ICO.bolt+'Wake</button></div>';
  }).join('');
}

/* ---------- render: schedules ---------- */
function minutesOf(h,m){return h*60+m}
function renderSchedules(data){
  var c=$('schedlist');
  if(!data.schedules.length){c.innerHTML='<div class="empty-msg">No scheduled restarts</div>';return}
  var nowM=0;
  var parts=(data.time||'').split(':');
  if(parts.length===2)nowM=minutesOf(parseInt(parts[0],10)||0,parseInt(parts[1],10)||0);
  var sorted=data.schedules.slice().sort(function(a,b){return minutesOf(a.hour,a.min)-minutesOf(b.hour,b.min)});
  var nextId=-1,bestDiff=1e9;
  sorted.forEach(function(s){
    if(!s.enabled)return;
    var diff=(minutesOf(s.hour,s.min)-nowM+1440)%1440;
    if(diff<bestDiff){bestDiff=diff;nextId=s.id}
  });
  c.innerHTML=sorted.map(function(s){
    var co=RC[s.relay%4];
    var isNext=s.id===nextId;
    return '<div class="sch-row'+(isNext?' next':'')+'">'+
      '<span class="sch-relay" style="background:'+co+'18;color:'+co+';border:1px solid '+co+'33">'+esc(s.relayName)+'</span>'+
      '<span class="sch-time">'+String(s.hour).padStart(2,'0')+':'+String(s.min).padStart(2,'0')+'</span>'+
      (isNext?'<span class="sch-next-tag">Next</span>':'')+
      '<span class="sch-status '+(s.enabled?'on':'off')+'">'+(s.enabled?'Active':'Off')+'</span></div>';
  }).join('');
}

/* ---------- render: ping ---------- */
function renderPing(p){
  var ic=$('pingcircle'),ix=$('pingicon'),ms=$('pingms');
  if(p.lastResult){
    ic.className='ping-indicator';
    ix.outerHTML='<svg class="icon" id="pingicon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M20 6 9 17l-5-5"/></svg>';
    ms.textContent=p.ms+' ms';
    ms.className='pm-val '+(p.ms<100?'fast':p.ms<300?'mid':'slow');
  }else{
    ic.className='ping-indicator fail';
    ix.outerHTML='<svg class="icon" id="pingicon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M18 6 6 18M6 6l12 12"/></svg>';
    ms.textContent='—';
    ms.className='pm-val slow';
  }
  $('pingfail').textContent=p.failCount;
  $('pingthr').textContent=p.threshold;
  $('pinghost').textContent=p.host;
  if(S.relays&&S.relays[p.relayTrigger])$('pingrelayname').textContent=S.relays[p.relayTrigger].name;
}

/* ---------- render: log ---------- */
function renderLog(list){
  var l=$('loglist');
  if(!list.length){l.innerHTML='<div class="empty-msg">No events</div>';return}
  var rev=list.slice().reverse();
  var key=rev.length+'|'+(rev[0]?rev[0].t+rev[0].m:'');
  var isNewFirst=key!==lastLogKey;
  lastLogKey=key;
  l.innerHTML=rev.map(function(e,i){
    return '<div class="log-item'+(isNewFirst&&i===0?' new':'')+'"><span class="log-t">'+esc(e.t)+'</span><span class="log-m">'+esc(e.m)+'</span></div>';
  }).join('');
}

/* ---------- connection status ---------- */
function setOnline(){
  consecutiveFails=0;
  pollDelay=3000;
  $('conndot').className='dot live';
  $('connlabel').textContent='Live';
  $('offlineBanner').classList.remove('show');
}
function setOffline(){
  consecutiveFails++;
  pollDelay=Math.min(10000,3000+consecutiveFails*1500);
  $('conndot').className='dot bad';
  $('connlabel').textContent='Offline';
  if(consecutiveFails>=2)$('offlineBanner').classList.add('show');
}

/* ---------- main refresh cycle ---------- */
function refresh(manual){
  if(fetching)return Promise.resolve();
  fetching=true;
  var btn=$('refreshBtn');
  if(manual)btn.classList.add('spinning');
  $('conndot').classList.toggle('live',consecutiveFails===0);

  return apiJson('/api/status').then(function(d){
    S=d;
    setOnline();
    $('ipsub').textContent=d.ip;
    $('datetime').textContent=d.datetime;
    $('uptime').textContent=fmtDur(d.uptime);
    var pct=wifiPct(d.wifi_rssi);
    $('wifipct').textContent=pct+'%';
    var bars=$('wifibars').children;
    var on=pct>=85?4:pct>=60?3:pct>=30?2:pct>0?1:0;
    for(var i=0;i<bars.length;i++)bars[i].classList.toggle('on',i<on);
    renderRelays(d.relays);
    renderWol(d.wol);
    renderPing(d.ping);
    return Promise.all([apiJson('/api/schedule'),apiJson('/api/log')]).then(function(res){
      renderSchedules(res[0]);
      renderLog(res[1]);
    }).catch(function(){/* keep last good schedule/log on partial failure */});
  }).catch(function(){
    setOffline();
  }).then(function(){
    fetching=false;
    if(manual)btn.classList.remove('spinning');
  });
}

function scheduleNext(){
  if(pollTimer)clearTimeout(pollTimer);
  pollTimer=setTimeout(function(){
    if(!document.hidden)refresh();
    scheduleNext();
  },pollDelay);
}

document.addEventListener('visibilitychange',function(){
  if(!document.hidden)refresh();
});

/* ---------- actions (exposed for inline handlers) ---------- */
function restartRelay(id,name){
  confirmModal('Restart '+name,'The relay will power off for a few seconds and then turn back on automatically. Continue?','Restart').then(function(ok){
    if(!ok)return;
    // optimistic UI update
    var r=S.relays&&S.relays.find(function(x){return x.id===id});
    if(r){
      r.restarting=true;
      r.remaining=r.restartSec;
      renderRelays(S.relays);
    }
    toast('Restarting '+name+'…');
    apiFire('/api/relays/restart?id='+id).then(function(ok2){
      if(!ok2)toast('Could not restart '+name,'err');
      refresh();
    });
  });
}
function wakeWol(id,name){
  toast('Sending packet to '+name+'…');
  apiFire('/api/wol/wake?id='+id).then(function(ok){
    toast(ok?(name+' woken up'):('Failed to wake '+name),ok?'ok':'err');
  });
}

$('refreshBtn').addEventListener('click',function(){refresh(true)});
$('retryBtn').addEventListener('click',function(){refresh(true)});

document.addEventListener('click',function(e){
  var btn=e.target.closest('[data-action]');
  if(!btn)return;
  var id=parseInt(btn.dataset.id,10);
  var name=btn.dataset.name;
  if(btn.dataset.action==='restart')restartRelay(id,name);
  else if(btn.dataset.action==='wake')wakeWol(id,name);
});

setInterval(tickRestartCountdowns,1000);

refresh();
scheduleNext();
})();
</script>
</body>
</html>
)rawliteral";

static void sendJson(AsyncWebServerRequest *req, int code, const String &json) {
    AsyncWebServerResponse *res = req->beginResponse(code, "application/json", json);
    res->addHeader("Access-Control-Allow-Origin", "*");
    req->send(res);
}

static int getParamInt(AsyncWebServerRequest *req, const char* name, int def) {
    if (req->hasParam(name)) return req->getParam(name)->value().toInt();
    return def;
}

static void handleGetStatus(AsyncWebServerRequest *req) {
    struct tm timeinfo;
    String dateTime = "?";
    if (getLocalTime(&timeinfo)) {
        char buf[40];
        const char* days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
        const char* months[] = {"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
        snprintf(buf, sizeof(buf), "%s %02d %s %04d  %02d:%02d:%02d",
            days[timeinfo.tm_wday], timeinfo.tm_mday, months[timeinfo.tm_mon + 1],
            timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        dateTime = buf;
    }
    String json = "{";
    json += "\"uptime\":" + String(millis() / 1000);
    json += ",\"datetime\":\"" + dateTime + "\"";
    json += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
    json += ",\"wifi_rssi\":" + String(WiFi.RSSI());
    json += ",\"relays\":" + relayStatusJson();
    json += ",\"wol\":" + wolStatusJson();
    json += ",\"ping\":" + pingStatusJson();
    json += "}";
    sendJson(req, 200, json);
}

static void sendHtml(AsyncWebServerRequest *req, int code, const String &html) {
    AsyncWebServerResponse *res = req->beginResponse(code, "text/html", html);
    res->addHeader("Cache-Control", "no-cache");
    req->send(res);
}

static String actionPage(const char *title, const char *msg, const char *color, bool ok) {
    String svg = ok
        ? "<svg width='48' height='48' viewBox='0 0 48 48' fill='none'><circle cx='24' cy='24' r='22' stroke='currentColor' stroke-width='3'/><path d='M14 24l7 7 13-13' stroke='currentColor' stroke-width='3' stroke-linecap='round' stroke-linejoin='round'/></svg>"
        : "<svg width='48' height='48' viewBox='0 0 48 48' fill='none'><circle cx='24' cy='24' r='22' stroke='currentColor' stroke-width='3'/><path d='M16 16l16 16M32 16L16 32' stroke='currentColor' stroke-width='3' stroke-linecap='round'/></svg>";
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>";
    html += title;
    html += "</title><style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:#0f1117;color:#e8eaed;min-height:100vh;display:flex;align-items:center;justify-content:center}.box{text-align:center;padding:48px 32px;max-width:400px;width:100%}.icon{width:88px;height:88px;border-radius:50%;margin:0 auto 28px;display:flex;align-items:center;justify-content:center;border:2px solid ";
    html += color;
    html += ";background:";
    html += color;
    html += "12;color:";
    html += color;
    html += "}h1{font-size:1.3rem;font-weight:700;margin-bottom:8px}p{font-size:.9rem;color:#9ca3af;line-height:1.6}.home{display:inline-block;margin-top:32px;color:#6366f1;text-decoration:none;font-size:.85rem;font-weight:600;border:1px solid #2e3348;padding:10px 24px;border-radius:10px;transition:all .15s}.home:hover{border-color:#6366f1;background:rgba(99,102,241,.1)}</style></head><body><div class='box'><div class='icon'>";
    html += svg;
    html += "</div><h1>";
    html += title;
    html += "</h1><p>";
    html += msg;
    html += "</p><a class='home' href='/'>Back to dashboard</a></div></body></html>";
    return html;
}

static String relayCardPage(int id) {
    int sec = appConfig.relays[id].restartSec;
    String name = appConfig.relays[id].name;
    String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>Restarting - ";
    html += name;
    html += "</title><style>*{margin:0;padding:0;box-sizing:border-box}:root{--bg:#0f1117;--surface:#1a1d27;--border:#2e3348;--border2:#3d4260;--restart:#f59e0b;--restart-glow:rgba(245,158,11,.12);--restart-border:rgba(245,158,11,.3);--on:#22c55e;--on-glow:rgba(34,197,94,.12);--on-border:rgba(34,197,94,.3);--accent:#6366f1;--text:#e8eaed;--text3:#6b7280}body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:var(--bg);color:var(--text);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:24px}.card{background:var(--surface);border:1px solid var(--border);border-radius:14px;padding:28px 16px;width:100%;max-width:340px;display:flex;flex-direction:column;align-items:center;text-align:center}.icon-wrap{width:64px;height:64px;border-radius:16px;display:flex;align-items:center;justify-content:center;margin-bottom:14px;border:2px solid var(--restart-border);background:var(--restart-glow);color:var(--restart);transition:all .3s}.icon-wrap.ok{border-color:var(--on-border);background:var(--on-glow);color:var(--on)}.rname{font-size:.9rem;font-weight:600;margin-bottom:2px}.rconf{font-size:.7rem;color:var(--text3);margin-bottom:12px}.badge{display:inline-flex;align-items:center;gap:5px;padding:4px 12px;border-radius:20px;font-size:.68rem;font-weight:600;letter-spacing:.3px;margin-bottom:14px}.badge.restarting{background:var(--restart-glow);color:var(--restart);border:1px solid var(--restart-border);animation:livepulse 1.5s ease infinite}.badge.ok{background:var(--on-glow);color:var(--on);border:1px solid var(--on-border)}.countdown{font-size:2rem;font-weight:700;font-variant-numeric:tabular-nums;color:var(--restart);line-height:1;margin-bottom:2px}.countdown.ok{color:var(--on)}.home{display:inline-block;margin-top:20px;color:var(--accent);text-decoration:none;font-size:.85rem;font-weight:600;border:1px solid var(--border2);padding:10px 24px;border-radius:10px;transition:all .15s}.home:hover{border-color:var(--accent);background:rgba(99,102,241,.1)}@keyframes spin{to{transform:rotate(360deg)}}@keyframes livepulse{0%,100%{opacity:1;box-shadow:0 0 8px var(--on)}50%{opacity:.5;box-shadow:0 0 4px var(--on)}}</style></head><body><div class='card'><div class='icon-wrap' id='ic'><svg width='32' height='32' viewBox='0 0 32 32' style='animation:spin 1s linear infinite' id='sp'><circle cx='16' cy='16' r='13' stroke='currentColor' stroke-width='3' fill='none' stroke-dasharray='60 20'/></svg><span id='bolt' style='display:none;font-size:28px'>&#9889;</span></div><div class='rname'>";
    html += name;
    html += "</div><div class='rconf'>Restart: ";
    html += String(sec);
    html += "s</div><div class='badge restarting' id='bd'>Restarting</div><div class='countdown' id='cnt'>";
    html += String(sec);
    html += "</div><div id='sec' style='font-size:.6rem;color:var(--text3);text-transform:uppercase;letter-spacing:1px'>seconds</div><a class='home' href='/'>Back to dashboard</a></div><script>let t=";
    html += String(sec);
    html += ";const cnt=document.getElementById('cnt'),bd=document.getElementById('bd'),ic=document.getElementById('ic'),sp=document.getElementById('sp'),bolt=document.getElementById('bolt');const iv=setInterval(()=>{t--;if(t<=0){clearInterval(iv);cnt.textContent='0';return}cnt.textContent=t},1000);async function check(){const d=await fetch('/api/status').then(r=>r.json());const r=d.relays[";
    html += String(id);
    html += "];if(r&&!r.restarting){clearInterval(iv);sp.style.display='none';bolt.style.display='inline';ic.className='icon-wrap ok';bd.className='badge ok';bd.innerHTML='&#9679; ONLINE';cnt.className='countdown ok';cnt.textContent='OK';document.getElementById('sec').textContent='Ready'}}setInterval(check,2000)</script></body></html>";
    return html;
}

static String wolCardPage(int id) {
    String name = appConfig.wolDevices[id].name;
    String mac = appConfig.wolDevices[id].mac;
    String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>WoL Sent - ";
    html += name;
    html += "</title><style>*{margin:0;padding:0;box-sizing:border-box}:root{--bg:#0f1117;--surface:#1a1d27;--border:#2e3348;--border2:#3d4260;--on:#22c55e;--on-glow:rgba(34,197,94,.12);--on-border:rgba(34,197,94,.3);--accent:#6366f1;--text:#e8eaed;--text2:#9ca3af;--text3:#6b7280}body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:var(--bg);color:var(--text);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:24px}.card{background:var(--surface);border:1px solid var(--border);border-radius:14px;padding:28px 16px;width:100%;max-width:340px;display:flex;flex-direction:column;align-items:center;text-align:center}.icon-wrap{width:64px;height:64px;border-radius:16px;display:flex;align-items:center;justify-content:center;font-size:28px;margin-bottom:14px;border:2px solid var(--on-border);background:var(--on-glow);color:var(--on)}.rname{font-size:.9rem;font-weight:600;margin-bottom:2px}.rmac{font-size:.7rem;color:var(--text3);font-family:Consolas,monospace;margin-bottom:12px}.badge{display:inline-flex;align-items:center;gap:5px;padding:4px 12px;border-radius:20px;font-size:.68rem;font-weight:600;letter-spacing:.3px;margin-bottom:14px;background:var(--on-glow);color:var(--on);border:1px solid var(--on-border)}.home{display:inline-block;margin-top:20px;color:var(--accent);text-decoration:none;font-size:.85rem;font-weight:600;border:1px solid var(--border2);padding:10px 24px;border-radius:10px;transition:all .15s}.home:hover{border-color:var(--accent);background:rgba(99,102,241,.1)}</style></head><body><div class='card'><div class='icon-wrap'><span>&#9889;</span></div><div class='rname'>";
    html += name;
    html += "</div><div class='rmac'>";
    html += mac;
    html += "</div><div class='badge'>&#9679; WoL Sent</div><a class='home' href='/'>Back to dashboard</a></div></body></html>";
    return html;
}

static void handleRestartRelay(AsyncWebServerRequest *req) {
    int id = getParamInt(req, "id", -1);
    if (id < 0 || id >= RELAY_COUNT) {
        sendHtml(req, 400, actionPage("Error", "Invalid relay ID", "#ef4444", false));
        return;
    }
    relayRestart(id);
    sendHtml(req, 200, relayCardPage(id));
}

static void handleWakeWol(AsyncWebServerRequest *req) {
    int id = getParamInt(req, "id", -1);
    if (id < 0 || !wolSend(id)) {
        sendHtml(req, 400, actionPage("Error", "Invalid device ID", "#ef4444", false));
        return;
    }
    sendHtml(req, 200, wolCardPage(id));
}

static void handleIndex(AsyncWebServerRequest *req) {
    AsyncWebServerResponse *res = req->beginResponse_P(200, "text/html", INDEX_HTML);
    res->addHeader("Cache-Control", "no-cache");
    req->send(res);
}

static void handleApiDoc(AsyncWebServerRequest *req) {
    AsyncWebServerResponse *res = req->beginResponse_P(200, "text/html", API_HTML);
    res->addHeader("Cache-Control", "no-cache");
    req->send(res);
}

inline void webserverInit() {
    server.on("/", HTTP_GET, handleIndex);
    server.on("/api-doc", HTTP_GET, handleApiDoc);
    server.on("/api/status", HTTP_GET, handleGetStatus);
    server.on("/api/relays/restart", HTTP_GET, handleRestartRelay);
    server.on("/api/wol/wake", HTTP_GET, handleWakeWol);
    server.on("/api/schedule", HTTP_GET, [](AsyncWebServerRequest *req) {
        sendJson(req, 200, scheduleStatusJson());
    });
    server.on("/api/log", HTTP_GET, [](AsyncWebServerRequest *req) {
        sendJson(req, 200, logJson());
    });

    server.onNotFound(handleIndex);

    server.begin();
    Serial.println("[WebServer] Started on port 80");
}

#endif
