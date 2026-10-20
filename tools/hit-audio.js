// Deterministic 22.05 kHz counterparts of Sound::Tick and Sound::Impact.
function hitSamples(critical){let rate=22050,count=Math.floor((critical?.38:.11)*rate),samples=new Float32Array(count),noise=0x0c17ca1,low=0;
 for(let i=0;i<count;i++){let t=i/rate,v=0;noise^=noise<<13;noise^=noise>>>17;noise^=noise<<5;let hiss=((noise&65535)-32768)/32768;
  if(!critical){low+=(hiss-low)*.12;v=(.55*Math.sin(2*Math.PI*(72*t+146*(1-Math.exp(-40*t))/40))+.28*low)*Math.exp(-t*38)}
  else{low+=(hiss-low)*.15;let swish=(hiss-low)*Math.sin(Math.PI*Math.min(1,t/.021));v=.29*swish*Math.exp(-t*34);let u=t-.012;if(u>=0){let attack=Math.exp(-u*170);v+=.65*hiss*attack+.37*Math.sin(2*Math.PI*(155*u-160*u*u))*Math.exp(-u*31);v+=(.25*Math.sin(2*Math.PI*1710*u)+.18*Math.sin(2*Math.PI*2890*u)+.10*Math.sin(2*Math.PI*4100*u))*Math.exp(-u*31)*(1-Math.exp(-u*1700));let snap=u-.041;if(snap>=0)v+=.21*hiss*Math.exp(-snap*115)}}
  let fade=Math.min(1,t/.0015)*Math.min(1,(count-1-i)/(rate*.008));
  samples[i]=critical?.83*Math.tanh(v*fade*1.2):Math.max(-.85,Math.min(.85,v*fade));
 }return samples}
function playHit(ctx,critical){let samples=hitSamples(critical),buffer=ctx.createBuffer(1,samples.length,22050);buffer.getChannelData(0).set(samples);let source=ctx.createBufferSource();source.buffer=buffer;source.connect(ctx.destination);source.start()}
