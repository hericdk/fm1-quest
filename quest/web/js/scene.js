// FM1 Quest — scene renderer (ported from design reference; palette-indexed pixel art)
(function(G){
const PALS={
 '1a':['#1f3320','#1f3320','#3f6b3a','#3f6b3a','#1f3320','#3f6b3a','#8fb07a','#e3d6a0','#a0522d','#a0522d','#e3d6a0','#8fb07a','#3f6b3a','#8fb07a','#a0522d','#e3d6a0'],
 '1c':['#1a1016','#2b2338','#5a3050','#2f4a3a','#6a4030','#4a3c48','#a89aa0','#f2e2c4','#c8443c','#e08a3a','#f0c860','#6a9a4a','#4a78b0','#8a6a9a','#e09090','#f0b890']};
const STY={
 '1a':{tree:'round',ground:'path',hp:6,bird:0,grass:3,grass2:0,dirt:7,dirt2:2,tree_c:3,treeHi:6,treeSh:0,arc:7,arc2:6,pink:8,buff:[8,6,7,6],eqBg:6,eqGlow:7,ped:3,pedHi:7,
  base:{sky:[6,7,7],orb:null,stars:0,mnt:6,mnt2:null},night:{sky:[0,0,2],orb:'moon',stars:1,mnt:2,mnt2:null},storm:{sky:[2,2,6],orb:null,stars:0,mnt:0,mnt2:null},
  solo:{sky:[6,7],floor:3,dash:6,edge:7,shadow:0}},
 '1c':{tree:'pine',ground:'dirt',hp:11,bird:0,grass:11,grass2:3,dirt:4,dirt2:0,tree_c:3,treeHi:11,treeSh:0,arc:7,arc2:12,pink:14,buff:[8,12,9,11],eqBg:1,eqGlow:2,ped:4,pedHi:10,
  base:{sky:[1,2,8,9],orb:'sunset',stars:0,mnt:2,mnt2:1},night:{sky:[0,0,1,1],orb:'moon',stars:1,mnt:5,mnt2:1},storm:{sky:[5,5,13,6],orb:null,stars:0,mnt:1,mnt2:0},
  solo:{sky:[1,2,13],floor:5,dash:6,edge:10,shadow:0},
  coast:{sky:[12,12,13,14],orb:'sunset',stars:0,mnt:13,mnt2:null,dirt:15,dirt2:14,grass:10,grass2:14,trees:false,sea:12},
  ruins:{sky:[1,2,5,13],orb:null,stars:1,mnt:5,mnt2:1,dirt:5,dirt2:4,grass:6,grass2:4,trees:false,ruinsOn:true}}};
const MON={'1a':{slime:{M:6,D:3},bat:{W:0,B:3},king:{M:6,D:3}},'1c':{slime:{M:11,D:3},bat:{W:2,B:5},king:{M:12,D:1}}};
function hop(b,amp){const f=((b%1)+1)%1;let lift=0,sq=0;if(f<0.55){lift=Math.sin(f/0.55*Math.PI)*amp;sq=-0.12*Math.sin(f/0.55*Math.PI);}else if(f<0.72)sq=0.22*(1-(f-0.55)/0.17);else if(f>0.9)sq=0.1;return {lift:Math.round(lift),sq};}
function slimeD(R,cx,gy,w,h,col,o){o=o||{};const sq=o.sq||0,W2=Math.max(4,Math.round(w*(1+sq))),H2=Math.max(3,Math.round(h*(1-sq))),top=gy-H2-(o.lift||0),sk=o.skip,sd=o.solid;
 const P=(x,y,i)=>{if(sk&&sk(x-cx,y-top))return;R(x,y,1,1,sd!=null?sd:i);};
 if(!o.noShadow&&sd==null){const sw=Math.round(W2*0.42*(1-Math.min(0.6,(o.lift||0)/30)));for(let x=-sw;x<=sw;x++)R(cx+x,gy,1,1,Math.abs(x)<sw*0.7?0:((x+gy)%2?0:5));}
 const fl=o.flash,M=fl?8:col.M,D=fl?2:col.D,Hc=fl?14:(col.H!=null?col.H:7);
 for(let yy=-1;yy<H2;yy++){const tt=Math.max(0,(yy+0.5)/H2);let hw=(W2/2)*Math.sqrt(Math.max(0,1-Math.pow(1-Math.min(1,tt/0.7),2)));if(tt>0.86)hw*=1-(tt-0.86)*0.9;hw=Math.round(hw);const y=top+yy;
  if(yy<0){const tw=Math.round(W2*0.16);for(let x=-tw;x<=tw;x++)P(cx+x,y,0);continue;}
  for(let x=-hw-1;x<=hw+1;x++){const edge=x<-hw||x>hw||yy===H2-1;P(cx+x,y,edge?0:(tt>0.7?D:M));}}
 const hx=Math.round(cx+W2*0.22),hyy=Math.round(top+H2*0.26),rx=Math.max(1,Math.round(W2*0.08)),ry=Math.max(1,Math.round(H2*0.13));
 for(let y=-ry;y<=ry;y++)for(let x=-rx;x<=rx;x++)if(x*x/(rx*rx+0.01)+y*y/(ry*ry+0.01)<=1)P(hx+x,hyy+y,Hc);
 const ex=Math.max(2,Math.round(W2*0.14)),ey=Math.round(top+H2*0.46),eh=Math.max(2,Math.round(H2*0.2));
 for(let y=0;y<eh;y++){P(cx-ex-1,ey+y,0);P(cx-ex,ey+y,0);P(cx+ex-1,ey+y,0);P(cx+ex,ey+y,0);}
 if(o.crown){const C=i=>sd!=null?sd:i;R(cx-4,top-5,9,4,C(0));R(cx-3,top-4,7,2,C(10));R(cx-3,top-6,1,2,C(10));R(cx,top-7,1,3,C(10));R(cx+3,top-6,1,2,C(10));R(cx,top-3,1,1,C(8));}
 return {top};}
const KNOB={'1a':{trk:'#c2b27a',cap:'#1f3320',capRim:null,ptr:'#e3d6a0',rim:null},'1c':{trk:'#2e2226',cap:'#6a4030',capRim:'#8a5a38',ptr:'#f2e2c4',rim:null}};
const SCN={
 title:{h:150,biome:'forest',walk:1},party:{h:84,biome:'forest',walk:1,mon:'cycle',att:1,hp:1},boss:{h:84,biome:'forest'},solo:{h:110},equip:{w:72,h:96},
 rec:{h:84,biome:'forest',mon:'boss',bubble:1},count:{h:84,biome:'forest',mon:'boss',click:1},steps:{h:84,biome:'cave',walk:1,mon:'cycle',att:1,hp:1},
 kit:{h:84,biome:'forest',chest:1},punch:{h:84,biome:'forest',walk:1,mon:'cycle',att:1,loop:1,hp:1},key:{h:84,biome:'forest',walk:1,flowers:1,birds:1},
 mix:{h:84,biome:'night',camp:1,att:1},erase:{h:84,biome:'cave',dissolve:1},fx:{h:84,biome:'storm',walk:1,mon:'cycle',att:1,hp:1,rain:0.6,mist:0.35,echo:1,dst:1,buff:1},
 menu:{h:60,biome:'night',sleep:1}};
const SLIME=["....0000....","..00MMMM00..",".0MMMMMMMM0.",".0M77MMMMM0.","0MM7MMMMMMM0","0MMMM0MM0MM0","0MMMM0MM0MM0","0MMMMMMMMMM0","0DDMMMMMMDD0",".0000000000."];
const BATA=["WW........WW","WWW.0000.WWW",".WWW0BB0WWW.","..WW8BB8WW..","....0BB0....",".....00....."];
const BATB=["....0000....","....0BB0....","..WW8BB8WW..",".WWW0BB0WWW.","WWW..00..WWW","WW........WW"];
const CL={
 warrior:{hair:4,skin:15,eye:1,tunic:7,shade:6,belt:4,pants:5,boot:4,shield:5,rim:6,blade:7,bladeSh:6,hilt:4},
 mage:{hat:12,band:9,hair:9,skin:15,eye:1,robe:12,trim:9,bow:13,boot:4,book:12,page:7},
 archer:{hair:1,beret:11,band:3,flower:10,skin:15,eye:12,shirt:7,vest:11,shorts:1,legs:6,boot:4,cape:3,bow:4,string:7,arrow:6},
 cleric:{hood:7,trim:10,hair:10,skin:15,eye:1,robe:7,stripe:10,staff:4,orb:12}};
const CLD={'1a':{cleric:{hood:6,trim:2,hair:3,skin:15,eye:1,robe:6,stripe:2,staff:4,orb:7},warrior:{hair:4,skin:15,eye:1,tunic:6,shade:3,belt:4,pants:5,boot:4,shield:5,rim:6,blade:7,bladeSh:6,hilt:4}}};let CURDIR='1a';
const RIVAL={hair:2,skin:13,eye:8,tunic:5,shade:1,belt:0,pants:1,boot:0,shield:2,rim:13,blade:13,bladeSh:2,hilt:0};
const ORDER=['cleric','archer','mage','warrior'];
const PX={cleric:12,archer:36,mage:62,warrior:88};
const HPV={mage:.81,archer:.6,cleric:.6,warrior:.6};
const PAT={mage:'0010000000100000',archer:'0000001000000010',cleric:'1000000000000000',warrior:'1000100010001000'};
const DUEL_H='1000100010000000',DUEL_R='0000000000001000',PSOLO='1000100010001010';
const hs=n=>{const s=Math.sin(n*127.1+311.7)*43758.5453;return s-Math.floor(s)};
const inr=(p,a,b)=>p!=null&&p>=a&&p<b;
const SKYC={};
function resolveP(ctx,dir){return PALS[dir].slice();}
const mkR=(c,P)=>(x,y,w,h,i)=>{c.fillStyle=P[i];c.fillRect(Math.round(x),Math.round(y),Math.max(1,Math.round(w)),Math.max(1,Math.round(h)))};
const disc=(R,cx,cy,r,i)=>{for(let dy=-r;dy<=r;dy++){const w=Math.floor(Math.sqrt(r*r-dy*dy));R(cx-w,cy+dy,2*w+1,1,i)}};
function spr(R,rows,x,y,map,o){o=o||{};const s=o.s||1,h=rows.length,w=rows[0].length;
 for(let j=0;j<h;j++){const row=rows[j];for(let i=0;i<w;i++){const ch=row[i];if(ch==='.')continue;if(o.skip&&o.skip(i,j))continue;
  let idx=o.solid!=null?o.solid:(map&&map[ch]!=null?map[ch]:parseInt(ch,16));if(isNaN(idx))continue;R(x+i*s,y+j*s,s,s,idx);}}}
function aph(pat,sF){if(!pat)return null;const L=pat.length,s=Math.floor(sF);for(let k=1;k<=2;k++){const n=s+k;if(pat[((n%L)+L)%L]==='1'&&n-sF<2)return (sF-n)/4;}
 for(let k=0;k<6;k++){const h=s-k;if(pat[((h%L)+L)%L]==='1'){const p=(sF-h)/4;return p<1.2?p:null;}}return null;}
function figure(R,cls,ox,oy,o){o=o||{};const s=o.s||1,C=o.cmap||(CLD[CURDIR]&&CLD[CURDIR][cls])||CL[cls],p=o.p,t=o.t||0,f=o.f||0;
 const sh=[],dt=[];const S=(x,y,w,h,i)=>sh.push([x,y,w,h,i]),D=(x,y,w,h,i)=>dt.push([x,y,w,h,i]);
 const L=(x0,y0,x1,y1,i)=>{const n=Math.max(Math.abs(x1-x0),Math.abs(y1-y0))||1;for(let k=0;k<=n;k++)D(Math.round(x0+(x1-x0)*k/n),Math.round(y0+(y1-y0)*k/n),1,1,i);};
 const blade=(x0,y0,x1,y1)=>{L(x0,y0+2,x1,y1+2,0);L(x0,y0+1,x1,y1+1,C.bladeSh);L(x0,y0,x1,y1,C.blade);};
 const res={};let dx=0;const by=f?-1:0;
 const legs=lc=>{if(o.sit)return;if(f){S(6,18,2,4,lc);S(8,18,2,4,lc);S(5,22,3,2,C.boot);S(8,22,3,2,C.boot);}else{S(5,18,2,4,lc);S(9,18,2,4,lc);S(4,22,3,2,C.boot);S(9,22,3,2,C.boot);}};
 if(cls==='warrior'){
  if(inr(p,0,0.6))dx=Math.round(3*Math.max(0,1-Math.abs(p-0.15)/0.45));if(p!=null&&p<0)dx=-1;
  legs(C.pants);
  S(1,11+by,4,7,C.shield);D(1,11+by,4,1,C.rim);D(2,13+by,2,2,C.rim);
  S(5,10+by,6,8,C.tunic);D(5,10+by,6,1,C.shade);D(5,15+by,6,1,C.belt);D(5,17+by,6,1,C.shade);
  S(3,1+by,9,9,C.hair);D(6,4+by,6,6,C.skin);D(6,4+by,1,3,C.hair);D(7,4+by,4,1,C.hair);D(9,6+by,1,2,C.eye);
  if(p==null||p>=0.6){S(10,11+by,2,4,C.tunic);S(11,14+by,2,2,C.skin);blade(12,15+by,19,21+by);D(11,15+by,2,1,C.hilt);}
  else if(p<0){S(9,8+by,2,4,C.tunic);S(8,6+by,2,2,C.skin);blade(8,5+by,3,-5+by);D(7,7+by,3,1,C.hilt);}
  else if(p<0.25){S(11,11+by,3,2,C.tunic);S(13,11+by,2,2,C.skin);blade(15,11+by,26,9+by);D(14,10+by,1,3,C.hilt);res.sw={cx:10,cy:12,prog:p/0.25,th:4};}
  else{S(11,12+by,3,2,C.tunic);S(13,13+by,2,2,C.skin);blade(14,14+by,23,19+by);D(14,13+by,1,3,C.hilt);res.sw={cx:10,cy:12,prog:1,th:Math.max(0.6,4*(1-(p-0.25)/0.35))};}
 }else if(cls==='mage'){const fy=o.rot?0:Math.round(-4+Math.sin(t*3)*1.5);const Y=v=>v+fy;
  S(6,Y(20),2,2,C.boot);S(9,Y(20),2,1,C.boot);S(3,Y(5),3,8,C.hair);
  S(4,Y(10),8,8,C.robe);S(3,Y(16),10,4,C.robe);D(7,Y(12),1,8,C.trim);D(3,Y(19),10,1,C.trim);
  const fl=Math.floor(t*10)%2;S(2,Y(9),2,3,C.robe);D(1,Y(7),3,2,9);D(2,Y(5-fl),2,2,10);D(1,Y(6),1,1,8);
  S(5,Y(5),6,5,C.skin);D(5,Y(6),6,1,C.hair);D(9,Y(7),1,2,C.eye);
  S(1,Y(4),13,2,C.hat);S(4,Y(1),7,3,C.hat);S(3,Y(-1),5,2,C.hat);S(2,Y(-3),3,2,C.hat);S(1,Y(-4),2,1,C.hat);D(4,Y(3),7,1,C.band);
  D(6,Y(10),4,2,C.bow);S(11,Y(11),2,3,C.robe);D(12,Y(13),1,1,C.skin);
  const bb=Math.round(Math.sin(t*3+1)),open=p!=null&&p>-0.4&&p<0.6;
  if(open){S(13,Y(8+bb),6,4,C.book);D(14,Y(9+bb),2,2,C.page);D(17,Y(9+bb),1,2,C.page);D(15,Y(6+bb),2,1,10);}else{S(13,Y(8+bb),4,5,C.book);D(14,Y(9+bb),2,3,C.page);}
  res.src={x:18,y:Y(10+bb)};
 }else if(cls==='archer'){
  legs(C.legs);
  S(2,10+by,4,10,C.cape);S(5,10+by,6,6,C.shirt);D(5,13+by,6,3,C.vest);S(5,16+by,6,2,C.shorts);
  S(3,2+by,9,8,C.hair);D(6,4+by,6,6,C.skin);D(6,4+by,5,1,C.hair);D(9,6+by,1,2,C.eye);
  S(2,0+by,10,3,C.beret);D(2,2+by,10,1,C.band);D(11,0+by,2,2,C.flower);
  [[13,3,1,2],[14,5,1,2],[15,7,1,2],[16,9,1,6],[15,15,1,2],[14,17,1,2],[13,19,1,2]].forEach(q=>S(q[0],q[1]+by,q[2],q[3],C.bow));
  if(p!=null&&p<0){L(13,3+by,9,12+by,C.string);L(9,12+by,13,20+by,C.string);L(9,12+by,19,12+by,C.arrow);D(19,11+by,1,3,C.arrow);S(8,11+by,3,2,C.shirt);S(15,11+by,2,2,C.skin);}
  else{L(13,4+by,13,20+by,C.string);S(11,11+by,3,2,C.shirt);S(15,11+by,2,2,C.skin);}
  res.src={x:19,y:12+by};
 }else{const cast=p!=null&&p>-0.3&&p<1,sy=cast?-4:0;
  S(4,10,8,8,C.robe);S(3,17,10,7,C.robe);D(7,11,2,13,C.stripe);D(3,23,10,1,C.stripe);
  S(3,1,10,9,C.hood);D(6,4,6,6,C.skin);D(6,4,6,1,C.hair);D(9,6,1,2,C.eye);D(3,1,10,1,C.trim);
  S(13,0+sy,1,23,C.staff);S(12,-3+sy,3,3,C.trim);D(13,-2+sy,1,1,cast?7:C.orb);
  S(11,11+Math.round(sy/2),2,3,C.robe);D(12,13+Math.round(sy/2),2,2,C.skin);
  res.src={x:13,y:-2+sy};res.cast=cast;}
 const E=(x,y,w,h,i)=>{x+=dx;let X=o.flip?16-x-w:x,Yy=y,Ww=w,Hh=h;if(o.rot){const nx=Yy+6,ny=16-X-Ww;X=nx;Yy=ny;Ww=h;Hh=w;}R(ox+X*s,oy+Yy*s,Ww*s,Hh*s,o.solid!=null?o.solid:i);};
 if(!o.noOut)sh.forEach(q=>E(q[0]-1,q[1]-1,q[2]+2,q[3]+2,0));sh.forEach(q=>E(q[0],q[1],q[2],q[3],q[4]));dt.forEach(q=>E(q[0],q[1],q[2],q[3],q[4]));
 const A=(px,py)=>{let X=px+dx;if(o.flip)X=16-X;return {x:ox+X*s,y:oy+py*s};};
 if(res.sw){const a=A(res.sw.cx,res.sw.cy);res.sw.x=a.x;res.sw.y=a.y;}
 if(res.src){const a=A(res.src.x,res.src.y);res.src.ax=a.x;res.src.ay=a.y;}
 return res;}
function swoosh(R,cx,cy,r,th,prog,col,col2,flip){const a0=-115,span=175,a1=a0+span*Math.max(0.05,Math.min(1,prog)),rr=Math.ceil(r+th+1);
 for(let y=-rr;y<=rr;y++)for(let x=-rr;x<=rr;x++){const d=Math.sqrt(x*x+y*y);if(d<r)continue;const a=Math.atan2(y,flip?-x:x)*180/Math.PI;if(a<a0||a>a1)continue;
  const u=(a-a0)/(a1-a0+1e-3),w=th*Math.sin(Math.PI*u);if(d<=r+w)R(cx+x,cy+y,1,1,d>r+w*0.5?col:col2);}}
function fireball(R,x,y,sc){sc=sc||1;R(x-2*sc,y,2*sc,1,8);R(x-4*sc,y+1,2*sc,1,9);R(x-1,y-1,3*sc,3*sc,9);R(x,y,Math.max(1,sc),Math.max(1,sc),10);}
function meteor(R,x,y,sc){for(let k=1;k<=9;k++){const tx=x-k*1.6*sc,ty=y-k*1.4*sc;R(tx,ty,Math.max(1,(3-k/4)*sc),Math.max(1,(3-k/4)*sc),k<3?10:k<6?9:8);}R(x-2*sc,y-2*sc,5*sc,5*sc,0);R(x-1.5*sc,y-1.5*sc,4*sc,4*sc,9);R(x-sc,y-sc,3*sc,3*sc,10);R(x-0.5*sc,y-0.5*sc,Math.max(1,sc),Math.max(1,sc),7);}
function summon(R,x,y,k,sc){const r=Math.max(1,Math.round(k*3*sc));for(let q=0;q<10;q++){const ang=q*0.628+k*7,rr=r+4+Math.sin(k*9+q)*1.5;R(x+Math.cos(ang)*rr,y+Math.sin(ang)*rr*0.7,1,1,q%2?10:9);}if(k<0.35){R(x,y-3,1,7,7);R(x-3,y,7,1,7);}R(x-r-1,y-r-1,2*r+3,2*r+3,0);R(x-r,y-r,2*r+1,2*r+1,9);R(x-r+1,y-r+1,Math.max(1,2*r-1),Math.max(1,2*r-1),10);R(x,y,1,1,7);}
function arcPt(x0,y0,x1,y1,q,h){return [x0+(x1-x0)*q,y0+(y1-y0)*q-Math.sin(q*Math.PI)*h];}
function arcArrow(R,x0,y0,x1,y1,q,h,col,trail){const a=arcPt(x0,y0,x1,y1,Math.max(0,q-0.08),h),b=arcPt(x0,y0,x1,y1,q,h);arrow(R,a[0],a[1],b[0],b[1],col,null);if(trail!=null)for(let k=1;k<6;k++){const p=arcPt(x0,y0,x1,y1,Math.max(0,q-0.08-k*0.04),h);if(k%2)R(p[0],p[1],1,1,trail);}}
function burst(R,x,y,q,cols){for(let k=0;k<10;k++){const a=k*0.628+q,r=2+q*12;R(x+Math.cos(a)*r,y+Math.sin(a)*r,q<0.5?2:1,q<0.5?2:1,cols[k%cols.length]);}}
function arrow(R,x0,y0,x1,y1,col,trail){const dx=x1-x0,dy=y1-y0,l=Math.sqrt(dx*dx+dy*dy)||1,ux=dx/l,uy=dy/l;for(let k=0;k<7;k++)R(x1-ux*k,y1-uy*k,1,1,col);R(x1+ux,y1+uy,1,1,7);if(trail!=null)for(let k=0;k<10;k++)if(k%2===0)R(x1-ux*(9+k*2)+Math.sin(k)*1.5,y1-uy*(9+k*2)+Math.cos(k)*1.5,1,1,trail);}
function slimeKing(R,bx,by,M,solid){spr(R,SLIME,bx,by,M.slime,{s:4,solid});const C=i=>solid!=null?solid:i;R(bx+13,by-1,22,1,C(0));R(bx+14,by-6,20,5,C(10));R(bx+14,by-10,4,4,C(10));R(bx+22,by-12,4,6,C(10));R(bx+30,by-10,4,4,C(10));R(bx+23,by-4,2,2,C(8));}
function knight(R,x,G,b,beat,solid){const K=(a,b2,w,h,i)=>R(a,b2,w,h,solid!=null?solid:i);
 K(x+10,G-16,7,16,0);K(x+11,G-16,5,15,5);K(x+24,G-16,7,16,0);K(x+25,G-16,5,15,5);
 K(x+6,G-38+b,30,23,0);K(x+7,G-37+b,28,21,6);K(x+7,G-37+b,28,2,7);K(x+7,G-20+b,28,3,10);
 K(x+12,G-50+b,18,13,0);K(x+13,G-49+b,16,11,6);K(x+15,G-45+b,12,2,0);K(x+17,G-45+b,3,2,(beat%1)<0.4?8:9);K(x+19,G-54+b,4,5,8);
 K(x+38,G-54+b,3,26,6);K(x+38,G-54+b,1,26,7);K(x+34,G-28+b,11,2,10);K(x+38,G-26+b,3,6,4);
 K(x-4,G-40+b,16,25,0);K(x-3,G-39+b,14,23,4);K(x-3,G-39+b,14,1,10);K(x-3,G-39+b,1,23,10);K(x+10,G-39+b,1,23,10);K(x-3,G-17+b,14,1,10);K(x+2,G-32+b,4,8,10);K(x+1,G-29+b,6,2,10);}
function treeAt(R,S,st,x,y,s){if(st==='round'){const th=Math.max(2,Math.round(40*s)),r=Math.max(2,Math.round(16*s));R(x-Math.round(2*s),y-th,4*s,th,S.treeSh);disc(R,Math.round(x),Math.round(y-th-r+2),r,S.tree_c);disc(R,Math.round(x-r*0.3),Math.round(y-th-r*1.3+2),Math.max(1,Math.round(r*0.45)),S.treeHi);}
 else{const th=Math.round(70*s),tr=6*s;R(x-1,y-tr,3*s,tr,4);for(let j=0;j<th;j++){const w=Math.floor((j%Math.max(2,Math.round(10*s)))*0.5+j*0.3);R(x-w,y-tr-th+j,2*w+1,1,S.tree_c);R(x+w,y-tr-th+j,1,1,S.treeHi);}}}
function drawEquip(R,Y,t,cls){const W=72,H=96;R(0,0,W,H,Y.eqBg);
 for(let y=0;y<H;y+=1)for(let x=(y%2);x<W;x+=2){const dx=x-36,dy=y-50;if(dx*dx/900+dy*dy/1700<1)R(x,y,1,1,Y.eqGlow);}
 for(let dy=-4;dy<=4;dy++){const w=Math.round(28*Math.sqrt(1-dy*dy/16));R(36-w,86+dy,2*w,1,dy<-2?Y.pedHi:Y.ped);}
 for(let k=0;k<6;k++){const p=(t*0.4+k/6)%1;R(10+hs(k)*52,84-p*70,1,1,Y.eqGlow===7?7:10);}
 figure(R,cls||'mage',8,cls==='mage'?32:26,{s:3,t,p:(t%2.4<0.8)?0.1:null});}
function drawDuel(c,R,dir,Y,W,H,t,sF,ST){const O=Y.solo;
 const ph=ST.playing?aph(ST.duelH,sF):null,pr=ST.playing?aph(ST.duelR,sF):null,shake=(inr(ph,0.05,0.16)||inr(pr,0.05,0.16))?2:0;
 c.setTransform(1,0,0,1,0,0);R(0,0,W,H,O.sky[O.sky.length-1]);
 const sx=shake?Math.round((hs(Math.floor(t*30))*2-1)*shake):0,sy=shake?Math.round((hs(Math.floor(t*30)+3)*2-1)*shake):0;c.setTransform(1,0,0,1,sx,sy);
 const n=O.sky.length;for(let i=0;i<n;i++){const y0=Math.round(i*H*0.7/n),y1=Math.round((i+1)*H*0.7/n);R(-4,y0-4,W+8,y1-y0+4,O.sky[i]);}
 const gy=x=>Math.round(102-x*0.2);
 for(let i=0;i<26;i++){const len=20+Math.floor(hs(i+3)*50),off=14+Math.floor(hs(i)*80),x0=((hs(i+9)*400-t*(120+hs(i)*90))%400+400)%400-80;for(let k=0;k<len;k++){const x=x0+k;R(x,gy(x)-off,1,1,i%3?O.dash:O.edge);}}
 for(let x=-4;x<W+4;x++){const y=gy(x);R(x,y,1,H-y+6,O.floor);R(x,y,1,1,O.edge);}
 for(let i=0;i<24;i++){const x0=((hs(i)*300-t*50)%300+300)%300-30,off=4+Math.floor(hs(i+1)*22);for(let k=0;k<6;k++)R(x0+k,gy(x0+k)+off,1,1,O.dash);}
 const home=38,rhome=164;let hx=home,rx=rhome;
 if(ph!=null){if(ph<0)hx=home-3;else if(ph<0.15)hx=home+(rhome-34-home)*(ph/0.15);else if(ph<0.6)hx=rhome-34;else if(ph<1)hx=rhome-34+(home-(rhome-34))*((ph-0.6)/0.4);}
 if(inr(ph,0.12,0.6))rx=rhome+Math.round(10*Math.min(1,(ph-0.12)/0.1));
 if(pr!=null){if(pr<0)rx=rhome+3;else if(pr<0.15)rx=rhome+((home+34)-rhome)*(pr/0.15);else if(pr<0.6)rx=home+34;else if(pr<1)rx=home+34+(rhome-(home+34))*((pr-0.6)/0.4);}
 if(inr(pr,0.12,0.6))hx=home-Math.round(10*Math.min(1,(pr-0.12)/0.1));
 hx=Math.round(hx);rx=Math.round(rx);const s=2,hy=gy(hx+16)-48,ry=gy(rx+16)-48;
 for(let k=0;k<12;k++){R(hx+4+k*2,gy(hx+16)+1,1,1,O.shadow);R(rx+4+k*2,gy(rx+16)+1,1,1,O.shadow);}
 if(inr(ph,0,0.15))for(let k=1;k<=2;k++){const gx=Math.round(hx-k*12);figure(R,'warrior',gx,gy(gx+16)-48,{s,p:ph,t,solid:13,noOut:1});}
 if(inr(pr,0,0.15))for(let k=1;k<=2;k++){const gx=Math.round(rx+k*12);figure(R,'warrior',gx,gy(gx+16)-48,{s,p:pr,t,flip:1,cmap:RIVAL,solid:2,noOut:1});}
 const rr=figure(R,'warrior',rx,ry,{s,p:pr,t,flip:1,cmap:RIVAL,solid:inr(ph,0.1,0.22)?7:null});
 const hr=figure(R,'warrior',hx,hy,{s,p:ph,t,solid:inr(pr,0.1,0.22)?7:null});
 if(hr.sw)swoosh(R,hr.sw.x,hr.sw.y,16,hr.sw.th*2,hr.sw.prog,Y.arc,Y.arc2,false);
 if(rr.sw)swoosh(R,rr.sw.x,rr.sw.y,16,rr.sw.th*2,rr.sw.prog,Y.pink,13,true);
 if(inr(ph,0.1,0.35))burst(R,rx+10,ry+22,(ph-0.1)/0.25*0.6,[7,10,Y.arc2]);
 if(inr(pr,0.1,0.35))burst(R,hx+22,hy+22,(pr-0.1)/0.25*0.6,[7,Y.pink]);
 c.setTransform(1,0,0,1,0,0);}
function drawScene(cv,dir,key,t,ST){
 const cfg0=SCN[key];if(!cfg0||!PALS[dir])return;const cfg=Object.assign({},cfg0);if(cfg.loop&&!ST.loopOn)cfg.loop=0;const W=cfg.w||240,H=cfg.h;
 if(cv.width!==W||cv.height!==H){cv.width=W;cv.height=H;}
 CURDIR=dir;const c=cv.getContext('2d');const R=mkR(c,PALS[dir]),Y=STY[dir],M=MON[dir];
 const bpm=ST.bpm,beat=ST.beat,sF=beat*4,bar=240/bpm;
 if(key==='equip'){drawEquip(R,Y,t,ST.eqCls);return;}
 if(key==='solo'){drawDuel(c,R,dir,Y,W,H,t,sF,ST);return;}
 const fxv=ST.fx||{dst:0,cho:0,dly:0,rev:0};
 if(cfg.att||key==='fx'){const any=Math.max(fxv.dst,fxv.cho,fxv.dly,fxv.rev)>0.08;
  cfg.rain=fxv.rev>0.08?0.15+fxv.rev*0.85:0;cfg.mist=fxv.cho>0.08?0.1+fxv.cho*0.5:0;cfg.echo=fxv.dly>0.12?1:0;cfg.dst=fxv.dst>0.12?1:0;cfg.buff=any?1:0;}
 const buffIdx=['dst','cho','dly','rev'].map((k,i)=>[fxv[k],i]).sort((a,b)=>b[0]-a[0])[0][1];
 const G=H-14;const bi=((Math.floor(beat)%4)+4)%4,bp=beat-Math.floor(beat);
 let shake=0;if(cfg.dissolve&&(beat%8)>=1.5&&(beat%8)<1.9)shake=2;if(key==='boss'&&((bi===0&&bp>=0.2&&bp<0.42)||(bi===3&&bp>=0.3&&bp<0.45)))shake=2;
 c.setTransform(1,0,0,1,shake?Math.round((hs(Math.floor(t*30))*2-1)*shake):0,shake?Math.round((hs(Math.floor(t*30)+5)*2-1)*shake):0);
 let sp=cfg.walk?ST.sp:0;if(cfg.loop)sp=300+((((beat%4)+4)%4)/4)*bar*bpm*0.22;
 const biome=(key==='boss')?'cave':(cfg.biome==='forest'&&key!=='title'&&key!=='kit'?ST.biome:cfg.biome);
 const S=Object.assign({},Y,biome==='night'?Y.night:biome==='storm'?Y.storm:biome==='coast'?Y.coast:biome==='ruins'?Y.ruins:Y.base);
 if(biome==='cave'){
  R(-4,-4,W+8,H+8,0);
  const ps=sp*0.35;for(let k=Math.floor(ps/44)-1;k<Math.floor((ps+W)/44)+2;k++){const x=Math.round(k*44-ps);R(x,0,14,G,1);R(x,0,2,G,5);
   if(((k%2)+2)%2===0){R(x+6,G-30,2,5,4);const fl=hs(k+Math.floor(t*8))>0.5;R(x+6,G-33,2,3,9);R(x+6+(fl?0:1),G-35,1,2,10);for(let q=0;q<8;q++){const a=q*0.785;R(x+7+Math.cos(a)*6,G-32+Math.sin(a)*6,1,1,4);}}}
  for(let x=0;x<W;x++){const wx=Math.floor((x+sp*0.9)/3);const h=4+Math.floor(hs(wx)*5)+(hs(wx+3)>0.85?8:0);R(x,0,1,h,5);}
  R(-4,G,W+8,H-G+4,5);R(-4,G,W+8,1,6);
  for(let r=0;r<3;r++){const y=G+4+r*4;R(0,y,W,1,0);const off=(r%2)*6;for(let k=Math.floor(sp/12)-1;k<Math.floor((sp+W)/12)+1;k++)R(k*12+off-sp,y-3,1,3,0);}
 }else{
  const ck=dir+biome+H;let sc=SKYC[ck];
  if(!sc){sc=document.createElement('canvas');sc.width=W;sc.height=H;const RR=mkR(sc.getContext('2d'),PALS[dir]);const cols=S.sky,n=cols.length;
   for(let i=0;i<n;i++){const y0=Math.round(i*G/n),y1=Math.round((i+1)*G/n);RR(0,y0,W,y1-y0,cols[i]);if(i>0&&cols[i]!==cols[i-1])for(let x=0;x<W;x+=2){RR(x,y0,1,1,cols[i-1]);if(x%4===0)RR(x+1,y0+1,1,1,cols[i-1]);}}
   if(S.stars)for(let i=0;i<30;i++)RR(Math.floor(hs(i)*W),Math.floor(hs(i+50)*(G-30)),1,1,hs(i+9)>0.7?6:7);
   if(S.orb==='moon'){disc(RR,198,14,7,7);RR(195,11,2,2,6);RR(200,16,3,2,6);}
   if(S.orb==='sunset'){const cy=G-28;disc(RR,172,cy,15,10);for(let k=0;k<4;k++)RR(150,cy+3+k*4,46,k+1,9);}
   SKYC[ck]=sc;}
  c.drawImage(sc,0,0);
  if(cfg.dst&&(beat%4)<0.18){R(0,0,W,G-10,6);let x=40+Math.floor(hs(Math.floor(beat/4))*160);for(let y=0;y<G-26;y+=3){R(x,y,1,3,7);x+=(hs(y+Math.floor(beat/4))>0.5?1:-1)*2;}}
  if(biome==='storm')for(let k=0;k<7;k++){const cx=((k*53-t*6-sp*0.2)%300+300)%300-30,cy=5+(k%3)*5;R(cx,cy,30,5,6);R(cx+6,cy-3,16,3,6);R(cx+2,cy+5,26,2,13);}
  if(S.mnt!=null)for(let x=0;x<W;x++){const wx=x+sp*0.12;const h=Math.round(24+8*Math.sin(wx*0.045)+5*Math.sin(wx*0.13+2));R(x,G-h,1,h,S.mnt);}
  if(S.mnt2!=null)for(let x=0;x<W;x++){const wx=x+sp*0.3;const h=Math.round(11+5*Math.sin(wx*0.07+1)+3*Math.sin(wx*0.19));R(x,G-h,1,h,S.mnt2);}
  const tsp=Y.tree==='round'?22:24,ts=sp*0.6;
  if(S.sea!=null){R(0,G-9,W,9,S.sea);for(let x=0;x<W;x++){const w=Math.floor((x+t*10+sp*0.5)/5)%2;if(w)R(x,G-9+Math.floor(hs(Math.floor((x+sp*0.5)/7))*8),2,1,7);}R(0,G-9,W,1,6);}
  if(S.ruinsOn){const rp=36;for(let k=Math.floor(sp*0.6/rp)-1;k<Math.floor((sp*0.6+W)/rp)+2;k++){const x=Math.round(k*rp+hs(k)*14-sp*0.6),hh=12+Math.floor(hs(k+4)*20),bw=7;R(x-1,G-hh-1,bw+2,hh+1,0);R(x,G-hh,bw,hh,6);R(x,G-hh,2,hh,7);R(x+bw-2,G-hh,2,hh,5);if(hs(k+8)>0.5){R(x-2,G-hh-3,bw+4,3,0);R(x-1,G-hh-2,bw+2,1,7);}else{R(x+2,G-hh,3,2,5);}for(let q=0;q<hh;q+=4)R(x+1,G-q-2,bw-2,1,5);}}
  if(S.trees!==false)for(let k=Math.floor(ts/tsp)-1;k<Math.floor((ts+W)/tsp)+2;k++){const x=Math.round(k*tsp+hs(k)*12-ts),th=10+Math.floor(hs(k+9)*12);
   if(Y.tree==='pine'){R(x,G-3,1,3,4);for(let j=0;j<th;j++){const w=Math.floor((j%5)*0.7+j*0.22);R(x-w,G-3-th+j,w*2+1,1,S.tree_c);R(x+w,G-3-th+j,1,1,S.treeHi);}}
   else{R(x-1,G-6,2,6,S.treeSh);const r=Math.max(4,Math.round(th*0.42)),cy=G-6-r+2;for(let yy=cy-r;yy<=cy+r;yy++)for(let xx=x-r;xx<=x+r;xx++){const ddx=xx-x,ddy=yy-cy;if(ddx*ddx+ddy*ddy>r*r)continue;let col=S.tree_c;if(ddx+ddy>r*0.5&&(xx+yy)%2===0)col=S.treeSh;else if(ddx+ddy<-r*0.5&&(xx+yy)%2===0)col=S.treeHi;R(xx,yy,1,1,col);}}}
  if(Y.ground==='path'){R(-4,G,W+8,H-G+4,S.dirt);R(-4,G,W+8,2,S.grass);for(let x=0;x<W;x+=2)R(x,G+2,1,1,S.grass);
   for(let k=Math.floor(sp/6)-1;k<Math.floor((sp+W)/6)+1;k++){const x=Math.round(k*6-sp),v=hs(k*3.1);if(v>0.55){R(x,G-1,1,1,S.grass);R(x+1,G-2,1,2,S.grass);}if(v>0.3&&v<0.5)R(x+2,G+5+Math.floor(hs(k+2)*(H-G-7)),2,1,S.dirt2);if(cfg.flowers&&v>0.78)R(x,G-3,1,1,8);}}
  else{R(-4,G,W+8,H-G+4,S.dirt);R(-4,G,W+8,2,S.grass);R(-4,G+2,W+8,1,S.grass2);
   for(let k=Math.floor(sp/6)-1;k<Math.floor((sp+W)/6)+1;k++){const x=Math.round(k*6-sp),v=hs(k*3.1);if(v>0.55)R(x,G-1,1,1,S.grass);if(v>0.3&&v<0.5)R(x+2,G+5+Math.floor(hs(k+2)*(H-G-7)),2,1,S.dirt2);if(cfg.flowers&&v>0.78){R(x,G-2,1,2,S.grass2);R(x,G-3,1,1,hs(k+4)>0.5?14:10);}}}
 }
 if(cfg.birds)for(let i=0;i<3;i++){const bx=((i*90+t*22)%280)-20,by=12+i*6+Math.round(Math.sin(t*3+i)*2),fp=Math.floor(t*6+i)%2;R(bx,by+fp,1,1,Y.bird);R(bx+1,by+1,1,1,Y.bird);R(bx+2,by+fp,1,1,Y.bird);}
 const fire=(x,g)=>{const f=Math.floor(t*10)%3;for(let dx=-12;dx<=12;dx+=2)R(x+dx,g,1,1,9);R(x-6,g-2,12,2,4);R(x-3,g-7,7,5,8);R(x-2,g-9-(f===1?1:0),5,6,9);R(x-1,g-11-(f===2?1:0),3,5,10);R(x,g-12-f,1,2,7);R(x-3+f*3,g-15-f*2,1,1,9);};
 const zz=(x,y,n)=>{for(let k=0;k<2;k++){const p=((t*0.7+k*0.5+n*0.23)%1);const zx=x+p*8,zy=y-p*14;R(zx,zy,3,1,7);R(zx+1,zy+1,1,1,7);R(zx,zy+2,3,1,7);}};
 const ph={};ORDER.forEach(cl=>ph[cl]=(cfg.att&&ST.playing&&!ST.mute[cl])?aph(ST.pats[cl],sF):null);
 const hy=G-24;
 if(key==='boss'){bossScene(R,dir,Y,M,W,H,G,t,beat,bi,bp);c.setTransform(1,0,0,1,0,0);return;}
 let mon=null;
 if(cfg.mon==='cycle'){const per=62,dieX=124,list=[];const k0=Math.floor((sp-300)/per)-1;
  for(let k=k0;k<k0+8;k++){const mx=k*per-sp+300;if(mx>W+20||mx<dieX-20)continue;list.push({k,mx,kind:((k%3)+3)%3===2?'bat':'slime'});}
  const hhs={};list.forEach(m=>{hhs[m.k]=hop(beat+m.k*0.37,4);m.my=m.kind==='bat'?G-40+Math.round(Math.sin(t*5+m.k)*2):G-20;});
  const tg=list.filter(m=>m.mx>=dieX&&m.mx<W-16).sort((x,y)=>x.mx-y.mx)[0]||null;
  if(tg)mon={x:tg.mx+12,y:tg.kind==='bat'?tg.my+6:G-8-hhs[tg.k].lift};
  if(!tg)ORDER.forEach(cl=>ph[cl]=null);else if(mon.x-12-(PX.warrior+16)>44)ph.warrior=null;
  list.forEach(m=>{const hh=hhs[m.k];
   if(m.mx>=dieX){const fl=tg&&m.k===tg.k&&(inr(ph.warrior,0,0.25)||inr(ph.mage,0.62,0.76)||inr(ph.archer,0.38,0.52));
    if(m.kind==='bat')spr(R,Math.floor(beat*2+m.k)%2?BATB:BATA,m.mx,m.my,M.bat,{s:2,solid:fl?7:null});else slimeD(R,m.mx+12,G,22,16,M.slime,{lift:hh.lift,sq:hh.sq,flash:fl});
    if(cfg.hp){const hpv=Math.max(0.08,Math.min(1,(m.mx-dieX)/(W-30-dieX))),top=m.kind==='bat'?m.my-5:G-22-hh.lift;R(m.mx,top,24,3,0);R(m.mx+1,top+1,Math.max(1,Math.round(22*hpv)),1,8);}}
   else{const age=dieX-m.mx;for(let p=0;p<10;p++){const ang=p*0.628,r=age*1.1;R(m.mx+12+Math.cos(ang)*r,m.my+8+Math.sin(ang)*r,age<9?2:1,age<9?2:1,age<10?7:6);}}});}
 if(cfg.mon==='boss'){const hk=hop(beat,cfg.click?5:3);const kr=slimeD(R,180,G,44,32,M.king,{crown:1,lift:hk.lift,sq:hk.sq});
  if(cfg.bubble&&(beat%1)<0.7){const x=202,y=kr.top-14;R(x,y,7,11,0);R(x+1,y+1,5,9,7);R(x+3,y+2,1,5,8);R(x+3,y+8,1,1,8);}}
 if(cfg.chest){const cx=150,cy=G-12;R(cx,cy,20,12,0);R(cx+1,cy+1,18,10,4);R(cx+1,cy+5,18,1,10);R(cx+9,cy+4,2,3,10);R(cx,cy-6,20,5,0);R(cx+1,cy-5,18,3,4);
  const it=[12,11,10,9];for(let k=0;k<4;k++){const ix=cx+2+k*4.5,iy=cy-16-Math.round(Math.sin(t*3+k)*2)-(k%2)*4;R(ix-1,iy-1,5,5,0);R(ix,iy,3,3,it[k]);}
  for(let k=0;k<4;k++)if(hs(k+Math.floor(t*4))>0.5)R(cx+Math.floor(hs(k*7)*20),cy-26-Math.floor(hs(k*3)*8),1,1,7);}
 let bomb=null;if(cfg.dissolve){const cyc=((beat%8)+8)%8,k=Math.floor(beat/8);ph.warrior=cyc<0.45?-0.2:cyc<0.7?0.1:null;
  [152,194].forEach((mx,m)=>{const hp=hop(beat+m*0.4,3);let dph=0;if(cyc>=1.8&&cyc<6)dph=Math.min(1.1,(cyc-1.8)/1.4);else if(cyc>=6)dph=Math.max(0,1.1-(cyc-6)/1.6);
   const sk=dph>0?(x,y)=>hs(x*13+y*7+k*31+m*5+500)<dph:null;slimeD(R,mx,G,22,16,M.slime,{lift:dph>0?0:hp.lift,sq:dph>0?0:hp.sq,flash:cyc>=1.5&&cyc<2.1,skip:sk,noShadow:dph>0.4});
   if(dph>0&&cyc<6)for(let n=0;n<14;n++)if(hs(n+k+m)<dph)R(mx-11+hs(n*3+m)*22,G-8-dph*24*hs(n+7)-n%3,1,1,n%2?8:7);});bomb={cyc};}
 if(cfg.camp){fire(46,G);const restC=ORDER.filter(cl=>ST.mute[cl]),fightC=ORDER.slice().reverse().filter(cl=>!ST.mute[cl]);
  const sit=[20,2,62];restC.forEach((cl,n)=>{figure(R,cl,sit[n]||2,hy+4,{t,sit:1});zz((sit[n]||2)+10,G-22,n);});
  const bx=184,by=G-40+Math.round(Math.sin(t*4)*3);const tgt={x:bx+12,y:by+6};const fl=inr(ph.mage,0.62,0.76)||inr(ph.warrior,0,0.25);
  if(fightC.length)spr(R,Math.floor(beat*2)%2?BATB:BATA,bx,by,M.bat,{s:2,solid:fl?7:null});
  const slots=[82,106,132,156],pos={},rs={};fightC.forEach((cl,n)=>{pos[cl]=slots[n];rs[cl]=figure(R,cl,slots[n],hy,{p:ph[cl],t,f:ST.playing?Math.floor(beat*2+n)%2:0});});
  fx(R,Y,rs,ph,tgt,pos,G);
 }else if(cfg.sleep){fire(120,G);[['cleric',52],['archer',80],['mage',136],['warrior',164]].forEach(([cl,x],n)=>{figure(R,cl,x,G-16,{rot:1,t});zz(x+6,G-18,n);});
 }else{
  if(cfg.echo)ORDER.forEach((cl,i)=>figure(R,cl,PX[cl]-8,hy,{f:(cfg.walk&&ST.playing)?Math.floor(beat*2+i+1)%2:0,t:t-0.2,solid:13,noOut:1}));
  const bph=cfg.buff?((beat%4)/4)*1.1-0.05:null;
  let wx=PX.warrior,wy=0,wf=null;const pw=ph.warrior;
  if(mon&&cfg.att){const appr=Math.max(PX.warrior,Math.min(mon.x-34,PX.warrior+72));
   if(pw!=null){if(pw<0){const q=Math.min(1,1+pw/0.5);wx=PX.warrior+(appr-PX.warrior)*q;wf=Math.floor(t*14)%2;}else if(pw<0.3)wx=appr;else if(pw<0.75){const q=(pw-0.3)/0.45;wx=appr+(PX.warrior-appr)*q;wy=-Math.round(Math.sin(q*Math.PI)*9);}}
   const gap=mon.x-12-(wx+16);if(gap<6&&(pw==null||pw>=0.75)){wx-=Math.min(16,6-gap);wy=-Math.round(Math.abs(Math.sin(t*8))*3);}}
  const rs={};ORDER.forEach((cl,i)=>{const isW=cl==='warrior';rs[cl]=figure(R,cl,isW?Math.round(wx):PX[cl],hy+(isW?wy:0),{p:cl==='cleric'&&cfg.buff?bph:ph[cl],f:isW&&wf!=null?wf:((cfg.walk&&ST.playing)?Math.floor(beat*2+i)%2:0),t});});
  if(wf!=null)for(let k=0;k<5;k++)R(wx-4-hs(k)*16,hy+6+k*3,6+hs(k+2)*8,1,Y.arc2);
  if(mon&&inr(pw,0,0.15)){const sx=mon.x-10,sy=mon.y;R(sx-5,sy,11,1,7);R(sx,sy-5,1,11,7);for(let n=-3;n<=3;n++){R(sx+n,sy+n,1,1,10);R(sx+n,sy-n,1,1,10);}}
  fx(R,Y,rs,cfg.buff?Object.assign({},ph,{cleric:bph}):ph,mon,PX,G,cfg.buff?Y.buff[buffIdx]:null);
  if(cfg.hp)ORDER.forEach(cl=>{const x=PX[cl]+2;R(x,G+4,13,3,0);R(x+1,G+5,Math.round(11*ST.hp[cl]),1,Y.hp);});
 }
 if(bomb){const cyc=bomb.cyc,bx0=PX.warrior+16,by0=G-16,tx=173,ty=G-6;
  if(cyc>=0.6&&cyc<1.5){const pt=arcPt(bx0,by0,tx,ty,(cyc-0.6)/0.9,30),bx=Math.round(pt[0]),by=Math.round(pt[1]),sp2=Math.floor(t*20)%2;R(bx-3,by-3,7,7,0);R(bx-2,by-2,5,5,5);R(bx-1,by-2,2,1,6);R(bx+1,by-5,1,2,4);R(bx+1+sp2,by-6,1,1,sp2?10:9);}
  if(cyc>=1.5&&cyc<2.8){const q=(cyc-1.5)/1.3,r=Math.round(q<0.3?q/0.3*24:24*(1-(q-0.3)/0.7*0.5));if(q<0.65)disc(R,tx,ty-6,r,0);
   for(let n=0;n<8;n++){const a=n*0.785+0.4,rr=r+3+q*8;R(tx+Math.cos(a)*rr-2,ty-6+Math.sin(a)*rr*0.8-2,5,5,q<0.5?6:5);}
   if(q<0.25){R(tx-16,ty-6,33,1,10);R(tx,ty-22,1,33,10);for(let n=-9;n<=9;n++){R(tx+n,ty-6+n,1,1,7);R(tx+n,ty-6-n,1,1,7);}}
   if(q>=0.6)for(let n=0;n<10;n++){const a=n*0.628,rr=10+q*20;R(tx+Math.cos(a)*rr,ty-6+Math.sin(a)*rr*0.6-q*6,3,3,6);}}}
 if(cfg.loop){const cx=PX.mage+8,cy=G-14;for(let k=0;k<12;k++){const a=t*3+k*0.524;R(cx+Math.cos(a)*16,cy+Math.sin(a)*8,1,1,k%2?12:13);}
  if(((((beat%4)+4)%4)/4*bar)<0.14)for(let k=0;k<6;k++)R(0,Math.floor(hs(k+Math.floor(t*20))*H),W,1,13);}
 if(cfg.mist)for(let y=G-24;y<G-6;y++)for(let x=(y%2);x<W;x+=2)if(hs(Math.floor((x+t*12)/5)*1.3+y*0.7)<cfg.mist*0.6)R(x,y,1,1,6);
 if(cfg.rain){const n=Math.floor(cfg.rain*70);for(let i=0;i<n;i++){const y=(hs(i+300)*H+t*150)%H,x=((hs(i)*W-y*0.3)%W+W)%W;R(x,y,1,3,12);if(y>G-3)R(x-1,G-1,3,1,6);}}
 if(cfg.click&&(beat%1)<0.12){R(0,0,W,2,7);R(0,H-2,W,2,7);R(0,0,2,H,7);R(W-2,0,2,H,7);}
 c.setTransform(1,0,0,1,0,0);}
function fx(R,Y,rs,ph,mon,pos,G,buffCol){
 const w=rs.warrior;if(w&&w.sw)swoosh(R,w.sw.x,w.sw.y,8,w.sw.th,w.sw.prog,Y.arc,Y.arc2,false);
 const m=rs.mage,pm=ph.mage;if(m&&m.src&&inr(pm,0,0.9)){const tx=mon?mon.x:250,ty=mon?mon.y:m.src.ay;const hx=m.src.ax-6,hy=m.src.ay-26;if(pm<0.35){summon(R,hx,hy+Math.round(Math.sin(pm*30)),Math.min(1,pm/0.3),1);}else if(pm<0.62){const q=(pm-0.35)/0.27,e=q*q;meteor(R,hx+(tx-hx)*e,hy+(ty-hy)*e,1.6);}else if(mon)burst(R,tx,ty,(pm-0.62)/0.3,[10,9,8,7]);}
 const a=rs.archer,pa=ph.archer;if(a&&a.src&&inr(pa,0,0.5)){const tx=mon?mon.x:250,ty=mon?mon.y:a.src.ay;if(pa<0.4){arcArrow(R,a.src.ax,a.src.ay,tx,ty,pa/0.4,22,7,Y.arc2);}else if(mon)burst(R,tx,ty,(pa-0.4)/0.15*0.5,[7,Y.arc2]);}
 const cr=rs.cleric,pc=ph.cleric;if(cr&&inr(pc,0,1)){const col=buffCol!=null?buffCol:10;Object.keys(rs).forEach(k=>{const x=pos[k]+8,r=6+pc*8;for(let q=0;q<6.28;q+=0.3)if(Math.floor(q*3+pc*10)%2===0)R(x+Math.cos(q)*r,G+Math.sin(q)*r*0.3,1,1,col);
  const ay=G-30-pc*8;R(x,ay,1,4,col);R(x-1,ay+1,3,1,col);});
  if(cr.src){const ox=cr.src.ax,oy=cr.src.ay;for(let k=0;k<4;k++){const ang=k*1.57+pc*4;R(ox+Math.cos(ang)*3,oy+Math.sin(ang)*3,1,1,7);}}}}
function bossScene(R,dir,Y,M,W,H,G,t,beat,bi,bp){
 const bx=150,bc={x:172,y:G-18};
 let solid=null;if((bi===0&&bp>=0.27&&bp<0.36)||(bi===1&&[0.38,0.56,0.74].some(v=>bp>=v&&bp<v+0.05))||(bi===2&&bp>=0.44&&bp<0.5))solid=7;
 let kl=0,ksq=0;if(bi===3&&bp<0.3)kl=Math.round(Math.sin(bp/0.3*Math.PI/2)*16);else if(bi===3&&bp<0.45)ksq=0.3*(1-(bp-0.3)/0.15);else{const hh=hop(beat,3);kl=hh.lift;ksq=hh.sq;}
 const drawBoss=sd=>{slimeD(R,bx+26,G,54,36,M.king,{crown:1,lift:kl,sq:ksq,flash:sd===7,solid:sd===0?0:null});slimeD(R,224,G,14,10,M.slime,Object.assign({flash:sd===7,solid:sd===0?0:null},hop(beat+0.5,4)));};
 drawBoss(solid);
 let wx=PX.warrior,wp=null,wy=0;const front=bc.x-40;
 if(bi===0){if(bp<0.18){wx=PX.warrior+(front-PX.warrior)*(bp/0.18);wp=-0.2;}else if(bp<0.6){wx=front;wp=(bp-0.18)/0.42*0.6;}else{const q=(bp-0.6)/0.4;wx=front+(PX.warrior-front)*q;wy=-Math.round(Math.sin(q*Math.PI)*10);}
  if(bp<0.22)for(let k=0;k<10;k++){const y=G-26+Math.floor(hs(k)*24);R(PX.warrior+hs(k+3)*(wx-PX.warrior),y,14+hs(k+5)*20,1,Y.arc2);}
  if(bp<0.2)for(let k=1;k<=2;k++)figure(R,'warrior',Math.round(wx-k*10),G-24,{p:-0.2,t,solid:13,noOut:1});}
 const pm=bi===1&&bp<0.85?0.1:null,pa=bi===2&&bp<0.45?-0.2:null,pc=bi===3?Math.min(0.9,bp):null;
 const rs={};rs.cleric=figure(R,'cleric',PX.cleric,G-24,{p:pc,t});rs.archer=figure(R,'archer',PX.archer,G-24,{p:pa,t});rs.mage=figure(R,'mage',PX.mage,G-24,{p:pm,t});rs.warrior=figure(R,'warrior',Math.round(wx),G-24+wy,{p:wp,t,f:bi===0&&bp<0.18?Math.floor(t*14)%2:0});
 if(rs.warrior.sw)swoosh(R,rs.warrior.sw.x,rs.warrior.sw.y,10,rs.warrior.sw.th*1.4,rs.warrior.sw.prog,Y.arc,Y.arc2,false);
 if(bi===0&&bp>=0.27&&bp<0.5)burst(R,bc.x-8,bc.y,(bp-0.27)/0.23*0.7,[7,10,Y.arc2]);
 if(bi===1&&rs.mage.src){for(let r=0;r<8;r++){const ang=r*0.785+bp*8;R(rs.mage.src.ax+Math.cos(ang)*5,rs.mage.src.ay+Math.sin(ang)*5,1,1,r%2?10:9);}const hx=rs.mage.src.ax-6,hy=rs.mage.src.ay-24;for(let k=0;k<3;k++){const st=k*0.18,q0=(bp-st)/0.2,q=(bp-st-0.2)/0.18,tx=bc.x-8+k*8,ty=bc.y-8+k*6,sx=hx+(k-1)*12;if(q0>=0&&q0<1)summon(R,sx,hy,q0,1.3);else if(q>=0&&q<1){const e=q*q;meteor(R,sx+(tx-sx)*e,hy+(ty-hy)*e,2);}else if(q>=1&&q<1.5)burst(R,tx,ty,(q-1)*1.4,[10,9,8,7]);}}
 if(bi===2&&rs.archer.src){for(let k=0;k<6;k++){const q=(bp-0.12-k*0.06)/0.32,x1=bc.x-14+k*6,y1=bc.y-10+hs(k)*16;if(q>=0&&q<1)arcArrow(R,rs.archer.src.ax,rs.archer.src.ay,x1,y1,q,34,7,Y.arc2);else if(q>=1&&q<1.3)R(x1-1,y1-1,3,3,7);}
  if(bp<0.45&&rs.archer.src)R(rs.archer.src.ax,rs.archer.src.ay-1,2,3,Y.arc2);}
 if(bi===3){if(bp>=0.3&&bp<0.85){const q=(bp-0.3)/0.55,x=bc.x-24-q*(bc.x-24-60);R(x,G-10,3,10,Y.arc);R(x+3,G-6,3,6,Y.arc2);for(let k=0;k<4;k++)R(x+4+k*3,G-12-hs(k+Math.floor(t*20))*8,1,1,4);}
  if(bp>=0.2){const pulse=bp>=0.7&&bp<0.8;for(let a=Math.PI;a<=2*Math.PI;a+=0.04){const x=62+Math.cos(a)*60,y=G+Math.sin(a)*34;if(pulse||Math.floor(a*40)%2===0)R(x,y,1,1,pulse?7:Y.arc2);}}}
 if(bi===0&&bp>=0.2&&bp<0.27){R(-4,-4,W+8,H+8,7);drawBoss(0);figure(R,'warrior',Math.round(wx),G-24,{p:0.1,t,solid:0,noOut:1});swoosh(R,Math.round(wx)+13,G-12,10,5,1,0,0,false);}}
function drawKnob(cv){const v=Math.max(0,Math.min(1,parseFloat(cv.dataset.knob)||0)),col=cv.dataset.col||'#fff',K=KNOB[cv.dataset.dir]||KNOB['1a'];const c=cv.getContext('2d');c.clearRect(0,0,20,20);
 const cx=9.5,cy=9.5,a1=-135+v*270;
 for(let y=0;y<20;y++)for(let x=0;x<20;x++){const dx=x-cx,dy=y-cy,r=Math.sqrt(dx*dx+dy*dy),ang=Math.atan2(dx,-dy)*180/Math.PI;let f=null;
  if(K.rim&&r>=8.6&&r<9.8)f=K.rim;else if(r>=6.2&&r<8.6&&Math.abs(ang)<=135)f=ang<=a1?col:K.trk;else if(r<5.2)f=(K.capRim&&r>=4.2)?K.capRim:K.cap;
  if(f){c.fillStyle=f;c.fillRect(x,y,1,1);}}
 const ar=a1*Math.PI/180;c.fillStyle=K.ptr;for(let s=0;s<=4.6;s+=0.5)c.fillRect(Math.round(cx-0.5+Math.sin(ar)*s),Math.round(cy-0.5-Math.cos(ar)*s),1,1);}
function drawPortrait(cv){const dir=cv.dataset.dir;if(!PALS[dir])return;CURDIR=dir;const c=cv.getContext('2d');c.fillStyle=cv.dataset.bg||'#000';c.fillRect(0,0,15,15);const cls=cv.dataset.portrait;figure(mkR(c,PALS[dir]),cls,-1,cls==='mage'?6:3,{t:0});}
function drawIcon(cv){const c=cv.getContext('2d'),col=cv.dataset.col||'#fff',ty=cv.dataset.icon;c.clearRect(0,0,12,12);c.fillStyle=col;const pts=[];
 for(let x=1;x<=10;x++){let y=6;if(ty==='saw')y=Math.round(9-((x-1)%5)*1.5);else if(ty==='sine')y=Math.round(6-Math.sin((x-1)/9*Math.PI*2)*3.5);else if(ty==='square')y=x<=5?3:9;else if(ty==='tri')y=Math.round(9-Math.abs(((x-1)%6)-3)*2);else y=2+Math.floor(hs(x*7)*8);pts.push([x,y]);}
 pts.forEach((p,i)=>{c.fillRect(p[0],p[1],1,1);if(i>0){const a=pts[i-1][1],b=p[1];for(let y=Math.min(a,b);y<=Math.max(a,b);y++)c.fillRect(p[0],y,1,1);}});}

G.Scene={drawScene,drawKnob,drawPortrait,drawIcon,PALS,ORDER,PX,figure,slimeD,spr,BATA,BATB,SLIME,CL,MON,hop};
})(window);
