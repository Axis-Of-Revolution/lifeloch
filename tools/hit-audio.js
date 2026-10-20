// Deterministic 22.05 kHz counterparts of Sound::Tick and Sound::Impact.
function hitSamples(critical){let rate=22050,count=Math.floor((critical?.38:.11)*rate),samples=new Float32Array(count),noise=0x0c17ca1,low=0;
 for(let i=0;i<count;i++){let t=i/rate,v=0;noise^=noise<<13;noise^=noise>>>17;noise^=noise<<5;let hiss=((noise&65535)-32768)/32768;
  if(!critical){low+=(hiss-low)*.12;v=(.55*Math.sin(2*Math.PI*(72*t+146*(1-Math.exp(-40*t))/40))+.28*low)*Math.exp(-t*38)}
  else{low+=(hiss-low)*.15;v=.22*(hiss-low)*Math.sin(Math.PI*Math.min(1,t/.025))*Math.exp(-t*25);let u=t-.018;if(u>=0){v+=.42*hiss*Math.exp(-u*85)+.28*Math.sin(2*Math.PI*(112*u-90*u*u))*Math.exp(-u*27);v+=(.23*Math.sin(2*Math.PI*1460*u)+.18*Math.sin(2*Math.PI*2330*u)+.11*Math.sin(2*Math.PI*3470*u))*Math.exp(-u*16)*(1-Math.exp(-u*1600))}}
  samples[i]=Math.max(-.85,Math.min(.85,v*Math.min(1,t/.0015)*Math.min(1,(count-1-i)/(rate*.008))));
 }return samples}
function playHit(ctx,critical){let samples=hitSamples(critical),buffer=ctx.createBuffer(1,samples.length,22050);buffer.getChannelData(0).set(samples);let source=ctx.createBufferSource();source.buffer=buffer;source.connect(ctx.destination);source.start()}
