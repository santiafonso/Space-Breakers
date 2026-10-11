# Per-16th-note pitch estimate of a (mostly mono) synth loop: harmonic product spectrum.
import soundfile as sf, numpy as np, sys
NAMES='C C# D D# E F F# G G# A A# B'.split()
def nm(m): return NAMES[m%12]+str(m//12-1)
for f in sys.argv[1:]:
    x,sr=sf.read(f); x=x.mean(1)
    step=len(x)/64  # 4 bars of 16ths
    out=[]
    for k in range(64):
        a=x[int(k*step):int((k+1)*step)]
        rms=np.sqrt((a**2).mean())
        if rms<0.01: out.append('.'); continue
        w=a*np.hanning(len(a)); n=1<<16
        sp=np.abs(np.fft.rfft(w,n)); fr=np.fft.rfftfreq(n,1/sr)
        h=sp.copy()
        for r in (2,3,4):
            d=sp[::r]; h[:len(d)]*=d
        lo=np.searchsorted(fr,50); hi=np.searchsorted(fr,2000)
        f0=fr[lo+np.argmax(h[lo:hi])]
        out.append(nm(int(round(69+12*np.log2(f0/440)))))
    print(f.split('/')[-1]); 
    for b in range(4): print('  ',' '.join('%-4s'%o for o in out[b*16:(b+1)*16]))
