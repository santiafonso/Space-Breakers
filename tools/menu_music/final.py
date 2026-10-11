import soundfile as sf, numpy as np, glob
exec(open('mix2.py').read().split("def demo")[0].replace("ogg(f'soft2/{n}_loop.ogg',x,sr)","pass"))
full=sum(x for _,x in lay)*g
print('full mix rms %.1f dBFS, peak %.2f'%(20*np.log10(np.sqrt((full**2).mean())),np.abs(full).max()))
for n,x in lay: ogg(f'final/{n}.ogg',x*g,sr)
