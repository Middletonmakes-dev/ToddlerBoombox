#pragma once
#include <pgmspace.h>

static const char WEB_PAGE[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Toddler Boombox File Manager</title>
<style>
body{font-family:system-ui,Arial;margin:0;background:#0b1020;color:#e6e6e6}
.top{padding:12px 14px;background:#111a33;position:sticky;top:0;border-bottom:1px solid #233056}
h1{font-size:16px;margin:0 0 6px 0;color:#00d4ff}
.row{display:flex;gap:8px;flex-wrap:wrap;align-items:center}
input,button{font-size:14px;border-radius:8px;border:1px solid #2a3a66;background:#0b1020;color:#e6e6e6;padding:8px 10px}
button{background:#16213e;cursor:pointer}
button:active{opacity:.85}
button:disabled{opacity:.5;cursor:not-allowed}
.main{padding:12px 14px;max-width:900px;margin:0 auto}
.card{background:#16213e;border:1px solid #233056;border-radius:12px;padding:12px;margin-bottom:12px}
.path{font-family:ui-monospace,Consolas,monospace;color:#9fb0ff}
.list{width:100%;border-collapse:collapse}
.list td{padding:10px 8px;border-bottom:1px solid #233056}
.name{cursor:pointer}
.dim{color:#9aa3b2}
.actions{display:flex;gap:8px;justify-content:flex-end;flex-wrap:wrap}
a{color:#00d4ff;text-decoration:none}
.small{font-size:12px;color:#9aa3b2}
.prog{font-size:13px;color:#00d4ff;min-height:18px}
.prog.err{color:#ff6b6b}
.prog.ok{color:#4cff88}
</style>
</head>
<body>
<div class="top">
  <h1>&#128221; Toddler Boombox SD File Manager</h1>
  <div class="row">
    <span class="path" id="cur">/</span>
    <button onclick="up()">Up</button>
    <button onclick="refresh()">Refresh</button>
    <button onclick="mkdir()">New Folder</button>
    <span class="small" id="space">&hellip;</span>
  </div>
  <div class="row" style="margin-top:8px">
    <input id="file" type="file" multiple accept=".mp3"/>
    <button id="upbtn" onclick="upload()">Upload Here</button>
  </div>
  <div class="prog" id="prog"></div>
</div>

<div class="main">
  <div class="card">
    <table class="list" id="tbl">
      <tr><td class="dim">Loading&hellip;</td></tr>
    </table>
  </div>
  <div class="small">Tip: folders /Music and /SFX are where the player looks by default.</div>
</div>

<script>
let cwd = "/";
let uploading = false;

function fmtSize(b){
  if(b < 1024) return b + " B";
  if(b < 1024*1024) return (b/1024).toFixed(1) + " KB";
  return (b/1024/1024).toFixed(1) + " MB";
}

function joinPath(a,b){
  if(a === "/") return "/" + b;
  return a + "/" + b;
}

function setProgress(msg, cls){
  const el = document.getElementById("prog");
  el.textContent = msg;
  el.className = "prog" + (cls ? " " + cls : "");
}

async function refresh(){
  document.getElementById("cur").textContent = cwd;

  // space
  try{
    const s = await fetch("/api/space").then(r=>r.json());
    if(s.ok) document.getElementById("space").textContent =
      `SD: ${s.used} MB used / ${s.total} MB (free ${s.free} MB)`;
  }catch(e){}

  // list
  const tbl = document.getElementById("tbl");
  tbl.innerHTML = `<tr><td class="dim">Loading&hellip;</td></tr>`;

  let data;
  try{
    data = await fetch("/api/list?path=" + encodeURIComponent(cwd)).then(r=>r.json());
  }catch(e){
    tbl.innerHTML = `<tr><td class="dim">List failed (timeout)</td></tr>`;
    return;
  }

  if(!data.ok){
    tbl.innerHTML = `<tr><td class="dim">List failed: ${data.err||"unknown"}</td></tr>`;
    return;
  }

  const items = data.items || [];
  if(items.length === 0){
    tbl.innerHTML = `<tr><td class="dim">Empty</td></tr>`;
    return;
  }

  items.sort((a,b)=> (b.dir - a.dir) || a.name.localeCompare(b.name));

  tbl.innerHTML = items.map(it => {
    const name = it.name;
    const full = joinPath(cwd, name);
    const left = it.dir
      ? `<td class="name" onclick="cd('${name.replaceAll("'","\\'")}')">&#128193; ${name}</td>`
      : `<td>&#127925; ${name}<div class="small dim">${fmtSize(it.size)}</div></td>`;

    const right = it.dir
      ? `<td class="actions">
           <button onclick="renameItem('${full.replaceAll("'","\\'")}')">Rename</button>
           <button onclick="del('${full.replaceAll("'","\\'")}')">Delete</button>
         </td>`
      : `<td class="actions">
           <a href="/api/download?path=${encodeURIComponent(full)}">Download</a>
           <button onclick="renameItem('${full.replaceAll("'","\\'")}')">Rename</button>
           <button onclick="del('${full.replaceAll("'","\\'")}')">Delete</button>
         </td>`;

    return `<tr>${left}${right}</tr>`;
  }).join("");
}

function cd(name){
  cwd = joinPath(cwd, name);
  refresh();
}

function up(){
  if(cwd === "/") return;
  const parts = cwd.split("/").filter(Boolean);
  parts.pop();
  cwd = "/" + parts.join("/");
  if(cwd === "") cwd = "/";
  refresh();
}

async function del(path){
  if(!confirm("Delete " + path + " ?")) return;
  await fetch("/api/delete?path=" + encodeURIComponent(path));
  refresh();
}

async function mkdir(){
  const name = prompt("Folder name?");
  if(!name) return;
  const p = joinPath(cwd, name);
  await fetch("/api/mkdir?path=" + encodeURIComponent(p));
  refresh();
}

async function renameItem(from){
  const base = from.split("/").pop();
  const nn = prompt("New name?", base);
  if(!nn || nn === base) return;
  const parent = from.split("/").slice(0,-1).join("/") || "/";
  const to = (parent === "/") ? ("/" + nn) : (parent + "/" + nn);
  await fetch("/api/rename?from=" + encodeURIComponent(from) + "&to=" + encodeURIComponent(to));
  refresh();
}

async function upload(){
  const fi = document.getElementById("file");
  const btn = document.getElementById("upbtn");

  if(uploading){ return; }
  if(!fi.files || fi.files.length === 0){ alert("Choose files first"); return; }

  uploading = true;
  btn.disabled = true;
  btn.textContent = "Uploading...";

  const total = fi.files.length;
  let ok = 0;
  let fail = 0;

  for(let i = 0; i < total; i++){
    const f = fi.files[i];
    setProgress(`Uploading ${i+1}/${total}: ${f.name} (${fmtSize(f.size)})`, "");

    const form = new FormData();
    form.append("file", f, f.name);

    try {
      const res = await fetch("/api/upload?path=" + encodeURIComponent(cwd),
        { method: "POST", body: form });
      if(res.ok){ ok++; }
      else { fail++; setProgress(`Failed: ${f.name} (server error)`, "err"); }
    } catch(e) {
      fail++;
      setProgress(`Failed: ${f.name} (network error)`, "err");
    }
  }

  fi.value = "";
  uploading = false;
  btn.disabled = false;
  btn.textContent = "Upload Here";

  if(fail === 0){
    setProgress(`Done! ${ok} file${ok>1?"s":""} uploaded`, "ok");
  } else {
    setProgress(`Done: ${ok} uploaded, ${fail} failed`, "err");
  }

  refresh();
}

refresh();
</script>
</body>
</html>
)rawliteral";
