/* Ammarflix flair , served as /ammarflix.js straight from res/ ( edit freely , no rebuild needed )
   - watch page : intro ( res/sound.mp3 + wordmark ) before every video , resume where you left off , subtitles , keyboard shortcuts
   - library    : progress bars on videos you started , keyboard shortcuts
   Shortcuts : space/k play-pause , f fullscreen , left/right or j/l -/+10s , up/down volume , m mute , c subtitles ,
               0 start over , i intro on/off , r random video , / search , ? this list */
(function(){
"use strict";

function store(key,value)
{
  try { if (value===undefined) { return localStorage.getItem(key); }
        if (value===null) { localStorage.removeItem(key); } else { localStorage.setItem(key,value); } } catch(e) {}
  return null;
}
function savedPosition(id) { try { return JSON.parse(store("ammarflix.pos."+id)||"null"); } catch(e) { return null; } }
function introEnabled() { return store("ammarflix.intro")!=="off"; }

var toastTimer=0,toastElement=null;
function toast(text)
{
  if (!toastElement) { toastElement=document.createElement("div"); toastElement.className="toast"; document.body.appendChild(toastElement); }
  toastElement.textContent=text;
  toastElement.classList.remove("out");
  clearTimeout(toastTimer);
  toastTimer=setTimeout(function(){ toastElement.classList.add("out"); },2200);
}

function clock(seconds)
{
  seconds=Math.floor(seconds);
  var h=Math.floor(seconds/3600),m=Math.floor(seconds/60)%60,s=seconds%60;
  return (h ? h+":"+(m<10?"0":"") : "")+m+":"+(s<10?"0":"")+s;
}

/* ---------------------------------------------------------------- shortcut sheet */
var help=null;
function toggleHelp()
{
  if (!help)
  {
    help=document.createElement("div");
    help.className="help"; help.hidden=true;
    var rows=[["Space / K","Play or pause"],["F","Fullscreen"],["← → / J L","Back / forward 10 seconds"],["↑ ↓","Volume"],
              ["M","Mute"],["C","Next subtitle track"],["0","Start over"],["I","Intro sound on / off"],["R","Random video"],["/","Search"],["?","This list"]];
    var html="<div><h2>Keyboard shortcuts</h2><dl>";
    rows.forEach(function(r){ html+="<dt><kbd>"+r[0]+"</kbd></dt><dd>"+r[1]+"</dd>"; });
    help.innerHTML=html+"</dl></div>";
    help.onclick=function(){ help.hidden=true; };
    document.body.appendChild(help);
  }
  help.hidden=!help.hidden;
}

/* ---------------------------------------------------------------- library page */
function decorateLibrary()
{
  document.querySelectorAll(".card[data-id]").forEach(function(card){
    var p=savedPosition(card.getAttribute("data-id"));
    if (!p || !p.d) { return; }
    var bar=document.createElement("div");
    bar.className="seen";
    bar.style.width=Math.min(100,Math.max(2,100*p.t/p.d))+"%";
    var thumb=card.querySelector(".thumb");
    if (thumb) { thumb.appendChild(bar); }
  });
}

/* ---------------------------------------------------------------- watch page */
var player=document.getElementById("player");
var subtitleSelect=document.getElementById("subs");

function setupSubtitles()
{
  //Subtitles are parsed here and added as cues , so neither the server's Content-Type for .srt/.vtt nor the
  //file's encoding matters ( non UTF-8 files are assumed to be Greek windows-1253 )
  var status=document.getElementById("substatus"),tracks={};
  function ts(s){var p=s.trim().replace(",",".").split(":"),t=0;for(var i=0;i<p.length;i++)t=t*60+parseFloat(p[i]);return t;}
  function parse(txt){var cues=[];txt.replace(/\r/g,"").split(/\n\s*\n/).forEach(function(b){
    var l=b.split("\n"),i=-1;for(var k=0;k<l.length;k++)if(l[k].indexOf("-->")>=0){i=k;break;}if(i<0)return;
    var p=l[i].split("-->"),a=ts(p[0]),e=ts(p[1].trim().split(/\s+/)[0]);
    var text=l.slice(i+1).join("\n").replace(/\{[^}]*\}/g,"").trim();
    if(!isNaN(a)&&!isNaN(e)&&e>a&&text)cues.push(new VTTCue(a,e,text));});return cues;}
  function decode(b){try{return new TextDecoder("utf-8",{fatal:true}).decode(b);}catch(x){return new TextDecoder("windows-1253").decode(b);}}

  subtitleSelect.onchange=function(){
    var u=subtitleSelect.value;
    for (var k in tracks) { tracks[k].mode="disabled"; }
    if (!u) { status.textContent=""; return; }
    if (tracks[u]) { tracks[u].mode="showing"; return; }
    status.textContent="Loading subtitles… (embedded tracks are extracted on first use, this can take a while)";
    fetch(u,{credentials:"same-origin"}).then(function(r){ if(!r.ok) throw r.status; return r.arrayBuffer(); }).then(function(b){
      var t=player.addTextTrack("subtitles");
      parse(decode(b)).forEach(function(c){ t.addCue(c); });
      tracks[u]=t;
      t.mode=(subtitleSelect.value===u)?"showing":"disabled";
      status.textContent="";
    }).catch(function(){ status.textContent="Could not load these subtitles"; });
  };
  player.addEventListener("error",function(){ status.textContent="Your browser cannot play this file (unsupported container or codec)"; });
}

function setupResume()
{
  var id=player.getAttribute("data-id"),lastSave=0;
  player.addEventListener("loadedmetadata",function(){
    var p=savedPosition(id);
    if (p && p.t>30 && p.t<player.duration-60)
    {
      player.currentTime=p.t;
      toast("Resuming at "+clock(p.t)+" · press 0 to start over");
    }
  },{once:true});
  function save()
  {
    if (!player.duration || player.currentTime<5) { return; }
    store("ammarflix.pos."+id,JSON.stringify({t:Math.floor(player.currentTime),d:Math.floor(player.duration)}));
  }
  player.addEventListener("timeupdate",function(){ var now=Date.now(); if (now-lastSave>5000) { lastSave=now; save(); } });
  player.addEventListener("pause",save);
  player.addEventListener("ended",function(){ store("ammarflix.pos."+id,null); });
  window.addEventListener("pagehide",save);
}

function startVideo()
{
  //Autoplay with sound can still be refused ( e.g. intro turned off on a first visit ) , the native controls are there then
  var attempt=player.play();
  if (attempt && attempt.catch) { attempt.catch(function(){}); }
  player.focus();
}

function setupIntro()
{
  var intro=document.getElementById("intro");
  if (!introEnabled()) { startVideo(); return; }

  var sound=new Audio("sound.mp3"),finished=false,fallbackTimer=0;
  intro.hidden=false;

  function finish()
  {
    if (finished) { return; }
    finished=true;
    clearTimeout(fallbackTimer);
    sound.pause();
    intro.classList.add("out");
    setTimeout(function(){ intro.hidden=true; },450);
    startVideo();
  }
  function play()
  {
    intro.classList.remove("waiting");
    intro.classList.add("playing");
    intro.onclick=finish; //Click to skip
    sound.currentTime=0;
    sound.onended=finish;
    fallbackTimer=setTimeout(finish,6000); //Never get stuck if the sound stalls
    var attempt=sound.play();
    if (attempt && attempt.catch) { attempt.catch(waitForClick); }
  }
  function waitForClick()
  {
    //The browser refused to start sound without a click on this page , so ask for one
    clearTimeout(fallbackTimer);
    intro.classList.remove("playing");
    intro.classList.add("waiting");
    intro.onclick=play;
  }
  play();
}

/* ---------------------------------------------------------------- keyboard */
function cycleSubtitles()
{
  if (!subtitleSelect || subtitleSelect.options.length<2) { toast("No subtitles for this video"); return; }
  subtitleSelect.selectedIndex=(subtitleSelect.selectedIndex+1)%subtitleSelect.options.length;
  subtitleSelect.onchange();
  toast("Subtitles: "+subtitleSelect.options[subtitleSelect.selectedIndex].text);
}

document.addEventListener("keydown",function(e){
  if (e.ctrlKey || e.metaKey || e.altKey) { return; }
  var tag=(e.target && e.target.tagName) || "";
  if (tag==="INPUT" || tag==="SELECT" || tag==="TEXTAREA") { if (e.key==="Escape") { e.target.blur(); } return; }

  var handled=true,k=e.key;
  var intro=document.getElementById("intro");
  var introShowing=( intro && !intro.hidden && !intro.classList.contains("out") );
  if (introShowing && (k===" " || k==="Enter" || k==="k" || k==="K")) { intro.click(); } else //Skip it , or start it if it waits for a click
  if (k==="?") { toggleHelp(); } else
  if (k==="Escape" && help && !help.hidden) { help.hidden=true; } else
  if (k==="r" || k==="R") { window.location.href="random.html"; } else
  if (k==="/") { var search=document.querySelector("header input[type=search]"); if (search) { search.focus(); search.select(); } } else
  if (k==="i" || k==="I") { var off=introEnabled(); store("ammarflix.intro",off?"off":null); toast(off ? "Intro sound off" : "Intro sound on"); } else
  if (!player) { handled=false; } else
  if (k===" " || k==="k" || k==="K") { if (player.paused) { startVideo(); } else { player.pause(); } } else
  if (k==="f" || k==="F") { if (document.fullscreenElement) { document.exitFullscreen(); } else { (player.closest(".stage")||player).requestFullscreen(); } } else
  if (k==="ArrowLeft" || k==="j" || k==="J")  { player.currentTime=Math.max(0,player.currentTime-10); } else
  if (k==="ArrowRight" || k==="l" || k==="L") { player.currentTime=Math.min(player.duration||0,player.currentTime+10); } else
  if (k==="ArrowUp")   { player.volume=Math.min(1,player.volume+0.1); player.muted=false; toast("Volume "+Math.round(player.volume*100)+"%"); } else
  if (k==="ArrowDown") { player.volume=Math.max(0,player.volume-0.1); toast("Volume "+Math.round(player.volume*100)+"%"); } else
  if (k==="m" || k==="M") { player.muted=!player.muted; toast(player.muted ? "Muted" : "Sound on"); } else
  if (k==="c" || k==="C") { cycleSubtitles(); } else
  if (k==="0") { player.currentTime=0; } else
  { handled=false; }

  if (handled) { e.preventDefault(); e.stopPropagation(); }
},true);

if (player) { setupSubtitles(); setupResume(); setupIntro(); } else { decorateLibrary(); }
})();
