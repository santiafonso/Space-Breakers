import soundfile as sf, numpy as np, glob, sys
def ogg(path,x,sr):
    with sf.SoundFile(path,'w',sr,2,format='OGG',subtype='VORBIS') as f:
        for i in range(0,len(x),4096): f.write(np.ascontiguousarray(x[i:i+4096]))
sr=44100; L=int(round(4*4*60/110*sr))
T={'01_pad':.03,'02_bass':.03,'03_harp':.022,'04_cello':.022,'05_piano':.02,'06_nylon':.022,'07_flute':.022,
   '08_musicbox':.012,'09_drums':.016,'10_strings':.02,'11_muted':.02,'12_drums2':.016,'13_lead':.022}
lay=[]
for f in sorted(glob.glob('soft2/[0-9][0-9]_*.wav')):
    n=f.split('/')[-1][:-4]; x,_=sf.read(f); x=x[L:2*L]; x=x*T[n]/np.sqrt((x**2).mean()); lay.append((n,x))
    ogg(f'soft2/{n}_loop.ogg',x,sr)
g=0.89/np.abs(sum(x for _,x in lay)).max()
def demo(path,start,P=2):
    out=[np.zeros((sr*2,2))]
    for k in range(start,len(lay)+1):
        for p in range(P):
            seg=np.zeros((L,2))
            for i,(n,x) in enumerate(lay):
                if i<k: seg+=x
                elif i==k and k<len(lay): seg+=x*(np.linspace(0,1,L)[:,None] if p==0 else 1)
            out.append(seg*g)
    y=np.concatenate(out); y[-sr*3:]*=np.linspace(1,0,sr*3)[:,None]; ogg(path,y,sr); return len(y)/sr
print('completo %.0fs'%demo('/home/santiafonso/demo_capas_menu.ogg',0))
print('desde la 9 %.0fs'%demo('/home/santiafonso/demo_capas_final.ogg',9))
