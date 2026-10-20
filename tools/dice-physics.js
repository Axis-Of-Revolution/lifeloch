// One convex rigid body against the tray floor and four walls.
// Results are read from the resting orientation, never supplied to the solver.
const PHYSICS_DT=1/240,ROLL_MS=3000;
const Vadd=(a,b)=>a.map((x,i)=>x+b[i]),Vmul=(a,k)=>a.map(x=>x*k),Vsub=(a,b)=>Vadd(a,Vmul(b,-1)),Vdot=(a,b)=>a.reduce((s,x,i)=>s+x*b[i],0),Vcross=(a,b)=>[a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]],Vunit=a=>Vmul(a,1/Math.max(.00001,Math.sqrt(Vdot(a,a))));
const Qnorm=q=>{let n=Math.sqrt(Vdot(q,q));return q.map(x=>x/n)},Qproduct=(a,b)=>[a[0]*b[0]-a[1]*b[1]-a[2]*b[2]-a[3]*b[3],a[0]*b[1]+a[1]*b[0]+a[2]*b[3]-a[3]*b[2],a[0]*b[2]-a[1]*b[3]+a[2]*b[0]+a[3]*b[1],a[0]*b[3]+a[1]*b[2]-a[2]*b[1]+a[3]*b[0]],Qaxis=(a,t)=>[Math.cos(t/2),...Vmul(Vunit(a),Math.sin(t/2))],Qrotate=(q,v)=>{let n=q.slice(1),t=Vmul(Vcross(n,v),2);return Vadd(v,Vadd(Vmul(t,q[0]),Vcross(n,t)))};
const trayPlanes=[{n:[0,0,1],d:0},{n:[.70710678,0,.70710678],d:4.94974747},{n:[-.70710678,0,.70710678],d:4.94974747},{n:[0,.70710678,.70710678],d:2.82842712},{n:[0,-.70710678,.70710678],d:2.82842712}];
class DiceBody{
 constructor(sides=20,seed=1){this.reset(sides,seed)}
 reset(sides,seed){this.sides=sides;this.mesh=diceMeshes[sides];let rng=seed||1,rand=()=>{rng^=rng<<13;rng^=rng>>>17;rng^=rng<<5;return (rng>>>0)/4294967296};
  this.p=[-2.8+rand()*1.8,(rand()-.5)*2.4,2.1+rand()*.5];this.v=[3+rand()*3,(rand()-.5)*3,3+rand()*2];
  let u=rand(),v=rand()*Math.PI*2,w=rand()*Math.PI*2;this.q=[Math.sqrt(u)*Math.cos(w),Math.sqrt(1-u)*Math.sin(v),Math.sqrt(1-u)*Math.cos(v),Math.sqrt(u)*Math.sin(w)];
  this.w=[(rand()-.5)*28,(rand()-.5)*28,(rand()-.5)*20];if(sides===2)this.w=[15+rand()*12,(rand()-.5)*8,(rand()-.5)*7];
  this.trayYaw=0;this.elapsed=0;this.motionAge=0;this.interactive=false;this.sleepTilt=[0,0];this.quiet=0;this.hits=0;this.lastHit=-1;this.settled=false;this.result=0;this.accumulator=0;this.tilt=[0,0];
  // Unit mass. Regular solids use isotropic inertia; the coin is a solid cylinder.
  this.inertia=sides===2?[.25,.25,.5]:[.27,.27,.27];
 }
 invI(v){let local=Qrotate([this.q[0],-this.q[1],-this.q[2],-this.q[3]],v);return Qrotate(this.q,local.map((n,i)=>n/this.inertia[i]))}
 impulse(j,r){this.v=Vadd(this.v,j);this.w=Vadd(this.w,this.invI(Vcross(r,j)))}
 velocity(r){return Vadd(this.v,Vcross(this.w,r))}
 effective(r,n){return 1+Vdot(n,Vcross(this.invI(Vcross(r,n)),r))}
 faceUp(){if(this.sides===4){let best=-Infinity,value=1;this.mesh.vertices.forEach((v,i)=>{let z=Qrotate(this.q,v)[2];if(z>best){best=z;value=i+1}});return value}let best=-Infinity,value=0;for(let f of this.mesh.faces)if(f.label){let z=Qrotate(this.q,f.normal)[2];if(z>best){best=z;value=f.label}}return value}
 step(tx=0,ty=0,yawRate=0){if(this.settled)return;let dt=PHYSICS_DT;this.elapsed+=dt;this.motionAge+=dt;let limit=this.interactive?.85:.16;this.tilt[0]+=(Math.max(-limit,Math.min(limit,tx))-this.tilt[0])*.08;this.tilt[1]+=(Math.max(-limit,Math.min(limit,ty))-this.tilt[1])*.08;let previousYaw=this.trayYaw;this.trayYaw+=(yawRate/1.7-this.trayYaw)*.08;let alpha=(this.trayYaw-previousYaw)/dt,omega=this.trayYaw,tilted=this.interactive&&(Vdot(this.tilt,this.tilt)>.0064||Math.abs(omega)>.035);
  this.v=Vadd(this.v,Vmul([2*omega*this.v[1]+alpha*this.p[1]+omega*omega*this.p[0],-2*omega*this.v[0]-alpha*this.p[0]+omega*omega*this.p[1],0],dt));this.w[2]-=alpha*dt;
  this.v=Vadd(this.v,Vmul([this.tilt[0]*28,this.tilt[1]*28,-28],dt));this.v=Vmul(this.v,Math.exp(-.18*dt));this.w=Vmul(this.w,Math.exp(-.25*dt));
  this.p=Vadd(this.p,Vmul(this.v,dt));let dq=Qproduct([0,...this.w],this.q);this.q=Qnorm(this.q.map((v,i)=>v+dq[i]*dt*.5));
  let contacts=[],floorContact=false;
  for(let vertex of this.mesh.vertices){let r=Qrotate(this.q,vertex),world=Vadd(this.p,r);for(let plane of trayPlanes){let distance=Vdot(world,plane.n)+plane.d;if(distance>.007)continue;let vn=Vdot(this.velocity(r),plane.n);if(vn<-1.4&&this.elapsed-this.lastHit>.045){this.hits++;this.lastHit=this.elapsed}let restitution=this.motionAge<1.3?.32:.08,target=(vn<-1?-vn*restitution:0)+Math.min(5,Math.max(0,-distance-.002)*.17/dt);contacts.push({r,n:plane.n,target,mu:plane.n[2]===1?(tilted?.24:this.motionAge>1.2?.65:.30):.01,j:0,t:[0,0,0]});if(plane.n[2]===1)floorContact=true}}
  for(let iteration=0;iteration<12;iteration++)for(let c of contacts){let vn=Vdot(this.velocity(c.r),c.n),j=Math.max(0,c.j+(c.target-vn)/this.effective(c.r,c.n)),dj=j-c.j;c.j=j;this.impulse(Vmul(c.n,dj),c.r);let velocity=this.velocity(c.r),tangent=Vsub(velocity,Vmul(c.n,Vdot(velocity,c.n))),speed=Math.sqrt(Vdot(tangent,tangent));if(speed<.00001)continue;let direction=Vmul(tangent,1/speed),delta=Vmul(direction,-speed/this.effective(c.r,direction)),candidate=Vadd(c.t,delta),max=c.j*c.mu,length=Math.sqrt(Vdot(candidate,candidate));if(length>max)candidate=Vmul(candidate,max/length);this.impulse(Vsub(candidate,c.t),c.r);c.t=candidate}
  // Dissipate spin through rolling resistance on felt; never align to a chosen face.
  if(floorContact){let late=tilted?0:Math.max(0,Math.min(1,(this.motionAge-1.8)/.8)),damping=tilted?1.2:(this.motionAge>1.35?2.6:.75)+late*9;this.w=Vmul(this.w,Math.exp(-damping*dt));this.v=Vmul(this.v,Math.exp(-((tilted?.8:2)+late*36)*dt))}
  let flat=this.mesh.faces.some(f=>(this.sides!==2||f.label)&&Qrotate(this.q,f.normal)[2]<-.9985);
  if(floorContact&&Vdot(this.v,this.v)<.04&&Vdot(this.w,this.w)<.035)this.quiet+=dt;else this.quiet=0;
  if(this.quiet>.10&&this.motionAge>.65&&Math.abs(this.trayYaw)<.035){this.settled=true;this.sleepTilt=[tx,ty];this.result=flat?this.faceUp():0;this.v=[0,0,0];this.w=[0,0,0]}
 }
 advance(seconds,tx=0,ty=0,yawRate=0){yawRate=Number.isFinite(yawRate)?Math.max(-4,Math.min(4,yawRate)):0;if(Math.abs(yawRate)<.04)yawRate=0;if(Math.hypot(tx,ty)>.20||Math.abs(yawRate)>.08)this.interactive=true;if(this.settled){if(Math.hypot(tx-this.sleepTilt[0],ty-this.sleepTilt[1])<.045&&Math.abs(yawRate)<.08)return this;this.settled=false;this.interactive=true;this.motionAge=this.quiet=this.accumulator=0;this.result=0}this.accumulator+=Math.max(0,Math.min(seconds,5))*1.7;let steps=Math.min(2040,Math.floor((this.accumulator+1e-9)/PHYSICS_DT));for(let i=0;i<steps&&!this.settled;i++)this.step(tx,ty,yawRate);this.accumulator-=steps*PHYSICS_DT;return this}
}
