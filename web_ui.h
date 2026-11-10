// Wi-Fi の操作画面。1 ページだけで、機種ごとにパネルの見た目とつまみの並びを切り替える
#pragma once

static const char INDEX_HTML[] = R"HTML(<!doctype html>
<html lang="ja">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Pico Synth</title>
<style>
:root{--bg:#111;--panel:#1d1d1d;--ink:#e8e0c8;--dim:#9a917c;--acc:#c8a060;--line:#444;--knob:#0c0c0c;--kline:#eee;--side:#6b3f1d}
body.acid{--bg:#3a3a38;--panel:#c9c9c4;--ink:#1c1c1c;--dim:#55554f;--acc:#e8742a;--line:#8d8d86;--knob:#2a2a2a;--kline:#fff;--side:#a9a9a3}
body.juno{--bg:#0d0d0f;--panel:#1b1b1e;--ink:#f0f0f0;--dim:#9a9aa2;--acc:#e0303a;--line:#3a3a40;--knob:#d8d8d8;--kline:#111;--side:#2a2a2e}
body.pro5{--bg:#0e0c0a;--panel:#161514;--ink:#e6dcc0;--dim:#948b74;--acc:#d9c9a0;--line:#4a4438;--knob:#0a0a0a;--kline:#f4f0e0;--side:#7a4a22}
body.ms{--bg:#141516;--panel:#2a2c2e;--ink:#f4f4f4;--dim:#a9acaf;--acc:#fff;--line:#5a5d60;--knob:#0d0d0d;--kline:#fff;--side:#1c1d1e}
body.sem{--bg:#3b372d;--panel:#e9e2cf;--ink:#1a1a1a;--dim:#5f594a;--acc:#2a4d8f;--line:#a39c88;--knob:#1a1a1a;--kline:#fff;--side:#2a2a2a}
body.str{--bg:#1c110a;--panel:#3a2416;--ink:#f0e6d0;--dim:#b39f80;--acc:#e0b050;--line:#6a4a30;--knob:#f0e6d0;--kline:#3a2416;--side:#5a3418}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
html,body{margin:0;background:var(--bg);color:var(--ink);font:13px/1.3 "Helvetica Neue",Arial,sans-serif;-webkit-user-select:none;user-select:none}
header{display:flex;flex-wrap:wrap;align-items:center;gap:8px;padding:8px 12px;background:#000;color:#ddd}
header h1{margin:0;font-size:15px;letter-spacing:.25em;font-weight:700}
#tabs{display:flex;flex-wrap:wrap;gap:4px;flex:1}
button{font:inherit;color:inherit;background:#222;border:1px solid #555;border-radius:3px;padding:6px 10px;cursor:pointer;letter-spacing:.08em}
#tabs button.on{background:#e8e0c8;color:#000;border-color:#e8e0c8;font-weight:700}
#stat{font-size:11px;color:#999;white-space:nowrap;font-variant-numeric:tabular-nums}
#led{display:inline-block;width:8px;height:8px;border-radius:50%;background:#333;margin-right:4px;vertical-align:middle}
#led.on{background:#f33;box-shadow:0 0 6px #f33}
#wrap{border-left:14px solid var(--side);border-right:14px solid var(--side);background:var(--panel);padding:10px 10px 4px}
#name{display:flex;align-items:baseline;gap:12px;margin:0 2px 8px}
#name b{font-size:22px;letter-spacing:.2em;color:var(--acc)}
#name span{color:var(--dim);font-size:12px}
#panel{display:flex;flex-wrap:wrap;gap:8px}
.sec{border:1px solid var(--line);border-radius:4px;padding:6px 8px 8px;display:flex;flex-direction:column;max-width:100%}
.sec h2{margin:0 0 6px;font-size:10px;letter-spacing:.18em;font-weight:700;color:var(--ink);border-bottom:2px solid var(--acc);padding-bottom:3px}
body.juno .sec:nth-child(3n+1) h2{border-color:#e0303a}
body.juno .sec:nth-child(3n+2) h2{border-color:#3a7be0}
body.juno .sec:nth-child(3n) h2{border-color:#e0a030}
.row{display:flex;flex-wrap:wrap;gap:10px;align-items:flex-end;flex:1}
.c{display:flex;flex-direction:column;align-items:center;gap:3px;min-width:46px;touch-action:none}
.c label{font-size:9px;letter-spacing:.08em;color:var(--dim);text-align:center;white-space:nowrap}
.c output{font-size:10px;font-variant-numeric:tabular-nums;color:var(--ink);min-height:13px}
.k{width:44px;height:44px;border-radius:50%;background:var(--knob);border:2px solid var(--line);position:relative;cursor:ns-resize;touch-action:none;box-shadow:0 2px 4px rgba(0,0,0,.5)}
.k i{position:absolute;left:50%;top:3px;width:3px;height:15px;margin-left:-1.5px;background:var(--kline);border-radius:2px}
.f{width:30px;height:92px;position:relative;cursor:ns-resize;touch-action:none}
.f:before{content:"";position:absolute;left:13px;top:4px;bottom:4px;width:4px;background:#000;border:1px solid var(--line);border-radius:2px}
.f i{position:absolute;left:3px;width:24px;height:12px;margin-top:-6px;background:var(--knob);border:1px solid #000;border-radius:2px;box-shadow:0 1px 3px rgba(0,0,0,.6)}
.f i:after{content:"";position:absolute;left:2px;right:2px;top:4px;height:2px;background:var(--kline)}
.o{display:flex;flex-direction:column;gap:2px}
.o button{padding:3px 6px;font-size:10px;min-width:40px;background:transparent;border-color:var(--line);color:var(--dim);letter-spacing:.04em}
.o button.on{background:var(--acc);border-color:var(--acc);color:var(--panel);font-weight:700}
#kbwrap{padding:8px 0 0;background:var(--bg);border-left:14px solid var(--side);border-right:14px solid var(--side)}
#kbbar{display:flex;gap:6px;align-items:center;padding:0 8px 6px;color:#aaa;font-size:11px}
#kb{position:relative;height:150px;touch-action:none;background:#000}
.wk,.bk{position:absolute;top:0;border:1px solid #000;border-radius:0 0 4px 4px}
.wk{background:#f4f1e8;height:100%}
.bk{background:#151515;height:60%;z-index:2}
.wk.on{background:#e8b060}.bk.on{background:#a06020}
footer{padding:8px 14px;color:#777;font-size:11px}
#err{display:none;background:#a00;color:#fff;padding:4px 12px;font-size:12px}
</style>
</head>
<body>
<header>
  <h1>PICO SYNTH</h1>
  <div id="tabs"></div>
  <span id="stat"><span id="led"></span><span id="st"></span></span>
  <button id="bsave">保存</button><button id="binit">初期化</button><button id="bpanic">音を止める</button>
</header>
<div id="err">本体と通信できません</div>
<div id="wrap">
  <div id="name"><b></b><span></span></div>
  <div id="panel"></div>
</div>
<div id="kbwrap">
  <div id="kbbar"><button id="od">◀ OCT</button><span id="ol"></span><button id="ou">OCT ▶</button>
    <span>鍵盤は下のほうを押すほど強く鳴る。PC のキーボードは A S D F … の段</span></div>
  <div id="kb"></div>
</div>
<footer>つまみは上下にドラッグ。変更は「保存」を押すまで電源を切ると消える。MIDI のプログラムチェンジ 0〜6 でも機種が切り替わる。</footer>
<script>
const LBL={o1wave:'WAVE',o1oct:'OCTAVE',o1lvl:'LEVEL',o2wave:'WAVE',o2oct:'OCTAVE',o2semi:'SEMI',o2det:'DETUNE',o2lvl:'LEVEL',
o3wave:'WAVE',o3oct:'OCTAVE',o3det:'DETUNE',o3lvl:'LEVEL',pw:'PW',pwm:'PWM',noise:'NOISE',ring:'RING',pmenv:'FILT ENV',pmosc:'OSC B',
cut:'CUTOFF',res:'RESO',fenv:'ENV',fkbd:'KBD',flfo:'LFO',fmode:'MODE',drive:'DRIVE',hpf:'HPF',hpres:'PEAK',
fa:'ATTACK',fd:'DECAY',fs:'SUSTAIN',fr:'RELEASE',aa:'ATTACK',ad:'DECAY',as:'SUSTAIN',ar:'RELEASE',vel:'VELOCITY',acc:'ACCENT',
lrate:'RATE',lwave:'WAVE',lpitch:'VIBRATO',glide:'GLIDE',legato:'LEGATO',cho:'CHORUS',dtime:'TIME',dfb:'REPEAT',dmix:'ECHO',vol:'VOLUME'};
const WV=['SAW','PULSE','TRI','SINE'],OC=["32'","16'","8'","4'","2'"];
const OPTS={o1wave:WV,o2wave:WV,o3wave:WV,o1oct:OC,o2oct:OC,o3oct:OC,lwave:['TRI','SAW','SQR','S&H'],cho:['OFF','I','II','ENS'],legato:['OFF','ON']};
const ENV1=['fa','fd','fs','fr'],ENV2=['aa','ad','as','ar'],ECHO=['dtime','dfb','dmix'];
// sec: [見出し, [キー または [キー, ラベル], ...]]。link: そのつまみと一緒に動かすパラメータ
const PANELS={
'MINI':{skin:'mini',sub:'3 VCO・24dB ラダーフィルター・モノフォニック',sec:[
 ['OSCILLATOR 1',['o1oct','o1wave']],['OSCILLATOR 2',['o2oct','o2wave','o2det']],['OSCILLATOR 3',['o3oct','o3wave','o3det']],
 ['MIXER',[['o1lvl','OSC 1'],['o2lvl','OSC 2'],['o3lvl','OSC 3'],'noise','drive']],
 ['FILTER',['cut',['res','EMPHASIS'],['fenv','CONTOUR'],'fkbd',...ENV1]],['LOUDNESS CONTOUR',ENV2],
 ['MODULATION',['lrate','lwave','lpitch',['flfo','FILTER']]],['CONTROLLERS',['glide','legato','pw']],['OUTPUT',[...ECHO,'vol']]]},
'ACID':{skin:'acid',sub:'1 VCO・レゾナンスの強いフィルター。強く弾くとアクセント、重ねて弾くとスライド',sec:[
 ['VCO',['o1wave',['o1oct','RANGE'],'o1lvl']],
 ['VCF',[['cut','CUT OFF'],['res','RESONANCE'],['fenv','ENV MOD'],'fd','acc']],
 ['SLIDE / VCA',[['glide','SLIDE'],'legato',['ad','DECAY'],'as','ar','drive']],['DELAY',ECHO],['OUTPUT',['vol']]],
 link:{fd:v=>({fr:v})}},
'JUNO':{skin:'juno',fader:1,sub:'DCO + サブオシレーター・コーラス・6 音ポリ',sec:[
 ['LFO',['lrate','lwave',['lpitch','PITCH']]],
 ['DCO',[['o1oct','RANGE'],['o1lvl','SAW'],['o2lvl','PULSE'],'pw','pwm',['o3lvl','SUB'],'noise']],
 ['HPF',[['hpf','FREQ']]],['VCF',[['cut','FREQ'],['res','RES'],'fenv','flfo',['fkbd','KYBD']]],
 ['ENV',[['aa','A'],['ad','D'],['as','S'],['ar','R']]],['CHORUS',['cho']],['OUTPUT',['vel','vol']]],
 link:{aa:v=>({fa:v}),ad:v=>({fd:v}),as:v=>({fs:v}),ar:v=>({fr:v}),o1oct:v=>({o2oct:v,o3oct:Math.max(0,v-1)})}},
'PRO-5':{skin:'pro5',sub:'2 VCO・ポリモジュレーション・5 音ポリ',sec:[
 ['POLY-MOD',['pmenv','pmosc']],['LFO',['lrate','lwave',['lpitch','FREQ'],['pwm','PW'],['flfo','FILTER']]],
 ['OSCILLATOR A',['o1oct','o1wave','pw']],['OSCILLATOR B',['o2oct','o2wave','o2semi',['o2det','FINE']]],
 ['MIXER',[['o1lvl','OSC A'],['o2lvl','OSC B'],'noise']],
 ['FILTER',['cut','res',['fenv','ENV AMT'],'fkbd',...ENV1]],['AMPLIFIER',ENV2],['OUTPUT',['cho','vel','vol']]]},
'MS':{skin:'ms',sub:'2 VCO・リングモジュレーター・発振する HPF と LPF・モノフォニック',sec:[
 ['VCO 1',['o1wave','o1oct','pw']],['VCO 2',['o2wave','o2oct',['o2semi','PITCH'],['o2det','FINE']]],
 ['MIXER',[['o1lvl','VCO 1'],['o2lvl','VCO 2'],'ring','noise','drive']],
 ['HIGH PASS FILTER',[['hpf','CUTOFF'],'hpres']],['LOW PASS FILTER',['cut',['res','PEAK'],['fenv','EG'],['flfo','MG'],'fkbd']],
 ['EG 1 (FILTER)',ENV1],['EG 2 (AMP)',ENV2],['MODULATION GENERATOR',['lrate','lwave','lpitch','pwm']],
 ['PORTAMENTO',['glide','legato']],['OUTPUT',[...ECHO,'vol']]]},
'SEM':{skin:'sem',sub:'2 VCO・12dB マルチモードフィルター・4 音ポリ',sec:[
 ['VCO 1',['o1oct','o1wave','o1lvl']],['VCO 2',['o2oct','o2wave','o2semi',['o2det','FINE'],'o2lvl']],['PULSE WIDTH',['pw','pwm']],
 ['VCF',['cut','res',['fmode','LP - NOTCH - HP'],'fenv','fkbd','flfo']],['ENV 1 (VCF)',ENV1],['ENV 2 (VCA)',ENV2],
 ['LFO',['lrate','lwave','lpitch']],['OUTPUT',['cho',...ECHO,'vel','vol']]]},
'STRINGS':{skin:'str',fader:1,sub:'ストリングマシン・アンサンブル・8 音ポリ',sec:[
 ['VOICES',[['o1lvl',"8'"],['o2lvl',"4'"],['o3lvl','BASS']]],['TONE',[['cut','BRILLIANCE'],['hpf','LOW CUT']]],
 ['ENVELOPE',[['aa','CRESCENDO'],['ar','SUSTAIN']]],['ENSEMBLE',['cho']],['VIBRATO',['lrate',['lpitch','DEPTH']]],['OUTPUT',[...ECHO,'vol']]]}
};
const $=q=>document.querySelector(q),el=(t,c,p)=>{const e=document.createElement(t);if(c)e.className=c;if(p)p.appendChild(e);return e};
let M,cur=-1,idx={},vals={},ctl={},touched={},pend={},base=48,lastMidi=0,panel;

// 通信は 1 本の列にして順番を守る（鍵盤のオンとオフが入れ替わらないように）
let chain=Promise.resolve(),qlen=0;
function req(u){qlen++;const p=chain.then(()=>fetch(u)).then(r=>{if(!r.ok)throw 0;$('#err').style.display='none';return r});
 chain=p.catch(()=>{$('#err').style.display='block'}).then(()=>{qlen--});return p}

function fmt(k,v){if(OPTS[k])return OPTS[k][v];if(k=='o2semi')return(v>12?'+':'')+(v-12);
 if(k=='o2det'||k=='o3det'){const c=Math.round((v-.5)*100);return(c>0?'+':'')+c+'c'}return(v*10).toFixed(1)}

function change(k,v,noLink){vals[k]=v;touched[k]=Date.now();pend[k]=v;if(ctl[k])ctl[k](v);
 if(!noLink&&panel.link&&panel.link[k]){const o=panel.link[k](v);for(const j in o)change(j,o[j],1)}}

function mkCtl(k,label,fader){
 const c=el('div','c'),steps=M.steps[idx[k]];
 if(OPTS[k]){const o=el('div','o',c),bs=OPTS[k].map((t,j)=>{const b=el('button','',o);b.textContent=t;b.onclick=()=>change(k,j);return b});
  ctl[k]=v=>bs.forEach((b,j)=>b.classList.toggle('on',j==v))}
 else{const f=el('div',fader?'f':'k',c),i=el('i','',f),out=el('output','',c),max=steps?steps-1:1;
  ctl[k]=v=>{const n=v/max;if(fader)i.style.top=(92-10-n*72)+'px';else f.style.transform='rotate('+(-135+270*n)+'deg)';out.textContent=fmt(k,v)};
  let y0,x0,n0,on=false;
  f.onpointerdown=e=>{f.setPointerCapture(e.pointerId);on=true;y0=e.clientY;x0=e.clientX;n0=vals[k]/max;e.preventDefault()};
  f.onpointermove=e=>{if(!on)return;let n=n0+((y0-e.clientY)+(fader?0:e.clientX-x0))/(fader?72:160);n=Math.min(1,Math.max(0,n));
   const v=steps?Math.round(n*max):n;if(v!=vals[k])change(k,v)};
  f.onpointerup=f.onpointercancel=()=>{on=false}}
 el('label','',c).textContent=label||LBL[k]||k;
 return c}

function build(){
 const m=M.models[cur];panel=PANELS[m.name]||{skin:'',sub:'',sec:[['ALL',M.keys]]};
 document.body.className=panel.skin;ctl={};
 $('#name b').textContent=m.name;$('#name span').textContent=panel.sub;
 const P=$('#panel');P.innerHTML='';
 for(const[t,items]of panel.sec){const s=el('div','sec',P);el('h2','',s).textContent=t;const r=el('div','row',s);
  for(const it of items){const k=Array.isArray(it)?it[0]:it;if(!(k in idx))continue;r.appendChild(mkCtl(k,Array.isArray(it)?it[1]:0,panel.fader))}}
 [...$('#tabs').children].forEach((b,i)=>b.classList.toggle('on',i==cur));
 base=m.voices==1?36:48;buildKb()}

function apply(s){
 const now=Date.now(),sw=s.m!=cur;
 if(sw){cur=s.m;touched={};pend={}}
 M.keys.forEach((k,i)=>{if(sw||now-(touched[k]||0)>1200)vals[k]=s.p[i]});
 if(sw)build();
 for(const k in ctl)ctl[k](vals[k]);
 $('#st').textContent='CPU '+s.load+'% (瞬間最大 '+s.peak+'%)  発音 '+s.voices;
 $('#led').classList.toggle('on',s.midi!=lastMidi);lastMidi=s.midi}
const getState=u=>req(u).then(r=>r.json()).then(apply).catch(()=>{});

// ---- 鍵盤 ----
const held={};  // pointerId -> 音
function nOn(n,v){req('/api/note?n='+n+'&v='+v);const k=$('#kb [data-n="'+n+'"]');if(k)k.classList.add('on')}
function nOff(n){req('/api/note?n='+n+'&v=0');const k=$('#kb [data-n="'+n+'"]');if(k)k.classList.remove('on')}
function buildKb(){
 const kb=$('#kb');kb.innerHTML='';$('#ol').textContent='C'+(base/12-1);
 const NW=22,w=100/NW;let x=0;
 for(let n=base;n<=base+36;n++){const blk=[1,3,6,8,10].includes(n%12),k=el('div',blk?'bk':'wk',kb);k.dataset.n=n;
  if(blk){k.style.left=(x*w-w*.3)+'%';k.style.width=(w*.6)+'%'}else{k.style.left=(x*w)+'%';k.style.width=w+'%';x++}}}
function keyAt(e){const t=document.elementFromPoint(e.clientX,e.clientY);if(!t||!t.dataset.n)return null;
 const r=t.getBoundingClientRect();return{n:+t.dataset.n,v:Math.round(40+87*Math.min(1,Math.max(0,(e.clientY-r.top)/r.height)))}}
const kb=$('#kb');
kb.onpointerdown=e=>{e.preventDefault();const k=keyAt(e);if(k){held[e.pointerId]=k.n;nOn(k.n,k.v)}};
kb.onpointermove=e=>{if(!(e.pointerId in held))return;const k=keyAt(e),o=held[e.pointerId];
 if(k&&k.n!=o){nOn(k.n,k.v);nOff(o);held[e.pointerId]=k.n}};
kb.onpointerup=kb.onpointercancel=kb.onpointerleave=e=>{if(e.pointerId in held){nOff(held[e.pointerId]);delete held[e.pointerId]}};
const PCK='awsedftgyhujkolp;',pcHeld={};
onkeydown=e=>{if(e.repeat||e.metaKey||e.ctrlKey)return;const i=PCK.indexOf(e.key.toLowerCase());
 if(i>=0&&!(e.key in pcHeld)){pcHeld[e.key]=base+12+i;nOn(base+12+i,e.shiftKey?120:90)}
 else if(e.key=='z')oct(-12);else if(e.key=='x')oct(12)};
onkeyup=e=>{if(e.key in pcHeld){nOff(pcHeld[e.key]);delete pcHeld[e.key]}};
function oct(d){const b=base+d;if(b<12||b>72)return;base=b;buildKb()}
$('#od').onclick=()=>oct(-12);$('#ou').onclick=()=>oct(12);

// ---- 起動 ----
$('#bsave').onclick=()=>getState('/api/save');
$('#binit').onclick=()=>{touched={};pend={};getState('/api/init')};
$('#bpanic').onclick=()=>req('/api/panic');
req('/api/meta').then(r=>r.json()).then(m=>{
 M=m;m.keys.forEach((k,i)=>idx[k]=i);document.title=m.ssid;
 m.models.forEach((x,i)=>{const b=el('button','',$('#tabs'));b.textContent=x.name;b.onclick=()=>{touched={};pend={};getState('/api/model?m='+i)}});
 getState('/api/state');
 // つまみの変更は 40ms ごとにまとめて送る
 setInterval(()=>{const ks=Object.keys(pend);if(!ks.length||qlen>1)return;
  const u='/api/set?'+ks.map(k=>k+'='+(+(+pend[k]).toFixed(4))).join('&');pend={};req(u)},40);
 // MIDI の CC やプログラムチェンジで変わったぶんを画面に反映する
 setInterval(()=>{if(qlen==0)getState('/api/state')},1500)});
</script>
</body>
</html>
)HTML";
