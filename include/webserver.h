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
#include "util.h"
#include "actions.h"
#include <array>

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
.ep-tag.post{background:rgba(245,158,11,.12);color:#f59e0b;border:1px solid rgba(245,158,11,.3)}
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
<code>JSON</code> endpoints are for integrations (Home Assistant, scripts). Actions use <code>POST</code> and answer <code>{"ok":true}</code> or <code>{"ok":false,"error":"…"}</code> with a 4xx/5xx status.<br>
Endpoints marked <code>HTML</code> are plain <code>GET</code> links for a browser, a phone shortcut or an NFC tag: the action runs as soon as the URL is opened.<br>
<span id="authnote">If <code>WEB_AUTH_USER</code> is set in <code>config.h</code>, every endpoint needs HTTP Basic Auth (<code>curl -u user:pass …</code>).</span>
</div>

<h2><span class="bar"></span>System status</h2>
<div class="ep">
<div class="ep-top"><span class="ep-tag get">JSON</span><span class="ep-desc">Full status: relays, WoL, ping, and the device clock.</span></div>
<div class="ep-url-wrap"><span class="ep-url" id="u-status"></span><button class="ep-copy" data-copy="u-status">Copy</button></div>
<div class="ep-example" id="ex-status">{
  "uptime": 3661,
  "datetime": "Mon 29 Jul 2026  14:30:00",
  "synced": true,
  "ip": "10.0.0.93",
  "wifi_rssi": -55,
  "heap": 182340,
  "build": "Oct 31 2026 19:40:00",
  "logSeq": 12,
  "relays": [{"id":0,"name":"Router","pin":16,"restartSec":10,"on":true,"restarting":false,"remaining":0,
              "restarts":1,"lastAgo":5400,"lastReason":"schedule"}],
  "wol": [{"id":0,"name":"Office PC","mac":"F4:4D:30:18:58:95","lastAgo":-1}],
  "ping": {"host":"8.8.8.8","port":53,"probeSec":15,"restartAfter":900,"relayTrigger":0,"checked":true,
           "lastResult":true,"ms":24,"lastAgo":5,"nextIn":10,"failCount":0,"down":false,"noWifi":false,
           "downFor":0,"restartIn":-1,"grace":0,"pending":false,"seq":4410,"lastMin":29891190,"stored":true}
}</div>
<div class="ep-note"><code>lastAgo</code> is in seconds (<code>-1</code> = never). <code>lastReason</code>: <code>web</code> (dashboard), <code>api</code> (GET link), <code>schedule</code>, <code>ping</code>. <code>logSeq</code> grows with every log entry, so you only need to fetch <code>/api/log</code> when it changes.</div>
</div>

<h2><span class="bar"></span>Relays</h2>
<div class="ep">
<div class="ep-top"><span class="ep-tag post">POST</span><span class="ep-tag get">JSON</span><span class="ep-desc">Restarts a relay. For integrations; returns immediately.</span></div>
<div class="ep-url-wrap"><span class="ep-url" id="u-curl-restart"></span><button class="ep-copy" data-copy="u-curl-restart">Copy</button></div>
<div class="ep-params">Parameter: <code>id</code> (0-3). Errors: <code>400</code> invalid id, <code>409</code> already restarting.</div>
</div>
<div class="ep">
<div class="ep-top"><span class="ep-tag html">HTML</span><span class="ep-desc">Restarts a relay (turns it off for X seconds and back on) and shows a live countdown confirmation screen.</span></div>
<div id="restart-urls"></div>
<div class="ep-params">Parameter: <code>id</code> (0-3)</div>
<div class="ep-note">The action runs as soon as the URL is requested, without waiting for the page to render — it works whether opened in a browser or called with <code>fetch</code>/<code>curl</code>.</div>
</div>

<h2><span class="bar"></span>Wake on LAN</h2>
<div class="ep">
<div class="ep-top"><span class="ep-tag post">POST</span><span class="ep-tag get">JSON</span><span class="ep-desc">Sends a Wake-on-LAN magic packet. For integrations.</span></div>
<div class="ep-url-wrap"><span class="ep-url" id="u-curl-wol"></span><button class="ep-copy" data-copy="u-curl-wol">Copy</button></div>
<div class="ep-params">Parameter: <code>id</code> (device index). Errors: <code>400</code> invalid id, <code>503</code> WiFi down.</div>
</div>
<div class="ep">
<div class="ep-top"><span class="ep-tag html">HTML</span><span class="ep-desc">Sends a Wake-on-LAN magic packet to power on a remote device.</span></div>
<div id="wol-urls"></div>
<div class="ep-params">Parameter: <code>id</code> (0-4)</div>
</div>

<h2><span class="bar"></span>Internet monitor</h2>
<div class="ep">
<div class="ep-top"><span class="ep-tag post">POST</span><span class="ep-tag get">JSON</span><span class="ep-desc">Runs a connectivity check now. The result appears in <code>ping</code> of <code>/api/status</code> a few seconds later. A manual check is not stored in the history and never triggers a restart.</span></div>
<div class="ep-url-wrap"><span class="ep-url" id="u-curl-ping"></span><button class="ep-copy" data-copy="u-curl-ping">Copy</button></div>
</div>
<div class="ep">
<div class="ep-top"><span class="ep-tag get">BINARY</span><span class="ep-desc">Minute-by-minute history of the last 7 days, recorded by the device whether or not the dashboard is open.</span></div>
<div class="ep-url-wrap"><span class="ep-url" id="u-history"></span><button class="ep-copy" data-copy="u-history">Copy</button></div>
<div class="ep-params">Optional <code>from</code> = epoch minute: only minutes from there on (to update incrementally).</div>
<div class="ep-example">Header (20 bytes, little-endian):
  0 "PMH2"   4 version=2   5 seconds between checks   6 reserved
  8 now (epoch s, u32)    12 first minute (epoch min, u32)   16 count (u32)
Then count x 3 bytes, one per minute from "first":
  lat    0 = no successful check, 1-150 = (v-1)*2 ms, 151-255 = 300+(v-151)*25 ms
  flags  bits 0-2 checks done, bits 3-5 failed, bit 6 = ESP32 had no WiFi (all 0 = no data)
  rssi   average WiFi signal, signed dBm (0 = unknown)</div>
</div>
<div class="ep">
<div class="ep-top"><span class="ep-tag get">JSON</span><span class="ep-desc">Last 40 outages (2+ consecutive failed checks), newest first.</span></div>
<div class="ep-url-wrap"><span class="ep-url" id="u-outages"></span><button class="ep-copy" data-copy="u-outages">Copy</button></div>
<div class="ep-example">[[1793440000, 1390, 1, 0], ...]   // [startEpoch, seconds, routerRestarted, noWifi]</div>
</div>

<h2><span class="bar"></span>Scheduled restarts</h2>
<div class="ep">
<div class="ep-top"><span class="ep-tag get">JSON</span><span class="ep-desc">Configured schedules and the device's local time. Schedules wait until <code>synced</code> is true (NTP).</span></div>
<div class="ep-url-wrap"><span class="ep-url" id="u-schedule"></span><button class="ep-copy" data-copy="u-schedule">Copy</button></div>
<div class="ep-example">{
  "time": "14:30",
  "synced": true,
  "schedules": [{"id":0,"relay":0,"relayName":"Router","hour":4,"min":0,"enabled":true,"ranToday":true}]
}</div>
</div>

<h2><span class="bar"></span>Event log</h2>
<div class="ep">
<div class="ep-top"><span class="ep-tag get">JSON</span><span class="ep-desc">Latest system events (max 50, oldest first).</span></div>
<div class="ep-url-wrap"><span class="ep-url" id="u-log"></span><button class="ep-copy" data-copy="u-log">Copy</button></div>
<div class="ep-example">[
  {"t":"29/07 14:30:12","m":"Router: power OFF for 10s (web)"},
  {"t":"29/07 14:30:22","m":"Router: power back ON"}
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
  $('u-history').textContent=base+'/api/ping/history';
  $('u-history').setAttribute('data-url',base+'/api/ping/history');
  $('u-outages').textContent=base+'/api/ping/outages';
  $('u-outages').setAttribute('data-url',base+'/api/ping/outages');
  [['u-curl-restart','/api/relays/restart?id=0'],['u-curl-wol','/api/wol/wake?id=0'],['u-curl-ping','/api/ping/check']].forEach(function(x){
    var cmd='curl -X POST "'+base+x[1]+'"';
    $(x[0]).textContent=cmd;$(x[0]).setAttribute('data-url',cmd);
  });

  var restartWrap=$('restart-urls');
  var relayNames=[null,null,null,null];
  var d=null;
  try{
    var r=await fetch(base+'/api/status');
    d=await r.json();
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
    if(!d)throw new Error('offline');
    var wol=d.wol||[];
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

/* --- additions --- */
.r-card .rmeta{font-size:.66rem;color:var(--text3);margin:-6px 0 14px;min-height:1.1em}
.r-card .badge.pending{background:var(--accent-glow);color:var(--accent);border:1px solid var(--accent)}
.r-card .btn{min-width:120px}
.btn .spin{animation:spin .8s linear infinite}
.is-offline [data-action]{opacity:.45;pointer-events:none}
.wol-row .wol-sent{font-size:.62rem;color:var(--on);margin-top:2px}
.sch-row .sch-done,.sch-row .sch-next-tag,.sch-row .sch-status{white-space:nowrap}
.grid-bottom>*,.grid-relays>*{min-width:0}
.sch-row{flex-wrap:wrap;row-gap:6px}
.sch-row .sch-relay{max-width:100%;overflow:hidden;text-overflow:ellipsis}
.wol-row>div:first-child{min-width:0}
.wol-row .wol-name{overflow-wrap:anywhere}
.sch-row .sch-done{font-size:.6rem;font-weight:700;color:var(--text3);border:1px solid var(--border2);padding:2px 8px;border-radius:10px}
.notice{display:flex;gap:8px;align-items:flex-start;font-size:.72rem;line-height:1.5;color:var(--restart);background:var(--restart-glow);border:1px solid var(--restart-border);border-radius:10px;padding:8px 12px;margin-bottom:10px}
.notice .icon{width:14px;height:14px;margin-top:2px}
.ping-indicator.pending{border-color:var(--border2);background:var(--surface2);color:var(--text3)}
.ping-indicator.grace{border-color:var(--restart-border);background:var(--restart-glow);color:var(--restart)}
.ping-metric .pm-val.muted{color:var(--text2)}
.ping-actions{margin-left:auto}
.pill.warn{color:var(--restart)}
.footer{max-width:1280px;margin:0 auto;padding:0 32px 32px;font-size:.66rem;color:var(--text3);display:flex;gap:14px;flex-wrap:wrap;justify-content:center}
/* --- internet monitor --- */
[hidden]{display:none!important}
.net-card{padding:20px 22px}
.net-head{display:flex;align-items:center;gap:16px;flex-wrap:wrap}
.net-head .ping-indicator{width:52px;height:52px;border-radius:14px}
.net-state{flex:1;min-width:180px}
.net-title{font-size:1.05rem;font-weight:700;line-height:1.25}
.net-sub{font-size:.74rem;color:var(--text2);margin-top:2px}
.net-actions{display:flex;gap:10px;align-items:center;flex-wrap:wrap}
.seg{display:inline-flex;border:1px solid var(--border2);border-radius:9px;padding:2px;background:var(--surface2)}
.seg button{border:0;background:transparent;color:var(--text2);font:inherit;font-size:.72rem;font-weight:600;padding:5px 12px;border-radius:7px;cursor:pointer}
.seg button.on{background:var(--surface);color:var(--text);box-shadow:0 1px 3px rgba(0,0,0,.25)}
.kpis{display:grid;grid-template-columns:repeat(auto-fit,minmax(130px,1fr));gap:10px;margin:18px 0 6px}
.kpi{background:var(--surface2);border:1px solid var(--border);border-radius:10px;padding:10px 12px;min-width:0}
.kpi .kv{font-size:1.25rem;font-weight:700;font-variant-numeric:tabular-nums;line-height:1.15;white-space:nowrap}
.kpi .kl{font-size:.6rem;color:var(--text3);text-transform:uppercase;letter-spacing:.9px;margin-top:3px}
.kpi .ks{font-size:.66rem;color:var(--text2);margin-top:3px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.kpi .kv .u{font-size:.72rem;font-weight:600;color:var(--text2);margin-left:2px}
.chart-block{margin-top:18px;min-width:0}
.chart-head{display:flex;align-items:baseline;justify-content:space-between;gap:10px;flex-wrap:wrap;margin-bottom:8px}
.chart-title{font-size:.72rem;font-weight:700;color:var(--text2)}
.chart-note{font-size:.66rem;color:var(--text3)}
.chart{position:relative;width:100%}
.chart svg{display:block;width:100%;overflow:visible}
.chart text,#heatmap text{font-size:10px;fill:var(--text3);font-family:inherit}
.legend{display:flex;gap:12px;flex-wrap:wrap;font-size:.66rem;color:var(--text2);align-items:center}
.legend i{display:inline-block;width:10px;height:10px;border-radius:3px;margin-right:5px;vertical-align:-1px}
.legend i.line{height:2px;border-radius:1px;vertical-align:3px}
.net-grid{display:grid;grid-template-columns:3fr 2fr;gap:20px}
@media(max-width:900px){.net-grid{grid-template-columns:1fr}}
#heatmap svg{display:block;width:100%}
#heatmap rect.cell{cursor:default}
#heatmap rect.cell:hover,.chart rect.bar:hover{stroke:var(--text);stroke-width:1.5}
.out-row{display:flex;align-items:center;gap:10px;flex-wrap:wrap;padding:8px 12px;border-radius:9px;background:var(--surface2);border:1px solid var(--border);margin-bottom:6px;font-size:.76rem}
.out-row .ot{font-family:'SF Mono',Consolas,monospace;font-size:.7rem;color:var(--text2);min-width:118px}
.out-row .od{font-weight:700;font-variant-numeric:tabular-nums;min-width:64px}
.tag{font-size:.6rem;font-weight:700;padding:2px 8px;border-radius:10px;border:1px solid var(--border2);color:var(--text2);white-space:nowrap}
.tag.crit{color:var(--off);border-color:var(--off-border);background:var(--off-glow)}
.tag.warn{color:var(--restart);border-color:var(--restart-border);background:var(--restart-glow)}
.tag.live{color:var(--off);border-color:var(--off);animation:livepulse 1.6s ease infinite}
.tip{position:fixed;z-index:400;pointer-events:none;background:var(--surface);color:var(--text);border:1px solid var(--border2);box-shadow:var(--shadow);border-radius:9px;padding:8px 10px;font-size:.72rem;line-height:1.5;opacity:0;transition:opacity .12s;max-width:240px}
.tip.show{opacity:1}
.tip b{font-weight:700}
.tip .tm{color:var(--text3);font-size:.66rem}
@media(max-width:600px){.net-card{padding:16px 14px}.net-actions{width:100%;justify-content:space-between}.kpis{grid-template-columns:repeat(2,1fr)}.kpis .kpi:last-child:nth-child(odd){grid-column:1/-1}}
.net-head .btn{margin-left:auto}
.lookup{margin-top:18px;background:var(--surface2);border:1px solid var(--border);border-radius:12px;padding:12px 14px}
.lookup label{display:block;font-size:.72rem;font-weight:700;color:var(--text2);margin-bottom:8px}
.lookup-row{display:flex;gap:10px;flex-wrap:wrap}
.lookup input{flex:1;min-width:200px;font:inherit;font-size:.82rem;color:var(--text);background:var(--surface);border:1px solid var(--border2);border-radius:9px;padding:8px 10px;color-scheme:inherit}
.lookup input:focus{outline:2px solid var(--accent);outline-offset:1px}
.verdict{display:flex;gap:12px;align-items:flex-start;margin-top:10px;border-radius:12px;padding:12px 14px;border:1px solid var(--border2);background:var(--surface2);font-size:.8rem;line-height:1.55}
.verdict .vi{width:30px;height:30px;border-radius:9px;display:flex;align-items:center;justify-content:center;flex-shrink:0;border:1px solid currentColor}
.verdict .vi .icon{width:16px;height:16px}
.verdict b{font-weight:700}
.verdict .vs{color:var(--text2);font-size:.74rem}
.verdict.ok .vi{color:var(--on);background:var(--on-glow)}
.verdict.warn .vi{color:var(--restart);background:var(--restart-glow)}
.verdict.crit .vi{color:var(--off);background:var(--off-glow)}
.verdict.none .vi{color:var(--text3)}
.verdict .vx{margin-left:auto;background:none;border:0;color:var(--text3);cursor:pointer;font-size:1rem;line-height:1;padding:2px 4px}
.net-toolbar{display:flex;justify-content:space-between;align-items:center;gap:10px;flex-wrap:wrap;margin-top:18px}
.nav{display:flex;align-items:center;gap:8px}
.nav .iconbtn:disabled{opacity:.35;cursor:default}
.nav-lbl{font-size:.74rem;font-weight:600;color:var(--text2);min-width:150px;text-align:center;font-variant-numeric:tabular-nums}
.out-row{cursor:pointer}
.out-row:hover{border-color:var(--accent)}
.chart{cursor:crosshair}
@media(max-width:600px){.nav{width:100%;justify-content:space-between}.nav-lbl{min-width:0;flex:1}.net-head .btn{margin-left:0;width:100%}}
/* compact horizontal relay cards on phones */
@media(max-width:600px){
.grid-relays{gap:10px}
.r-card{display:grid;grid-template-columns:52px 1fr auto;column-gap:14px;row-gap:2px;align-items:center;text-align:left;padding:14px 16px}
.r-card .ring-wrap{grid-column:1;grid-row:1/span 4;width:52px;height:52px;margin:0}
.r-card .ring-center .icon{width:22px;height:22px}
.r-card .rname,.r-card .rconf,.r-card .badge,.r-card .rmeta{grid-column:2;margin:0;justify-self:start}
.r-card .badge{margin:3px 0}
.r-card .btn{grid-column:3;grid-row:1/span 4;min-width:0;padding:9px 14px}
.r-card .countdown{grid-column:3;grid-row:1/span 2;align-self:end;font-size:1.4rem;text-align:right}
.r-card .clbl{grid-column:3;grid-row:3/span 2;align-self:start;margin:0;text-align:right}
}
@media(max-width:600px){.footer{padding:0 14px 24px}.ping-actions{margin-left:0;width:100%}.ping-actions .btn{width:100%}}
@media(prefers-reduced-motion:reduce){*,*::before,*::after{animation-duration:.01ms!important;animation-iteration-count:1!important;transition-duration:.01ms!important}}
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
<span class="pill" id="timepill" title="Device time"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 3"/></svg><span id="datetime">--</span></span>
<span class="sep"></span>
<span class="pill" title="Uptime"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 22a10 10 0 1 0 0-20 10 10 0 0 0 0 20Z"/><path d="M12 6v6l4 2"/></svg><span id="uptime">0s</span></span>
<span class="sep"></span>
<span class="pill" id="wifipill" title="WiFi signal"><span class="wifi-bars" id="wifibars"><i></i><i></i><i></i><i></i></span><span id="wifipct">--</span></span>
<button class="iconbtn" id="refreshBtn" title="Refresh now" aria-label="Refresh now"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 12a9 9 0 1 1-3-6.7"/><path d="M21 3v6h-6"/></svg></button>
</div>
</div>
<div id="offlineBanner" role="alert"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 9v4M12 17h.01M10.3 3.9 1.8 18a2 2 0 0 0 1.7 3h17a2 2 0 0 0 1.7-3L13.7 3.9a2 2 0 0 0-3.4 0Z"/></svg><span id="offlineMsg">Lost connection to the device. Retrying…</span><button id="retryBtn">Retry now</button></div>

<div class="main">
<div class="section"><div class="section-title"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M13 2 3 14h7l-1 8 10-12h-7l1-8Z"/></svg>Relays</div><div class="grid-relays" id="relays">
<div class="card r-card"><div class="skel" style="width:76px;height:76px;border-radius:50%;margin-bottom:12px"></div><div class="skel" style="width:70%;height:14px;margin-bottom:8px"></div><div class="skel" style="width:50%;height:10px;margin-bottom:16px"></div><div class="skel" style="width:100%;height:32px;border-radius:10px"></div></div>
</div></div>

<div class="section"><div class="section-title"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 2 2 7l10 5 10-5-10-5Z"/><path d="M2 17l10 5 10-5M2 12l10 5 10-5"/></svg>Operations</div>
<div class="grid-bottom">
<div class="card"><div class="card-head"><span class="ch-dot" style="background:var(--accent)"></span>Wake on LAN</div><div id="wollist"></div></div>
<div class="card"><div class="card-head"><span class="ch-dot" style="background:var(--restart)"></span>Scheduled restarts</div><div id="schedlist"></div></div>
</div></div>

<div class="section"><div class="section-title"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 12h4l2 8 4-16 2 8h4"/></svg>Internet monitor</div>
<div class="card net-card">
<div class="net-head">
<div class="ping-indicator pending" id="pingcircle"></div>
<div class="net-state"><div class="net-title" id="netTitle">Waiting for the first measurement</div><div class="net-sub" id="netSub">&nbsp;</div></div>
<button class="btn btn-ghost" data-action="ping" id="pingBtn">Check now</button>
</div>

<form class="lookup" id="lookForm" autocomplete="off">
<label for="lookAt">Was it the internet? Pick a moment from the last 7 days</label>
<div class="lookup-row"><input type="datetime-local" id="lookAt" required><button class="btn btn-primary" type="submit">Check</button></div>
</form>
<div class="verdict" id="netVerdict" hidden></div>

<div class="net-toolbar">
<div class="seg" id="netRange" role="group" aria-label="Time range"><button data-range="60">1h</button><button data-range="360" class="on">6h</button><button data-range="1440">24h</button><button data-range="10080">7d</button></div>
<div class="nav">
<button class="iconbtn" id="navPrev" title="Earlier" aria-label="Earlier"></button>
<span class="nav-lbl" id="navLbl">Last 6 h</span>
<button class="iconbtn" id="navNext" title="Later" aria-label="Later"></button>
<button class="btn btn-ghost" id="navNow" hidden>Now</button>
</div>
</div>
<div class="kpis" id="netKpis"></div>
<div class="chart-block">
<div class="chart-head"><span class="chart-title">Latency and failed checks <span class="chart-note">· click the chart to inspect a minute</span></span><span class="legend" id="latLegend"></span></div>
<div class="chart" id="latChart"><div class="empty-msg">Loading history…</div></div>
</div>
<div class="net-grid">
<div class="chart-block">
<div class="chart-head"><span class="chart-title">Last 7 days, hour by hour</span><span class="chart-note">click an hour to open it</span></div>
<div id="heatmap"></div>
<div class="legend" id="heatLegend"></div>
</div>
<div class="chart-block">
<div class="chart-head"><span class="chart-title">Latency distribution</span><span class="chart-note" id="histNote"></span></div>
<div class="chart" id="histChart"></div>
</div>
</div>
<div class="chart-block">
<div class="chart-head"><span class="chart-title">Outages</span><span class="chart-note" id="outNote"></span></div>
<div id="outages"></div>
</div>
<div class="ping-desc" id="pingdesc">Loading…</div>
</div>
<div id="tip" class="tip" role="tooltip"></div>
</div>

<div class="section"><div class="section-title"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2Z"/></svg>Log</div>
<div class="card"><div class="log-wrap" id="loglist"><div class="empty-msg">No events</div></div></div>
</div>
</div>
<div class="footer" id="footer"></div>

<div class="modal-backdrop" id="modalBackdrop">
<div class="modal" role="dialog" aria-modal="true" aria-labelledby="modalTitle" aria-describedby="modalMsg">
<div class="modal-icon"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 12a9 9 0 1 1-3-6.7"/><path d="M21 3v6h-6"/></svg></div>
<h3 id="modalTitle">Restart</h3>
<p id="modalMsg">Confirm?</p>
<div class="modal-actions">
<button class="btn btn-ghost" id="modalCancel">Cancel</button>
<button class="btn btn-warn" id="modalConfirm"><svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 12a9 9 0 1 1-3-6.7"/><path d="M21 3v6h-6"/></svg><span>Restart</span></button>
</div>
</div>
</div>

<div id="toastWrap" aria-live="polite"></div>

<script>
(function(){
'use strict';

var SVG='<svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">';
var ICO={
  bolt:'<svg class="icon" viewBox="0 0 24 24" fill="currentColor"><path d="M13 2 3 14h7l-1 8 10-12h-7l1-8Z"/></svg>',
  check:SVG.replace('stroke-width="2"','stroke-width="2.5"')+'<path d="M20 6 9 17l-5-5"/></svg>',
  x:SVG.replace('stroke-width="2"','stroke-width="2.5"')+'<path d="M18 6 6 18M6 6l12 12"/></svg>',
  clock:SVG+'<circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 3"/></svg>',
  pause:SVG+'<path d="M10 5v14M14 5v14"/></svg>',
  warn:SVG+'<path d="M12 9v4M12 17h.01M10.3 3.9 1.8 18a2 2 0 0 0 1.7 3h17a2 2 0 0 0 1.7-3L13.7 3.9a2 2 0 0 0-3.4 0Z"/></svg>',
  restart:SVG+'<path d="M21 12a9 9 0 1 1-3-6.7"/><path d="M21 3v6h-6"/></svg>',
  spinner:'<svg class="icon spin" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round"><path d="M21 12a9 9 0 1 1-6.2-8.6"/></svg>',
  sun:'<svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4"/></svg>',
  moon:SVG+'<path d="M21 12.8A9 9 0 1 1 11.2 3 7 7 0 0 0 21 12.8Z"/></svg>'
};
var RC=['#6366f1','#22c55e','#ec4899','#f59e0b'];
var REASON={web:'dashboard',api:'URL / shortcut',schedule:'schedule',ping:'no internet'};

var S=null;               // last /api/status
var SCH=null;             // last /api/schedule
var deadlines={};         // relay id -> ms timestamp when it powers back on
var pending={};           // relay id -> ms timestamp of a restart we requested
var relaySig='';
var lastLogSeq=-1, lastLogLen=0;
var lastSchedFetch=0;
var consecutiveFails=0, pollDelay=3000, pollTimer=null, fetching=false;
var pingWaiting=false;
var confirmResolve=null, lastFocus=null;

/* ---------- helpers ---------- */
function $(id){return document.getElementById(id)}
function esc(s){return String(s==null?'':s).replace(/[&<>"']/g,function(c){return({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'})[c]})}
function pad2(n){return String(n).padStart(2,'0')}
function toast(msg,kind){
  var t=document.createElement('div');
  t.className='toast'+(kind?' '+kind:'');
  t.textContent=msg;
  $('toastWrap').appendChild(t);
  requestAnimationFrame(function(){t.classList.add('show')});
  setTimeout(function(){t.classList.remove('show');setTimeout(function(){t.remove()},300)},2800);
}
function fmtDur(s){
  s=Math.max(0,Math.floor(s));
  if(s<60)return s+'s';
  if(s<3600)return Math.floor(s/60)+'m '+(s%60)+'s';
  if(s<86400)return Math.floor(s/3600)+'h '+Math.floor((s%3600)/60)+'m';
  return Math.floor(s/86400)+'d '+Math.floor((s%86400)/3600)+'h';
}
function fmtAgo(s){
  if(s==null||s<0)return '';
  if(s<10)return 'just now';
  if(s<60)return s+'s ago';
  if(s<3600)return Math.floor(s/60)+'m ago';
  if(s<86400)return Math.floor(s/3600)+'h ago';
  return Math.floor(s/86400)+'d ago';
}
function fmtIn(s){
  if(s<=0)return 'now';
  if(s<60)return 'in '+s+'s';
  if(s<3600)return 'in '+Math.ceil(s/60)+'m';
  return 'in '+Math.floor(s/3600)+'h '+Math.floor((s%3600)/60)+'m';
}
function fmtEvery(s){return s%3600===0?(s/3600)+' h':s%60===0?(s/60)+' min':fmtDur(s)}
function wifiPct(rssi){return Math.min(100,Math.max(0,Math.round((rssi+100)*2)))}
function relayName(id){return S&&S.relays[id]?S.relays[id].name:('Relay '+id)}

/* ---------- theme ---------- */
function currentTheme(){return document.documentElement.getAttribute('data-theme')||'dark'}
function applyThemeIcon(){$('themeBtn').innerHTML=currentTheme()==='light'?ICO.moon:ICO.sun}
$('themeBtn').addEventListener('click',function(){
  var next=currentTheme()==='light'?'dark':'light';
  document.documentElement.setAttribute('data-theme',next);
  try{localStorage.setItem('pm-theme',next)}catch(e){}
  applyThemeIcon();
});
applyThemeIcon();

/* ---------- confirm modal ---------- */
function confirmModal(title,msg,label){
  lastFocus=document.activeElement;
  $('modalTitle').textContent=title;
  $('modalMsg').textContent=msg;
  $('modalConfirm').querySelector('span').textContent=label||'Restart';
  $('modalBackdrop').classList.add('show');
  return new Promise(function(resolve){
    confirmResolve=resolve;
    setTimeout(function(){$('modalCancel').focus()},50);
  });
}
function closeModal(result){
  $('modalBackdrop').classList.remove('show');
  if(confirmResolve){confirmResolve(result);confirmResolve=null}
  if(lastFocus&&lastFocus.focus)lastFocus.focus();
}
$('modalCancel').addEventListener('click',function(){closeModal(false)});
$('modalConfirm').addEventListener('click',function(){closeModal(true)});
$('modalBackdrop').addEventListener('click',function(e){if(e.target===$('modalBackdrop'))closeModal(false)});
document.addEventListener('keydown',function(e){
  if(!$('modalBackdrop').classList.contains('show'))return;
  if(e.key==='Escape')closeModal(false);
  if(e.key==='Tab'){ // keep focus inside the dialog
    var a=$('modalCancel'),b=$('modalConfirm');
    if(e.shiftKey&&document.activeElement===a){e.preventDefault();b.focus()}
    else if(!e.shiftKey&&document.activeElement===b){e.preventDefault();a.focus()}
  }
});

/* ---------- network ---------- */
function getJson(path){
  return fetch(path,{cache:'no-store'}).then(function(r){
    if(!r.ok)throw new Error('HTTP '+r.status);
    return r.json();
  });
}
function postJson(path){
  return fetch(path,{method:'POST',cache:'no-store'}).then(function(r){
    return r.json().catch(function(){return{}}).then(function(j){
      if(!r.ok||j.ok===false)throw new Error(j.error||('HTTP '+r.status));
      return j;
    });
  });
}

/* ---------- relays ---------- */
var C=2*Math.PI*33;
function remainingOf(id){
  var d=deadlines[id];
  return d?Math.max(0,Math.ceil((d-Date.now())/1000)):0;
}
function ringOffset(rem,total){
  var f=total>0?Math.max(0,Math.min(1,rem/total)):0;
  return (C*(1-f)).toFixed(1);
}
function relayState(r){
  // A restart we just requested may not be reflected by the device yet
  if(!r.restarting&&pending[r.id]&&Date.now()-pending[r.id]<6000)return 'pending';
  return r.restarting?'restarting':'on';
}
function relayMeta(r){
  if(r.lastAgo<0)return 'No restarts since boot';
  return 'Last: '+fmtAgo(r.lastAgo)+' · '+(REASON[r.lastReason]||r.lastReason)+(r.restarts>1?' · '+r.restarts+'×':'');
}
function renderRelays(){
  var list=S.relays;
  list.forEach(function(r){
    if(r.restarting){
      delete pending[r.id];
      var want=Date.now()+r.remaining*1000;
      // only resync if our local countdown drifted by more than ~1.5 s
      if(!deadlines[r.id]||Math.abs(deadlines[r.id]-want)>1500)deadlines[r.id]=want;
    }else if(relayState(r)!=='pending'){
      delete deadlines[r.id];
    }
  });
  var sig=list.map(function(r){return [r.id,r.name,relayState(r),relayMeta(r)].join('|')}).join(';');
  if(sig===relaySig){tickCountdowns();return}
  relaySig=sig;

  $('relays').innerHTML=list.map(function(r,i){
    var st=relayState(r);
    var head='<div class="rname">'+esc(r.name)+'</div><div class="rconf">GPIO '+r.pin+' &middot; '+r.restartSec+'s off</div>';
    if(st==='on'){
      return '<div class="card r-card">'+
        '<div class="ring-wrap"><div class="ring-center" style="color:'+RC[i%4]+'">'+ICO.bolt+'</div></div>'+head+
        '<div class="badge ok">'+ICO.check+'Powered</div>'+
        '<div class="rmeta">'+esc(relayMeta(r))+'</div>'+
        '<button class="btn btn-warn" data-action="restart" data-id="'+r.id+'" aria-label="Restart '+esc(r.name)+'">'+ICO.restart+'Restart</button>'+
      '</div>';
    }
    var rem=st==='pending'?r.restartSec:remainingOf(r.id);
    return '<div class="card r-card restarting">'+
      '<div class="ring-wrap"><svg viewBox="0 0 76 76"><circle class="ring-track" cx="38" cy="38" r="33"/>'+
      '<circle class="ring-prog" id="ring'+r.id+'" cx="38" cy="38" r="33" stroke-dasharray="'+C.toFixed(1)+'" stroke-dashoffset="'+ringOffset(rem,r.restartSec)+'"/></svg>'+
      '<div class="ring-center">'+ICO.restart.replace('class="icon"','class="icon spin" style="animation-duration:1.4s"')+'</div></div>'+head+
      (st==='pending'?'<div class="badge pending">'+ICO.clock+'Sending…</div>':'<div class="badge restarting">'+ICO.x+'Power off</div>')+
      '<div class="countdown" id="rem'+r.id+'" aria-live="off">'+rem+'s</div>'+
      '<div class="clbl" id="remlbl'+r.id+'">until power on</div>'+
    '</div>';
  }).join('');
}
function tickCountdowns(){
  Object.keys(deadlines).forEach(function(id){
    var r=S&&S.relays[id];if(!r)return;
    var rem=remainingOf(id);
    var cd=$('rem'+id),ring=$('ring'+id),lbl=$('remlbl'+id);
    if(cd)cd.textContent=rem>0?rem+'s':'…';
    if(lbl&&rem===0)lbl.textContent='powering on';
    if(ring)ring.setAttribute('stroke-dashoffset',ringOffset(rem,r.restartSec));
  });
}

/* ---------- wake on lan ---------- */
var wolSig='';
function renderWol(){
  var list=S.wol;
  var sig=list.map(function(x){return x.id+x.name+fmtAgo(x.lastAgo)}).join(';');
  if(sig===wolSig)return;
  wolSig=sig;
  var l=$('wollist');
  if(!list.length){l.innerHTML='<div class="empty-msg">No devices configured (WOL_x_NAME / WOL_x_MAC in config.h)</div>';return}
  l.innerHTML=list.map(function(x){
    return '<div class="wol-row"><div><div class="wol-name">'+esc(x.name)+'</div><div class="wol-mac">'+esc(x.mac)+'</div>'+
      (x.lastAgo>=0?'<div class="wol-sent">Packet sent '+fmtAgo(x.lastAgo)+'</div>':'')+'</div>'+
      '<button class="btn btn-ghost" data-action="wake" data-id="'+x.id+'" aria-label="Wake '+esc(x.name)+'">'+ICO.bolt+'<span>Wake</span></button></div>';
  }).join('');
}

/* ---------- schedules ---------- */
function minutesOf(h,m){return h*60+m}
function renderSchedules(){
  var c=$('schedlist'),data=SCH;
  if(!data)return;
  var html='';
  if(!data.synced)html+='<div class="notice">'+ICO.warn+'<span>Clock not synced yet (NTP). Scheduled restarts wait until the time is known.</span></div>';
  if(!data.schedules.length){c.innerHTML=html+'<div class="empty-msg">No scheduled restarts</div>';return}
  var nowM=-1,p=(data.time||'').split(':');
  if(data.synced&&p.length===2){
    // advance the device minute locally between fetches
    nowM=(minutesOf(+p[0],+p[1])+Math.floor((Date.now()-lastSchedFetch)/60000))%1440;
  }
  var sorted=data.schedules.slice().sort(function(a,b){return minutesOf(a.hour,a.min)-minutesOf(b.hour,b.min)});
  var nextId=-1,best=1e9;
  if(nowM>=0)sorted.forEach(function(s){
    if(!s.enabled)return;
    var diff=(minutesOf(s.hour,s.min)-nowM+1440)%1440;
    if(diff===0&&s.ranToday)diff=1440;
    if(diff<best){best=diff;nextId=s.id}
  });
  c.innerHTML=html+sorted.map(function(s){
    var co=RC[s.relay%4],isNext=s.id===nextId;
    return '<div class="sch-row'+(isNext?' next':'')+'">'+
      '<span class="sch-relay" style="background:'+co+'18;color:'+co+';border:1px solid '+co+'33">'+esc(s.relayName)+'</span>'+
      '<span class="sch-time">'+pad2(s.hour)+':'+pad2(s.min)+'</span>'+
      (isNext?'<span class="sch-next-tag">Next · '+fmtIn(best*60)+'</span>':(s.ranToday?'<span class="sch-done">Done today</span>':''))+
      '<span class="sch-status '+(s.enabled?'on':'off')+'">'+(s.enabled?'Active':'Off')+'</span></div>';
  }).join('');
}

/* ---------- internet monitor ---------- */
/* Minute-by-minute history recorded by the device (binary /api/ping/history) */
var NM=10080,recId=new Float64Array(NM).fill(-1),recLat=new Uint8Array(NM),recFl=new Uint8Array(NM),recRs=new Int8Array(NM);
var histNow=0,histAt=0,histLast=0,histSeq=-1,histFull=0,OUT=[],histLoading=false;
var span=360,live=true,viewEnd=0,markMin=null;
try{var sr=+localStorage.getItem('pm-span');if([60,360,1440,10080].indexOf(sr)>=0)span=sr}catch(e){}
var DOW=['Sun','Mon','Tue','Wed','Thu','Fri','Sat'],MON=['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];
function hhmm(d){return pad2(d.getHours())+':'+pad2(d.getMinutes())}
function dayLbl(d){return DOW[d.getDay()]+' '+d.getDate()+' '+MON[d.getMonth()]}
function minDate(m){return new Date(m*60000)}
function whenLbl(m){var d=minDate(m),t=new Date();t.setHours(0,0,0,0);var dd=Math.round((new Date(d).setHours(0,0,0,0)-t)/86400000);
  return (dd===0?'today':dd===-1?'yesterday':dayLbl(d))+' '+hhmm(d)}
function fmtN(n){return n>=10000?(n/1000).toFixed(n>=100000?0:1)+'k':String(n)}
function fmtMs(v){return v>=1000?(v/1000).toFixed(1)+' s':Math.round(v)+' ms'}
function fmtPct(p){if(p==null)return '—';if(p>=100)return '100%';var d=p>=99?2:1,f=Math.pow(10,d);return (Math.floor(p*f)/f).toFixed(d)+'%'}
function spanLbl(s){return s===60?'1 h':s===360?'6 h':s===1440?'24 h':'7 days'}
function quantile(s,q){if(!s.length)return null;var i=(s.length-1)*q,lo=Math.floor(i),hi=Math.ceil(i);return s[lo]+(s[hi]-s[lo])*(i-lo)}
function niceCeil(v){var p=Math.pow(10,Math.floor(Math.log10(v))),m=v/p;return (m<=1?1:m<=2?2:m<=2.5?2.5:m<=5?5:10)*p}
function decLat(c){return c===0?null:c<=150?(c-1)*2:300+(c-151)*25}
function nowMin(){return histNow?Math.floor((histNow*1000+(Date.now()-histAt))/60000):Math.floor(Date.now()/60000)}
function rec(m){var s=m%NM;if(recId[s]!==m||!recFl[s])return null;var f=recFl[s];
  return {lat:decLat(recLat[s]),n:f&7,fail:(f>>3)&7,nowifi:!!(f&64),rssi:recRs[s]}}

function loadHistory(){
  if(histLoading)return Promise.resolve();
  var full=!histFull||Date.now()-histFull>600000;
  histLoading=true;
  return fetch('/api/ping/history'+(!full&&histLast?'?from='+histLast:''),{cache:'no-store'}).then(function(r){
    if(!r.ok)throw new Error('HTTP '+r.status);return r.arrayBuffer();
  }).then(function(buf){
    var dv=new DataView(buf);
    if(buf.byteLength<20||dv.getUint8(0)!==80||dv.getUint8(3)!==50)throw new Error('bad history');
    histNow=dv.getUint32(8,true);histAt=Date.now();
    var first=dv.getUint32(12,true),count=dv.getUint32(16,true);
    for(var i=0;i<count&&20+i*3+2<buf.byteLength;i++){var m=first+i,s=m%NM,o=20+i*3;
      recId[s]=m;recLat[s]=dv.getUint8(o);recFl[s]=dv.getUint8(o+1);recRs[s]=dv.getInt8(o+2)}
    if(count)histLast=first+count-1;
    if(full)histFull=Date.now();
    return getJson('/api/ping/outages').then(function(o){OUT=o}).catch(function(){});
  }).then(function(){histLoading=false;renderNet()},function(){histLoading=false});
}

/* aggregate minutes [m0, m1) */
function agg(m0,m1){
  var a={n:0,fail:0,ok:0,sum:0,minutes:0,lats:[],jit:0,jn:0,prev:null,rs:0,rn:0,rmin:0,nowifi:0};
  for(var m=m0;m<m1;m++){var r=rec(m);if(!r){a.prev=null;continue}
    a.minutes++;a.n+=r.n;a.fail+=r.fail;var ok=r.n-r.fail;a.ok+=ok;if(r.nowifi)a.nowifi++;
    if(r.lat!=null&&ok>0){a.sum+=r.lat*ok;a.lats.push(r.lat);if(a.prev!=null){a.jit+=Math.abs(r.lat-a.prev);a.jn++}a.prev=r.lat}else a.prev=null;
    if(r.rssi<0){a.rs+=r.rssi;a.rn++;a.rmin=Math.min(a.rmin,r.rssi)}}
  a.lats.sort(function(x,y){return x-y});
  a.avg=a.ok?a.sum/a.ok:null;a.p95=quantile(a.lats,.95);a.med=quantile(a.lats,.5);a.max=a.lats.length?a.lats[a.lats.length-1]:null;
  a.jitter=a.jn?a.jit/a.jn:null;a.rssi=a.rn?a.rs/a.rn:null;
  return a;
}

/* shared tooltip */
var tip=$('tip');
function showTip(html,x,y){
  tip.innerHTML=html;tip.classList.add('show');
  var w=tip.offsetWidth,h=tip.offsetHeight;
  tip.style.left=Math.min(window.innerWidth-w-8,Math.max(8,x+14))+'px';
  tip.style.top=(y-h-12<8?y+16:y-h-12)+'px';
}
function hideTip(){tip.classList.remove('show')}

/* status header */
function renderPing(){
  var p=S.ping,ic=$('pingcircle'),state,t,sub,rn=relayName(p.relayTrigger);
  if(!p.checked){state='pending';t='Waiting for the first measurement';sub='The device checks every '+p.probeSec+' s, starting a few seconds after boot.'}
  else if(p.failCount===0){state='ok';t='Online';sub=p.ms+' ms · checked '+fmtAgo(p.lastAgo)+(p.grace>0?' · router restarted recently':'')}
  else if(p.grace>0){state='grace';t='Router restarted';sub='Waiting for it to boot · automatic restarts paused for '+fmtDur(p.grace)}
  else if(!p.down){state='fail';t='Unstable';sub='The last check got no response · '+rn+' is restarted only after '+fmtEvery(p.restartAfter)+' down'}
  else{state='fail';t=(p.noWifi?'No WiFi for ':'No internet for ')+fmtDur(p.downFor);
    sub=p.restartIn>0?(rn+' restarts in '+fmtDur(p.restartIn)+' if it does not come back'):('Restarting '+rn+'…')}
  ic.className='ping-indicator'+(state==='ok'?'':' '+state);
  ic.innerHTML=state==='ok'?ICO.check:state==='fail'?ICO.x:state==='grace'?ICO.pause:ICO.clock;
  ic.setAttribute('aria-label',t);
  $('netTitle').textContent=t;$('netSub').textContent=sub;
  $('pingdesc').innerHTML='Recorded by the device itself every <strong>'+p.probeSec+' s</strong> (TCP to <strong>'+esc(p.host)+':'+p.port+
    '</strong>), whether or not this page is open, and kept minute by minute for 7 days'+
    (p.stored?' in flash, so it survives restarts and updates.':'. <strong>No flash partition found: the history is lost when the ESP32 restarts.</strong>')+
    ' After <strong>'+fmtEvery(p.restartAfter)+'</strong> without internet it restarts <strong>'+esc(rn)+'</strong>, then waits for it to boot.';
  var b=$('pingBtn');
  if(pingWaiting&&!p.pending&&p.lastAgo>=0&&p.lastAgo<15){
    pingWaiting=false;
    toast(p.lastResult?('Internet OK · '+p.ms+' ms'):('No response from '+p.host),p.lastResult?'ok':'err');
  }
  b.disabled=pingWaiting;
  b.innerHTML=pingWaiting?ICO.spinner+'Checking…':'Check now';
}

/* ---------- view window ---------- */
function win(){var end=live?nowMin()+1:viewEnd;return [end-span,end]}
function setView(newSpan,end,mark){
  if(newSpan)span=newSpan;
  var nm=nowMin()+1,oldest=nm-NM;
  if(end==null||end>=nm){live=true;viewEnd=nm}else{live=false;viewEnd=Math.max(oldest+span,end)}
  if(mark!==undefined)markMin=mark;
  try{localStorage.setItem('pm-span',span)}catch(e){}
  [].forEach.call($('netRange').children,function(x){x.classList.toggle('on',+x.dataset.range===span)});
  renderNet();
}
function renderNav(){
  var w=win(),nm=nowMin()+1;
  $('navLbl').textContent=live?'Last '+spanLbl(span):(whenLbl(w[0])+' – '+(span>=1440?whenLbl(w[1]):hhmm(minDate(w[1]))));
  $('navNext').disabled=live;
  $('navPrev').disabled=w[0]<=nm-NM;
  $('navNow').hidden=live;
}
$('navPrev').innerHTML=SVG+'<path d="M15 18l-6-6 6-6"/></svg>';
$('navNext').innerHTML=SVG+'<path d="M9 18l6-6-6-6"/></svg>';
$('navPrev').addEventListener('click',function(){setView(0,win()[1]-span)});
$('navNext').addEventListener('click',function(){setView(0,win()[1]+span)});
$('navNow').addEventListener('click',function(){setView(0,null,null)});
$('netRange').addEventListener('click',function(e){
  var b=e.target.closest('button[data-range]');if(!b)return;
  var s=+b.dataset.range,w=win();
  // keep the inspected minute (or the window centre) in view when zooming
  var c=markMin!=null&&markMin>=w[0]&&markMin<w[1]?markMin:Math.round((w[0]+w[1])/2);
  setView(s,live&&markMin==null?null:c+Math.round(s/2));
});

/* ---------- KPI tiles ---------- */
function kpi(v,l,s){return '<div class="kpi"><div class="kv">'+v+'</div><div class="kl">'+l+'</div><div class="ks">'+(s||'&nbsp;')+'</div></div>'}
function outagesIn(m0,m1){
  var list=[];
  OUT.forEach(function(o){if(!o[0])return;var s=Math.floor(o[0]/60),e=s+Math.ceil(o[1]/60);if(e>=m0&&s<m1)list.push(o)});
  return list;
}
function renderKpis(a,w){
  var u='<span class="u">ms</span>',nm=nowMin()+1,wk=agg(nm-NM,nm),outs=outagesIn(w[0],w[1]),longest=0;
  outs.forEach(function(o){longest=Math.max(longest,o[1])});
  if(live&&S&&S.ping.down){outs.push(0);longest=Math.max(longest,S.ping.downFor)}
  $('netKpis').innerHTML=
    kpi(fmtPct(a.n?a.ok/a.n*100:null),'Uptime · this view',a.n?(a.fail+' of '+fmtN(a.n)+' checks failed'):'no data')+
    kpi(fmtPct(wk.n?wk.ok/wk.n*100:null),'Uptime · 7 days',wk.minutes?(Math.round(wk.minutes/60)+' h recorded'):'no data yet')+
    kpi(a.avg!=null?Math.round(a.avg)+u:'—','Avg latency',a.med!=null?'median '+Math.round(a.med)+' ms':'')+
    kpi(a.p95!=null?Math.round(a.p95)+u:'—','95th percentile',a.max!=null?'worst minute '+fmtMs(a.max):'')+
    kpi(a.jitter!=null?Math.round(a.jitter)+u:'—','Jitter','minute to minute')+
    kpi(String(outs.length),'Outages · this view',outs.length?'longest '+fmtDur(longest):'none')+
    kpi(a.rssi!=null?Math.round(a.rssi)+'<span class="u">dBm</span>':'—','WiFi signal',a.rn?'weakest '+a.rmin+' dBm':'');
}

/* ---------- latency chart ---------- */
function renderLatency(w){
  var el=$('latChart');
  $('latLegend').innerHTML='<span><i class="line" style="background:var(--accent)"></i>Avg latency</span><span><i style="background:var(--off)"></i>Failed checks</span><span><i style="background:var(--restart)"></i>No WiFi</span><span><i style="background:var(--surface2);box-shadow:inset 0 0 0 1px var(--border2)"></i>No data</span>';
  if(!histNow){el.innerHTML='<div class="empty-msg">Waiting for the device clock (NTP) to date the history.</div>';el.onpointermove=el.onclick=null;return}
  var W=el.clientWidth||600,Hh=W<500?180:220,pL=44,pR=8,pT=14,pB=22,pw=W-pL-pR,ph=Hh-pT-pB;
  var m0=w[0],m1=w[1],nb=Math.max(1,Math.min(span,Math.floor(pw/2.5))),bpx=pw/nb,B=[],okv=[];
  for(var k=0;k<nb;k++)B.push({m0:m0+Math.floor(k*span/nb),m1:m0+Math.floor((k+1)*span/nb)});
  B.forEach(function(b){var a=agg(b.m0,b.m1);b.a=a;if(a.avg!=null)okv.push(a.avg)});
  okv.sort(function(x,y){return x-y});
  var ymax=niceCeil(Math.max(40,(quantile(okv,.98)||40)*1.25));
  function X(m){return pL+(m-m0)/span*pw}
  function Y(v){return pT+ph-Math.min(v,ymax)/ymax*ph}
  var s='<svg viewBox="0 0 '+W+' '+Hh+'" height="'+Hh+'" role="img" aria-label="Latency and failed checks">';
  for(var g=0;g<=4;g++){var gv=ymax*g/4,gy=Y(gv);
    s+='<line x1="'+pL+'" x2="'+(W-pR)+'" y1="'+gy+'" y2="'+gy+'" style="stroke:var(--border)"/><text x="'+(pL-6)+'" y="'+(gy+3)+'" text-anchor="end">'+(g?fmtMs(gv):'0')+'</text>'}
  // no-data stretches
  var gs=-1;
  function gap(i0,i1){var x0=pL+i0*bpx,x1=pL+i1*bpx;s+='<rect x="'+x0+'" y="'+pT+'" width="'+(x1-x0)+'" height="'+ph+'" style="fill:var(--surface2)"/>';
    if(x1-x0>70)s+='<text x="'+((x0+x1)/2)+'" y="'+(pT+ph/2)+'" text-anchor="middle">no data</text>'}
  B.forEach(function(b,i){if(!b.a.minutes){if(gs<0)gs=i}else if(gs>=0){gap(gs,i);gs=-1}});
  if(gs>=0&&B[nb-1].m0<nowMin())gap(gs,nb);
  // failed checks: band opacity = share of failed checks
  B.forEach(function(b,i){var a=b.a;if(!a.fail)return;
    var c=a.nowifi?'var(--restart)':'var(--off)',bx=pL+i*bpx,ww=Math.max(2,bpx),fr=a.fail/a.n;
    s+='<rect x="'+bx+'" y="'+pT+'" width="'+ww+'" height="'+ph+'" style="fill:'+c+'" opacity="'+(0.1+0.35*fr).toFixed(2)+'"/>'+
       '<rect x="'+bx+'" y="'+(pT+ph-4)+'" width="'+ww+'" height="4" rx="1" style="fill:'+c+'"/>'});
  var line='',area='',seg=[];
  function flush(){if(!seg.length)return;var pts=seg.map(function(p){return p[0].toFixed(1)+' '+p[1].toFixed(1)}).join('L');
    line+='M'+pts;area+='M'+seg[0][0].toFixed(1)+' '+(pT+ph)+'L'+pts+'L'+seg[seg.length-1][0].toFixed(1)+' '+(pT+ph)+'Z';seg=[]}
  B.forEach(function(b,i){if(b.a.avg!=null)seg.push([pL+(i+.5)*bpx,Y(b.a.avg)]);else flush()});flush();
  s+='<path d="'+area+'" style="fill:var(--accent)" opacity=".12"/><path d="'+line+'" fill="none" style="stroke:var(--accent)" stroke-width="2" stroke-linejoin="round" stroke-linecap="round"/>';
  // time axis
  var iv=span===60?10:span===360?60:span===1440?240:1440,d=minDate(m0),ticks=[];
  if(iv>=1440){ // local midnights (DST-safe)
    for(var dd=new Date(d.getFullYear(),d.getMonth(),d.getDate()+1);dd.getTime()<=m1*60000;dd=new Date(dd.getFullYear(),dd.getMonth(),dd.getDate()+1))ticks.push([Math.floor(dd.getTime()/60000),DOW[dd.getDay()]+' '+dd.getDate()]);
  }else{
    var mm=d.getHours()*60+d.getMinutes();
    for(var t=m0+((iv-mm%iv)%iv);t<=m1;t+=iv)ticks.push([t,hhmm(minDate(t))]);
  }
  ticks.forEach(function(k){var tx=X(k[0]);if(tx<pL+18||tx>W-pR-18)return;
    s+='<line x1="'+tx+'" x2="'+tx+'" y1="'+(pT+ph)+'" y2="'+(pT+ph+4)+'" style="stroke:var(--border2)"/><text x="'+tx+'" y="'+(Hh-6)+'" text-anchor="middle">'+k[1]+'</text>'});
  // inspected minute
  if(markMin!=null&&markMin>=m0&&markMin<m1){var mx=X(markMin+.5);
    s+='<line x1="'+mx+'" x2="'+mx+'" y1="'+(pT-2)+'" y2="'+(pT+ph)+'" style="stroke:var(--text)" stroke-width="1.5"/>'+
       '<text x="'+Math.min(W-pR-24,Math.max(pL+24,mx))+'" y="'+(pT-4)+'" text-anchor="middle" style="fill:var(--text);font-weight:700">'+hhmm(minDate(markMin))+'</text>'}
  s+='<line id="latX" x1="0" x2="0" y1="'+pT+'" y2="'+(pT+ph)+'" style="stroke:var(--text3)" visibility="hidden"/>'+
     '<circle id="latDot" r="4" style="fill:var(--accent);stroke:var(--surface)" stroke-width="2" visibility="hidden"/></svg>';
  el.innerHTML=s;
  var cx=$('latX'),dot=$('latDot');
  function idx(e){var r=el.getBoundingClientRect();return Math.floor(((e.clientX-r.left)*W/r.width-pL)/bpx)}
  function off(){cx.setAttribute('visibility','hidden');dot.setAttribute('visibility','hidden');hideTip()}
  el.onpointerleave=off;
  el.onpointermove=function(e){
    var i=idx(e);if(i<0||i>=nb){off();return}
    var b=B[i],a=b.a,mx=pL+(i+.5)*bpx;
    cx.setAttribute('x1',mx);cx.setAttribute('x2',mx);cx.setAttribute('visibility','visible');
    if(a.avg!=null){dot.setAttribute('cx',mx);dot.setAttribute('cy',Y(a.avg));dot.setAttribute('visibility','visible')}else dot.setAttribute('visibility','hidden');
    var h='<div class="tm">'+(b.m1-b.m0>1?hhmm(minDate(b.m0))+' – '+hhmm(minDate(b.m1)):dayLbl(minDate(b.m0))+' '+hhmm(minDate(b.m0)))+'</div>';
    if(!a.minutes)h+='No data (device off or restarting)';
    else{
      if(a.avg!=null)h+='<b>'+fmtMs(a.avg)+'</b>'+(b.m1-b.m0>1&&a.max!=null?' avg · worst '+fmtMs(a.max):'');
      h+='<div>'+(a.n-a.fail)+' of '+a.n+' checks OK</div>';
      if(a.fail)h+='<div style="color:'+(a.nowifi?'var(--restart)':'var(--off)')+'">'+a.fail+' failed'+(a.nowifi?' (ESP32 without WiFi)':'')+'</div>';
      if(a.rssi!=null)h+='<div class="tm">WiFi '+Math.round(a.rssi)+' dBm</div>';
    }
    showTip(h+'<div class="tm">click to inspect</div>',e.clientX,e.clientY);
  };
  el.onclick=function(e){var i=idx(e);if(i<0||i>=nb)return;var b=B[i];
    var m=b.m0;for(var q=b.m0;q<b.m1;q++){var r=rec(q);if(r&&r.fail){m=q;break}}
    inspect(m,false)};
}

/* ---------- verdict for one minute ---------- */
function runAround(m,test,limit){var a=m,b=m;
  while(a-1>m-limit&&test(a-1))a--;while(b+1<m+limit&&test(b+1))b++;return [a,b]}
function verdict(m){
  var r=rec(m),when=whenLbl(m),nm=nowMin();
  if(m>nm)return {k:'none',t:'That moment is in the future.'};
  if(m<nm-NM+1)return {k:'none',t:'The device keeps the last 7 days only.'};
  if(!histNow)return {k:'none',t:'The device clock is not synced yet, so the history cannot be dated.'};
  if(!r){var g=runAround(m,function(q){return !rec(q)&&q<=nm},NM);
    return {k:'none',t:'<b>No data for '+when+'.</b>',s:'The device was off, restarting or without a clock from '+whenLbl(g[0])+' to '+whenLbl(g[1]+1)+' ('+fmtDur((g[1]-g[0]+1)*60)+'). It cannot tell whether the internet worked then.'}}
  if(!r.fail){
    var ctx=null;for(var dlt=1;dlt<=15&&!ctx;dlt++){[m-dlt,m+dlt].forEach(function(q){var x=rec(q);if(!ctx&&x&&x.fail)ctx=q})}
    return {k:'ok',t:'<b>Internet was working at '+when+'.</b> '+r.n+' of '+r.n+' checks answered'+(r.lat!=null?', '+fmtMs(r.lat)+' average':'')+(r.rssi<0?', WiFi '+r.rssi+' dBm':'')+'.',
      s:ctx==null?'No failed checks in the 15 minutes before or after, so the problem you saw was probably not the internet connection.':'Nearest failed check: '+whenLbl(ctx)+' ('+Math.abs(ctx-m)+' min '+(ctx<m?'before':'after')+').'}}
  // episode = failing minutes, bridging gaps of up to 3 good minutes (intermittent drops)
  function bad(q){var x=rec(q);return x&&x.fail>0}
  var run=[m,m],g,q;
  for(q=m-1,g=0;q>m-600&&g<=3;q--){if(bad(q)){run[0]=q;g=0}else g++}
  for(q=m+1,g=0;q<m+600&&q<=nm&&g<=3;q++){if(bad(q)){run[1]=q;g=0}else g++}
  var n=0,f=0,wifi=false,full=true;
  for(q=run[0];q<=run[1];q++){var x=rec(q);if(!x)continue;n+=x.n;f+=x.fail;if(x.nowifi)wifi=true;if(x.fail<x.n)full=false}
  var dur=(run[1]-run[0]+1)*60,from=whenLbl(run[0]),to=hhmm(minDate(run[1]+1));
  var o=OUT.filter(function(o){var s=Math.floor(o[0]/60);return o[0]&&s<=run[1]+1&&s+Math.ceil(o[1]/60)>=run[0]})[0];
  var extra=o&&o[2]?' The router was restarted automatically during this outage.':'';
  if(wifi)return {k:'warn',t:'<b>The ESP32 had lost WiFi</b> from '+from+' to '+to+' ('+fmtDur(dur)+').',
    s:'Probably a WiFi/router problem rather than the internet line: '+f+' of '+n+' checks failed.'+extra};
  if(full)return {k:'crit',t:'<b>No internet</b> from '+from+' to '+to+' ('+fmtDur(dur)+').',s:'All '+n+' checks in that period failed while the WiFi was connected.'+extra};
  return {k:'warn',t:'<b>Unstable internet</b> from '+from+' to '+to+' ('+fmtDur(dur)+').',s:f+' of '+n+' checks failed; the connection was dropping intermittently.'+extra};
}
function inspect(m,recentre){
  markMin=m;
  if(recentre!==false){var w=win();if(m<w[0]||m>=w[1]||span>360)setView(span>360?60:span,m+Math.round((span>360?60:span)/2));else renderNet()}
  else renderNet();
}
function renderVerdict(){
  var el=$('netVerdict');
  if(markMin==null){el.hidden=true;return}
  var v=verdict(markMin),ic={ok:ICO.check,warn:ICO.warn,crit:ICO.x,none:ICO.clock}[v.k];
  el.className='verdict '+v.k;el.hidden=false;
  el.innerHTML='<div class="vi">'+ic+'</div><div><div>'+v.t+'</div>'+(v.s?'<div class="vs">'+v.s+'</div>':'')+'</div><button class="vx" id="vClose" aria-label="Close">×</button>';
  $('vClose').onclick=function(){markMin=null;renderNet()};
}
$('lookForm').addEventListener('submit',function(e){
  e.preventDefault();
  var v=$('lookAt').value;if(!v)return;
  var d=new Date(v);if(isNaN(d))return;
  inspect(Math.floor(d.getTime()/60000));
});
function setLookLimits(){
  var nm=nowMin();function iso(m){var d=minDate(m);return d.getFullYear()+'-'+pad2(d.getMonth()+1)+'-'+pad2(d.getDate())+'T'+pad2(d.getHours())+':'+pad2(d.getMinutes())}
  $('lookAt').min=iso(nm-NM+1);$('lookAt').max=iso(nm);
  if(!$('lookAt').value)$('lookAt').value=iso(nm);
}

/* ---------- 7 x 24 heatmap ---------- */
function renderHeatmap(){
  var el=$('heatmap'),lg=$('heatLegend');
  lg.innerHTML='<span><i style="background:var(--on)"></i>≥ 99% of checks OK</span><span><i style="background:var(--restart)"></i>90 – 99%</span><span><i style="background:var(--off)"></i>below 90%</span><span><i style="background:var(--surface3);box-shadow:inset 0 0 0 1px var(--border2)"></i>no data</span>';
  if(!histNow){el.innerHTML='<div class="empty-msg">Waiting for the device clock (NTP).</div>';return}
  var nm=nowMin(),W=el.clientWidth||400,lw=62,gap=2,cw=(W-lw)/24,ch=Math.max(13,Math.min(20,cw)),Hh=7*(ch+gap)+18,w=win();
  var s='<svg viewBox="0 0 '+W+' '+Hh+'" height="'+Hh+'" role="img" aria-label="Last 7 days hour by hour">';
  var today=new Date();today.setHours(0,0,0,0);
  for(var r=0;r<7;r++){
    var day=new Date(today.getFullYear(),today.getMonth(),today.getDate()-6+r),y=r*(ch+gap);
    s+='<text x="0" y="'+(y+ch/2+3.5)+'">'+(r===6?'Today':r===5?'Yesterday':dayLbl(day))+'</text>';
    for(var hr=0;hr<24;hr++){
      var hm=Math.floor(new Date(day.getFullYear(),day.getMonth(),day.getDate(),hr).getTime()/60000);
      if(hm>nm)continue;
      var a=agg(hm,hm+60),fill='var(--surface3)',lbl=dayLbl(minDate(hm))+', '+pad2(hr)+':00 – '+pad2((hr+1)%24)+':00|';
      if(a.n){var up=a.ok/a.n*100;fill=up>=99?'var(--on)':up>=90?'var(--restart)':'var(--off)';
        lbl+='<b>'+fmtPct(up)+' of checks OK</b>'+(a.fail?' · '+a.fail+' failed':'')+'|'+(a.avg!=null?'avg '+Math.round(a.avg)+' ms · ':'')+a.minutes+' of 60 min recorded'}
      else lbl+='No data';
      var sel=hm<w[1]&&hm+60>w[0]&&span<=60;
      s+='<rect class="cell" x="'+(lw+hr*cw+gap/2).toFixed(1)+'" y="'+y+'" width="'+Math.max(2,cw-gap).toFixed(1)+'" height="'+ch+'" rx="3" style="fill:'+fill+(sel?';stroke:var(--text);stroke-width:2':'')+';cursor:pointer" data-m="'+hm+'" data-tip="'+esc(lbl)+'"/>';
    }
  }
  [0,6,12,18].forEach(function(hr){s+='<text x="'+(lw+hr*cw+gap/2)+'" y="'+(Hh-3)+'">'+pad2(hr)+':00</text>'});
  el.innerHTML=s+'</svg>';
  el.onpointermove=function(e){var t=e.target.getAttribute&&e.target.getAttribute('data-tip');if(!t){hideTip();return}
    var p=t.split('|');showTip('<div class="tm">'+p[0]+'</div>'+p.slice(1).join('<br>')+'<div class="tm">click to open this hour</div>',e.clientX,e.clientY)};
  el.onpointerleave=hideTip;
  el.onclick=function(e){var m=e.target.getAttribute&&e.target.getAttribute('data-m');if(!m)return;hideTip();setView(60,+m+60,null)};
}

/* ---------- latency distribution (not time-based) ---------- */
var BINS=[[0,20,'0–20','<20'],[20,40,'20–40','<40'],[40,60,'40–60','<60'],[60,100,'60–100','<100'],[100,200,'100–200','<200'],[200,500,'200–500','<500'],[500,1e9,'500+','500+']];
function renderHist(a){
  var el=$('histChart');
  $('histNote').textContent='minute averages in this view';
  if(!a.lats.length){el.innerHTML='<div class="empty-msg">No successful checks in this view.</div>';el.onpointermove=null;return}
  var c=BINS.map(function(){return 0});
  a.lats.forEach(function(v){for(var i=0;i<BINS.length;i++)if(v<BINS[i][1]){c[i]++;break}});
  var tot=a.lats.length,W=el.clientWidth||300,Hh=170,pL=8,pR=8,pT=18,pB=34,pw=W-pL-pR,ph=Hh-pT-pB,slot=pw/BINS.length,bw=Math.min(44,slot-6);
  var mx=Math.max.apply(null,c),top=c.indexOf(mx),short=slot<52;
  var s='<svg viewBox="0 0 '+W+' '+Hh+'" height="'+Hh+'" role="img" aria-label="Latency distribution"><line x1="'+pL+'" x2="'+(W-pR)+'" y1="'+(pT+ph)+'" y2="'+(pT+ph)+'" style="stroke:var(--border2)"/>';
  c.forEach(function(v,i){
    var h=v?Math.max(3,v/mx*ph):0,x=pL+i*slot+(slot-bw)/2,y=pT+ph-h,r=Math.min(4,h/2,bw/2),pc=v/tot*100;
    if(h)s+='<path class="bar" d="M'+x+' '+(pT+ph)+'V'+(y+r)+'Q'+x+' '+y+' '+(x+r)+' '+y+'H'+(x+bw-r)+'Q'+(x+bw)+' '+y+' '+(x+bw)+' '+(y+r)+'V'+(pT+ph)+'Z" style="fill:var(--accent)" data-tip="'+BINS[i][2]+' ms|<b>'+v+'</b> minutes · '+pc.toFixed(1)+'%"/>';
    if(i===top)s+='<text x="'+(x+bw/2)+'" y="'+(y-5)+'" text-anchor="middle" style="fill:var(--text2);font-weight:600">'+Math.round(pc)+'%</text>';
    s+='<text x="'+(x+bw/2)+'" y="'+(pT+ph+14)+'" text-anchor="middle">'+BINS[i][short?3:2]+'</text>';
  });
  el.innerHTML=s+'<text x="'+(W/2)+'" y="'+(Hh-3)+'" text-anchor="middle">latency (ms)</text></svg>';
  el.onpointermove=function(e){var t=e.target.getAttribute&&e.target.getAttribute('data-tip');if(!t){hideTip();return}
    var p=t.split('|');showTip('<div class="tm">'+p[0]+'</div>'+p[1],e.clientX,e.clientY)};
  el.onpointerleave=hideTip;
}

/* ---------- outages ---------- */
function renderOutages(){
  var rows=[],p=S&&S.ping;
  if(p&&p.down)rows.push({start:Date.now()-p.downFor*1000,dur:p.downFor,restarted:p.restartIn===0,noWifi:p.noWifi,live:true});
  OUT.forEach(function(o){rows.push({start:o[0]?o[0]*1000:null,dur:o[1],restarted:o[2],noWifi:o[3]})});
  $('outNote').textContent=OUT.length?('last '+OUT.length+' saved on the device · click one to see it'):'';
  if(!rows.length){$('outages').innerHTML='<div class="empty-msg">No outages recorded. Isolated failed checks still show in the chart.</div>';return}
  $('outages').innerHTML=rows.slice(0,10).map(function(o){
    var d=o.start?new Date(o.start):null;
    return '<div class="out-row"'+(d?' data-start="'+Math.floor(o.start/60000)+'" data-dur="'+o.dur+'" tabindex="0"':'')+'><span class="ot">'+(d?dayLbl(d)+' '+hhmm(d):'before clock sync')+'</span><span class="od">'+fmtDur(o.dur)+'</span>'+
      (o.live?'<span class="tag live">Ongoing</span>':'')+
      (o.noWifi?'<span class="tag warn">No WiFi</span>':'<span class="tag crit">No internet</span>')+
      (o.restarted?'<span class="tag">Router restarted</span>':'')+'</div>';
  }).join('');
}
function openOutage(row){
  var st=+row.dataset.start,dur=Math.ceil(+row.dataset.dur/60),need=dur+30;
  var sp=need<=60?60:need<=360?360:1440;
  markMin=st;setView(sp,st+Math.round(dur/2)+Math.round(sp/2));
  $('latChart').scrollIntoView({behavior:'smooth',block:'center'});
}
$('outages').addEventListener('click',function(e){var r=e.target.closest('.out-row[data-start]');if(r)openOutage(r)});
$('outages').addEventListener('keydown',function(e){var r=e.target.closest('.out-row[data-start]');if(r&&(e.key==='Enter'||e.key===' ')){e.preventDefault();openOutage(r)}});

function renderNet(){
  if(!S)return;
  var w=win(),a=histNow?agg(w[0],w[1]):agg(0,0);
  renderNav();renderKpis(a,w);renderLatency(w);renderHeatmap();renderHist(a);renderOutages();renderVerdict();setLookLimits();
}
[].forEach.call($('netRange').children,function(x){x.classList.toggle('on',+x.dataset.range===span)});
var rsT;window.addEventListener('resize',function(){clearTimeout(rsT);rsT=setTimeout(renderNet,200)});
window.addEventListener('scroll',hideTip,{passive:true});

/* ---------- log ---------- */
function renderLog(list){
  var l=$('loglist');
  if(!list.length){l.innerHTML='<div class="empty-msg">No events</div>';return}
  var fresh=lastLogLen?Math.max(0,Math.min(list.length,(S?S.logSeq:0)-lastLogSeqRendered)):0;
  lastLogLen=list.length;
  l.innerHTML=list.slice().reverse().map(function(e,i){
    return '<div class="log-item'+(i<fresh?' new':'')+'"><span class="log-t">'+esc(e.t)+'</span><span class="log-m">'+esc(e.m)+'</span></div>';
  }).join('');
}
var lastLogSeqRendered=0;

/* ---------- header / footer ---------- */
function renderHeader(){
  $('ipsub').textContent=S.ip;
  $('datetime').textContent=S.synced?S.datetime:'clock not synced';
  $('timepill').classList.toggle('warn',!S.synced);
  $('uptime').textContent=fmtDur(S.uptime);
  var pct=wifiPct(S.wifi_rssi);
  $('wifipct').textContent=pct+'%';
  $('wifipill').title='WiFi signal: '+S.wifi_rssi+' dBm';
  var on=pct>=85?4:pct>=60?3:pct>=30?2:pct>0?1:0,bars=$('wifibars').children;
  for(var i=0;i<bars.length;i++)bars[i].classList.toggle('on',i<on);
  $('footer').innerHTML='<span>Firmware '+esc(S.build)+'</span><span>Free memory '+Math.round(S.heap/1024)+' KB</span><span>'+esc(S.ip)+'</span>';
}

/* ---------- connection status ---------- */
function setOnline(){
  consecutiveFails=0;pollDelay=3000;
  $('conndot').className='dot live';
  $('connlabel').textContent='Live';
  $('offlineBanner').classList.remove('show');
  document.body.classList.remove('is-offline');
}
function setOffline(err){
  consecutiveFails++;
  pollDelay=Math.min(10000,3000+consecutiveFails*1500);
  $('conndot').className='dot bad';
  $('connlabel').textContent='Offline';
  if(consecutiveFails>=2){
    $('offlineMsg').textContent=(err&&/401/.test(err.message))?'Not authorized. Reload the page to log in.':'Lost connection to the device. Retrying…';
    $('offlineBanner').classList.add('show');
    document.body.classList.add('is-offline');
  }
}

/* ---------- refresh cycle ---------- */
function refresh(manual){
  if(fetching)return Promise.resolve();
  fetching=true;
  if(manual)$('refreshBtn').classList.add('spinning');
  return getJson('/api/status').then(function(d){
    S=d;
    setOnline();
    renderHeader();
    renderRelays();
    renderWol();
    renderPing();
    var jobs=[];
    // internet history: fetch the new minutes whenever the device recorded a check
    if(d.ping.seq!==histSeq||!histAt){histSeq=d.ping.seq;jobs.push(loadHistory())}
    else if(histAt)renderOutages();
    // schedules rarely change: fetch on load, every minute, or when forced
    if(manual||!SCH||Date.now()-lastSchedFetch>60000){
      jobs.push(getJson('/api/schedule').then(function(s){SCH=s;lastSchedFetch=Date.now()}));
    }
    // the log only when something new was added
    if(d.logSeq!==lastLogSeq){
      var seq=d.logSeq;
      jobs.push(getJson('/api/log').then(function(l){
        renderLog(l);lastLogSeqRendered=seq;lastLogSeq=seq;
      }));
    }
    return Promise.all(jobs).catch(function(){}).then(renderSchedules);
  }).catch(function(e){
    setOffline(e);
  }).then(function(){
    fetching=false;
    if(manual)$('refreshBtn').classList.remove('spinning');
  });
}
function scheduleNext(){
  clearTimeout(pollTimer);
  pollTimer=setTimeout(function(){
    if(document.hidden){scheduleNext();return}
    refresh().then(scheduleNext);
  },pollDelay);
}
document.addEventListener('visibilitychange',function(){if(!document.hidden)refresh()});

/* ---------- actions ---------- */
function restartRelay(id){
  var r=S.relays[id];if(!r)return;
  var msg='The relay will cut power for '+r.restartSec+' s and then turn it back on automatically.';
  if(S.ping&&S.ping.relayTrigger===id)msg+=' This is the router relay: the internet (and maybe this page) will be down for a few minutes.';
  confirmModal('Restart '+r.name+'?',msg,'Restart').then(function(ok){
    if(!ok)return;
    pending[id]=Date.now();
    relaySig='';renderRelays();
    postJson('/api/relays/restart?id='+id).then(function(){
      toast('Restarting '+r.name+'…','ok');
    }).catch(function(e){
      delete pending[id];
      toast('Could not restart '+r.name+': '+e.message,'err');
    }).then(function(){setTimeout(function(){refresh()},400)});
  });
}
function wakeWol(id,btn){
  var x=S.wol[id];if(!x)return;
  btn.disabled=true;
  btn.innerHTML=ICO.spinner+'<span>Sending</span>';
  postJson('/api/wol/wake?id='+id).then(function(){
    toast('Magic packet sent to '+x.name+'. It may take a minute to boot.','ok');
  }).catch(function(e){
    toast('Could not wake '+x.name+': '+e.message,'err');
  }).then(function(){
    wolSig='';
    setTimeout(function(){refresh()},800);
  });
}
function checkPing(){
  pingWaiting=true;
  renderPing();
  postJson('/api/ping/check').then(function(){
    setTimeout(function(){refresh()},1500);
    setTimeout(function(){if(pingWaiting){refresh()}},4500);
    setTimeout(function(){if(pingWaiting){pingWaiting=false;renderPing()}},9000);
  }).catch(function(e){
    pingWaiting=false;renderPing();
    toast('Check failed: '+e.message,'err');
  });
}

$('refreshBtn').addEventListener('click',function(){refresh(true)});
$('retryBtn').addEventListener('click',function(){refresh(true)});
document.addEventListener('click',function(e){
  var btn=e.target.closest('[data-action]');
  if(!btn||btn.disabled||!S)return;
  var id=parseInt(btn.dataset.id,10),a=btn.dataset.action;
  if(a==='restart')restartRelay(id);
  else if(a==='wake')wakeWol(id,btn);
  else if(a==='ping')checkPing();
});

setInterval(function(){if(S){tickCountdowns()}},1000);
setInterval(function(){if(SCH)renderSchedules()},30000);

refresh().then(scheduleNext);
})();
</script>
</body>
</html>
)rawliteral";

// ============================================================
// Helpers
// ============================================================

// Optional HTTP Basic Auth (WEB_AUTH_USER / WEB_AUTH_PASS in config.h).
static bool authOk(AsyncWebServerRequest *req) {
    if (strlen(WEB_AUTH_USER) == 0) return true;
    if (req->authenticate(WEB_AUTH_USER, WEB_AUTH_PASS)) return true;
    req->requestAuthentication("PowerManager");
    return false;
}
#define REQUIRE_AUTH(req) do { if (!authOk(req)) return; } while (0)

static void sendJson(AsyncWebServerRequest *req, int code, const String &json) {
    AsyncWebServerResponse *res = req->beginResponse(code, "application/json", json);
    res->addHeader("Access-Control-Allow-Origin", "*");
    res->addHeader("Cache-Control", "no-store");
    req->send(res);
}

static void sendJsonResult(AsyncWebServerRequest *req, int code, const char *error) {
    String json = "{\"ok\":";
    json += (code < 300) ? "true" : "false";
    if (error) {
        json += ",\"error\":\"";
        json += jsonEscape(error);
        json += "\"";
    }
    json += "}";
    sendJson(req, code, json);
}

static void sendHtml(AsyncWebServerRequest *req, int code, const String &html) {
    AsyncWebServerResponse *res = req->beginResponse(code, "text/html", html);
    res->addHeader("Cache-Control", "no-store");
    req->send(res);
}

static int getParamInt(AsyncWebServerRequest *req, const char* name, int def) {
    if (!req->hasParam(name)) return def;
    String v = req->getParam(name)->value();
    if (v.length() == 0) return def;
    for (size_t i = 0; i < v.length(); i++) {
        if (!isDigit(v[i])) return def;   // "abc".toInt() would silently be 0
    }
    return v.toInt();
}

// ============================================================
// JSON API
// ============================================================

static void handleGetStatus(AsyncWebServerRequest *req) {
    REQUIRE_AUTH(req);
    struct tm t;
    bool synced = timeNow(t);
    char dateTime[40] = "?";
    if (synced) {
        static const char* days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
        static const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
        snprintf(dateTime, sizeof(dateTime), "%s %02d %s %04d  %02d:%02d:%02d",
            days[t.tm_wday], t.tm_mday, months[t.tm_mon],
            t.tm_year + 1900, t.tm_hour, t.tm_min, t.tm_sec);
    }
    String json;
    json.reserve(1600);
    json = "{";
    json += "\"uptime\":" + String(uptimeSec());
    json += ",\"datetime\":\"" + String(dateTime) + "\"";
    json += ",\"synced\":" + String(synced ? "true" : "false");
    json += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
    json += ",\"wifi_rssi\":" + String(WiFi.RSSI());
    json += ",\"heap\":" + String(ESP.getFreeHeap());
    json += ",\"build\":\"" __DATE__ " " __TIME__ "\"";
    json += ",\"logSeq\":" + String(logGetSeq());
    json += ",\"relays\":" + relayStatusJson();
    json += ",\"wol\":" + wolStatusJson();
    json += ",\"ping\":" + pingStatusJson();
    json += "}";
    sendJson(req, 200, json);
}

// POST /api/relays/restart?id=X  -> {"ok":true}
static void handlePostRestart(AsyncWebServerRequest *req) {
    REQUIRE_AUTH(req);
    int id = getParamInt(req, "id", -1);
    if (id < 0 || id >= RELAY_COUNT) { sendJsonResult(req, 400, "invalid relay id"); return; }
    if (appConfig.relays[id].restarting) { sendJsonResult(req, 409, "already restarting"); return; }
    if (!actionPush(ACTION_RESTART, id, "web")) { sendJsonResult(req, 503, "busy, try again"); return; }
    sendJsonResult(req, 200, nullptr);
}

// POST /api/wol/wake?id=X  -> {"ok":true}
static void handlePostWake(AsyncWebServerRequest *req) {
    REQUIRE_AUTH(req);
    int id = getParamInt(req, "id", -1);
    if (!wolValidId(id)) { sendJsonResult(req, 400, "invalid device id"); return; }
    if (WiFi.status() != WL_CONNECTED) { sendJsonResult(req, 503, "WiFi not connected"); return; }
    if (!actionPush(ACTION_WOL, id, "web")) { sendJsonResult(req, 503, "busy, try again"); return; }
    sendJsonResult(req, 200, nullptr);
}

// GET /api/ping/history[?from=epochMinute] -> binary, see ping.h
static void handlePingHistory(AsyncWebServerRequest *req) {
    REQUIRE_AUTH(req);
    uint32_t from = 0;
    if (req->hasParam("from")) from = strtoul(req->getParam("from")->value().c_str(), nullptr, 10);
    uint32_t first = 0;
    uint32_t count = netHistoryRange(from, first);
    std::array<uint8_t, NET_HDR_LEN> h = {'P', 'M', 'H', '2', 2, (uint8_t)PING_PROBE_SEC, 0, 0};
    uint32_t nowE = timeSynced() ? (uint32_t)time(nullptr) : 0;
    memcpy(h.data() + 8, &nowE, 4);
    memcpy(h.data() + 12, &first, 4);
    memcpy(h.data() + 16, &count, 4);
    AsyncWebServerResponse *res = req->beginResponse("application/octet-stream", NET_HDR_LEN + (size_t)count * 3,
        [h, first, count](uint8_t *buf, size_t maxLen, size_t index) -> size_t {
            return netHistoryFill(h.data(), first, count, buf, maxLen, index);
        });
    res->addHeader("Access-Control-Allow-Origin", "*");
    res->addHeader("Cache-Control", "no-store");
    req->send(res);
}

// POST /api/ping/check  -> {"ok":true}; the result shows up in /api/status
static void handlePostPingCheck(AsyncWebServerRequest *req) {
    REQUIRE_AUTH(req);
    if (!actionPush(ACTION_PING_NOW, -1, "web")) { sendJsonResult(req, 503, "busy, try again"); return; }
    sendJsonResult(req, 200, nullptr);
}

// ============================================================
// HTML action pages (GET, for browsers / shortcuts / NFC tags)
// ============================================================

static const char PAGE_CSS[] PROGMEM =
    "*{margin:0;padding:0;box-sizing:border-box}"
    ":root{--bg:#0b0d13;--surface:#161923;--border:#2a2f42;--border2:#3a4058;--text:#eceef2;--text3:#666c82;--accent:#6366f1;"
    "--warn:#f59e0b;--warn-bg:rgba(245,158,11,.12);--ok:#22c55e;--ok-bg:rgba(34,197,94,.12);--err:#ef4444;--err-bg:rgba(239,68,68,.12)}"
    "@media(prefers-color-scheme:light){:root{--bg:#f4f5f9;--surface:#fff;--border:#e1e3ec;--border2:#cdd0e0;--text:#181a24;--text3:#868ca0;"
    "--accent:#5457e8;--warn:#c2660a;--ok:#15803d;--err:#dc2626}}"
    "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:var(--bg);color:var(--text);min-height:100vh;"
    "display:flex;align-items:center;justify-content:center;padding:24px}"
    ".card{background:var(--surface);border:1px solid var(--border);border-radius:16px;padding:32px 20px;width:100%;max-width:340px;text-align:center}"
    ".ic{width:64px;height:64px;border-radius:16px;margin:0 auto 16px;display:flex;align-items:center;justify-content:center;border:2px solid currentColor}"
    ".ic svg{width:30px;height:30px}.warn{color:var(--warn);background:var(--warn-bg)}.ok{color:var(--ok);background:var(--ok-bg)}.err{color:var(--err);background:var(--err-bg)}"
    "h1{font-size:1.05rem;font-weight:700;margin-bottom:4px}.sub{font-size:.75rem;color:var(--text3);margin-bottom:16px;font-family:'SF Mono',Consolas,monospace}"
    ".big{font-size:2.2rem;font-weight:700;font-variant-numeric:tabular-nums;line-height:1.1}.lbl{font-size:.62rem;color:var(--text3);text-transform:uppercase;letter-spacing:1px;margin-top:4px}"
    "p{font-size:.85rem;color:var(--text3);line-height:1.5}"
    "a{display:inline-block;margin-top:24px;color:var(--accent);text-decoration:none;font-size:.85rem;font-weight:600;border:1px solid var(--border2);padding:10px 24px;border-radius:10px}"
    "@keyframes spin{to{transform:rotate(360deg)}}.spin{animation:spin 1.2s linear infinite}";

static const char ICON_OK[]   = "<svg viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2.5' stroke-linecap='round' stroke-linejoin='round'><path d='M20 6 9 17l-5-5'/></svg>";
static const char ICON_ERR[]  = "<svg viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2.5' stroke-linecap='round'><path d='M18 6 6 18M6 6l12 12'/></svg>";
static const char ICON_SPIN[] = "<svg class='spin' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2.5' stroke-linecap='round'><path d='M21 12a9 9 0 1 1-6.2-8.6'/></svg>";
static const char ICON_BOLT[] = "<svg viewBox='0 0 24 24' fill='currentColor'><path d='M13 2 3 14h7l-1 8 10-12h-7l1-8Z'/></svg>";

static String pageShell(const String &title, const String &body, const char *script) {
    String html;
    html.reserve(2600 + body.length());
    html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
           "<meta name='color-scheme' content='dark light'><title>";
    html += title;
    html += "</title><style>";
    html += FPSTR(PAGE_CSS);
    html += "</style></head><body><div class='card'>";
    html += body;
    html += "<a href='/'>Back to dashboard</a></div>";
    if (script) { html += "<script>"; html += script; html += "</script>"; }
    html += "</body></html>";
    return html;
}

static String errorPage(const char *msg) {
    String body = "<div class='ic err'>";
    body += ICON_ERR;
    body += "</div><h1>Error</h1><p>";
    body += htmlEscape(msg);
    body += "</p>";
    return pageShell("Error", body, nullptr);
}

static String relayPage(int id, bool queued) {
    const RelayConfig &r = appConfig.relays[id];
    String name = htmlEscape(r.name);
    String body = "<div class='ic warn' id='ic'>";
    body += ICON_SPIN;
    body += "</div><h1>";
    body += name;
    body += "</h1><div class='sub'>";
    body += queued ? "Power off for " : "Already restarting &middot; ";
    body += String(r.restartSec);
    body += "s</div><div class='big' id='cnt'>";
    body += String(queued ? r.restartSec : relayGetRemaining(id));
    body += "</div><div class='lbl' id='lbl'>seconds until power on</div>";

    // Countdown driven by the device status; falls back to a local timer
    String js = "var id=" + String(id) + ",t=" + String(queued ? r.restartSec : relayGetRemaining(id)) + ",done=false;"
        "var cnt=document.getElementById('cnt'),lbl=document.getElementById('lbl'),ic=document.getElementById('ic');"
        "function fin(){if(done)return;done=true;ic.className='ic ok';ic.innerHTML=\"";
    js += ICON_OK;
    js += "\";cnt.textContent='ON';lbl.textContent='power restored'}"
        "setInterval(function(){if(!done&&t>0){t--;cnt.textContent=t}},1000);"
        "var seen=false;setInterval(function(){fetch('/api/status',{cache:'no-store'}).then(function(r){return r.json()}).then(function(d){"
        "var r=d.relays[id];if(!r)return;if(r.restarting){seen=true;t=r.remaining;cnt.textContent=t}else if(seen||t<=0){fin()}}).catch(function(){})},1500);";
    return pageShell("Restarting " + name, body, js.c_str());
}

static String wolPage(int id) {
    const WoLDevice &d = appConfig.wolDevices[id];
    String body = "<div class='ic ok'>";
    body += ICON_BOLT;
    body += "</div><h1>";
    body += htmlEscape(d.name);
    body += "</h1><div class='sub'>";
    body += htmlEscape(d.mac);
    body += "</div><p>Magic packet sent. The device may take a minute to boot.</p>";
    return pageShell("Wake " + htmlEscape(d.name), body, nullptr);
}

// GET /api/relays/restart?id=X  -> HTML page with live countdown
static void handleGetRestart(AsyncWebServerRequest *req) {
    REQUIRE_AUTH(req);
    int id = getParamInt(req, "id", -1);
    if (id < 0 || id >= RELAY_COUNT) { sendHtml(req, 400, errorPage("Invalid relay ID")); return; }
    if (appConfig.relays[id].restarting) { sendHtml(req, 200, relayPage(id, false)); return; }
    if (!actionPush(ACTION_RESTART, id, "api")) { sendHtml(req, 503, errorPage("Device busy, try again")); return; }
    sendHtml(req, 200, relayPage(id, true));
}

// GET /api/wol/wake?id=X  -> HTML confirmation page
static void handleGetWake(AsyncWebServerRequest *req) {
    REQUIRE_AUTH(req);
    int id = getParamInt(req, "id", -1);
    if (!wolValidId(id)) { sendHtml(req, 400, errorPage("Invalid device ID")); return; }
    if (WiFi.status() != WL_CONNECTED || !actionPush(ACTION_WOL, id, "api")) {
        sendHtml(req, 503, errorPage("Could not send the packet right now, try again"));
        return;
    }
    sendHtml(req, 200, wolPage(id));
}

static void handleIndex(AsyncWebServerRequest *req) {
    REQUIRE_AUTH(req);
    AsyncWebServerResponse *res = req->beginResponse_P(200, "text/html", INDEX_HTML);
    res->addHeader("Cache-Control", "no-cache");
    req->send(res);
}

static void handleApiDoc(AsyncWebServerRequest *req) {
    REQUIRE_AUTH(req);
    AsyncWebServerResponse *res = req->beginResponse_P(200, "text/html", API_HTML);
    res->addHeader("Cache-Control", "no-cache");
    req->send(res);
}

static void handleNotFound(AsyncWebServerRequest *req) {
    // Unknown API paths get a JSON 404; everything else falls back to the dashboard
    if (req->url().startsWith("/api/")) {
        REQUIRE_AUTH(req);
        sendJsonResult(req, 404, "not found");
        return;
    }
    handleIndex(req);
}

inline void webserverInit() {
    server.on("/", HTTP_GET, handleIndex);
    server.on("/api-doc", HTTP_GET, handleApiDoc);
    server.on("/api/status", HTTP_GET, handleGetStatus);

    server.on("/api/relays/restart", HTTP_POST, handlePostRestart);
    server.on("/api/relays/restart", HTTP_GET, handleGetRestart);
    server.on("/api/wol/wake", HTTP_POST, handlePostWake);
    server.on("/api/wol/wake", HTTP_GET, handleGetWake);
    server.on("/api/ping/check", HTTP_POST, handlePostPingCheck);

    server.on("/api/schedule", HTTP_GET, [](AsyncWebServerRequest *req) {
        REQUIRE_AUTH(req);
        sendJson(req, 200, scheduleStatusJson());
    });
    server.on("/api/ping/history", HTTP_GET, handlePingHistory);
    server.on("/api/ping/outages", HTTP_GET, [](AsyncWebServerRequest *req) {
        REQUIRE_AUTH(req);
        sendJson(req, 200, pingOutagesJson());
    });
    server.on("/api/log", HTTP_GET, [](AsyncWebServerRequest *req) {
        REQUIRE_AUTH(req);
        sendJson(req, 200, logJson());
    });

    server.onNotFound(handleNotFound);

    server.begin();
    Serial.printf("[WebServer] Started on port %d%s\n", WEB_PORT,
        strlen(WEB_AUTH_USER) ? " (basic auth enabled)" : "");
}

#endif
