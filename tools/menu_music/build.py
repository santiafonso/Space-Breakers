# Rebuild the Tension loops (C minor, 110 BPM, 4 bars) with soft GM instruments, one MIDI per layer.
import mido, re
TPB=480; S16=TPB//4; BPM=110
N={'C':0,'C#':1,'D':2,'D#':3,'E':4,'F':5,'F#':6,'G':7,'G#':8,'A':9,'A#':10,'B':11}
def m(s): a=re.match(r'([A-G]#?)(-?\d)',s); return N[a[1]]+12*(int(a[2])+1)
def seq(steps, dur=3): # steps: list of 64 note names or '.'; a note starts where it differs from previous / after a rest
    ev=[];prev=None
    for i,s in enumerate(steps):
        if s!='.' and (s!=prev): ev.append((i,m(s),dur))
        prev=s if s!='.' else None
    return ev
def bars(*rows): return ' '.join(rows).split()
saw1=bars(*['F4 F4 F4 . C4 C4 C4 . D#4 D#4 D#4 . G4 G4 G4 .']*4)
saw2=bars('. . . . . . . . . . . . C5 C5 C5 .','. . . . D#5 D#5 D#5 . . . . . . . . .','A#4 A#4 A#4 . C5 C5 C5 . . . . . . . . .','C5 C5 C5 . . . . . D#5 D#5 D#5 . C5 C5 C5 .')
saw3=bars(*['A#5 A#5 G5 G5 D#5 D#5 C5 C5']*8)
p2=bars('. . . . D#2 D#2 D#2 . F2 F2 F2 . . . . .','G2 G2 G2 . F2 F2 F2 . . . . . . . . .','A#1 A#1 A#1 . C2 C2 C2 . . . . . . . . .','D#2 D#2 D#2 D#2 F2 F2 F2 . G2 G2 G2 . A#2 A#2 A#2 .')
LAYERS={}
# 1 pad: Cm9 held per 2 bars (new)
LAYERS['01_pad']=(89,[(0,n,32) for n in (48,55,58,62,63)]+[(32,n,32) for n in (48,55,58,62,63)],52,[])
# 2 bass: soft pedal C2 on beats (original Pulse 1, thinned to quarter notes)
LAYERS['02_bass']=(33,[(i,36,4) for i in range(0,64,4)],70,[])
# 3 harp arp (Saw 1)
LAYERS['03_harp']=(46,[(a,b+12,6) for a,b,c in seq(saw1)],62,[])
# 4 cello line (Pulse 2) an octave up
LAYERS['04_cello']=(42,[(a,b+12,4) for a,b,c in seq(p2)],66,[])
# 5 piano chords (new): Cm7 / Abmaj7 / Bb / Cm on bar starts
ch=[(0,(48,51,55,58)),(16,(44,48,51,55)),(32,(46,50,53,58)),(48,(48,51,55,60))]
LAYERS['05_piano']=(0,[(t+o,n,12) for t,c in ch for o in (0,8) for n in c],48,[])
# 6 flute melody (Saw 2)
LAYERS['07_flute']=(73,[(a,b,5) for a,b,c in seq(saw2)],64,[])
# 7 music box sparkle (Saw 3), quiet
LAYERS['08_musicbox']=(10,[(a,b,2) for a,b,c in seq(saw3)],38,[])
# 8 brushes: soft kick + rim on the backbeat (from Noise 1)
LAYERS['09_drums']=(None,[],0,[(i,36,58) for i in (0,8,16,24,32,40,48,56)]+[(i,37,44) for i in (4,12,20,28,36,44,52,60)]+[(i,42,22) for i in range(2,64,4)])

CH=[(48,51,55,58),(44,48,51,55),(46,50,53,58),(48,51,55,60)]  # Cm7 Abmaj7 Bb Cm, one per bar
pat=[0,2,3,1,2,3,1,3]
LAYERS['06_nylon']=(24,[(b*16+k*2,(CH[b][pat[k]]-12 if k==0 else CH[b][pat[k]]),3) for b in range(4) for k in range(8)],70,[])
LAYERS['10_strings']=(48,[(b*16,n+12,16) for b in range(4) for n in CH[b][1:]],56,[])
acc=[1,0,0,1,0,0,1,0,1,0,1,0,0,1,0,0]
LAYERS['11_muted']=(28,[(b*16+k,n,1) for b in range(4) for k in range(16) if acc[k] for n in (CH[b][0]-12,CH[b][0]-5)],80,[])
LAYERS['12_drums2']=(None,[],0,[(i,36,64) for b in range(4) for i in (b*16+6,b*16+10)]+[(i,46,34) for i in range(2,64,4)]+[(i,70,26) for i in range(1,64,2)]+[(i,38,40) for i in (15,31,47,63)])
LAYERS['13_lead']=(27,[(a,b-12,4) for a,b,c in seq(saw2)]+[(a,b,4) for a,b,c in seq(saw2)],66,[])
REP=3
for name,(prog,notes,vel,drums) in LAYERS.items():
    mf=mido.MidiFile(ticks_per_beat=TPB); t=mido.MidiTrack(); mf.tracks.append(t)
    t.append(mido.MetaMessage('set_tempo',tempo=mido.bpm2tempo(BPM)))
    ev=[]
    chn=9 if prog is None else 0
    if prog is not None: t.append(mido.Message('program_change',program=prog,channel=0))
    t.append(mido.Message('control_change',control=91,value=90,channel=chn))  # reverb send
    t.append(mido.Message('control_change',control=93,value=30,channel=chn))  # chorus
    for r in range(REP):
        off=r*64*S16
        for a,n,d in notes: ev+= [(off+a*S16,1,n,vel),(off+(a+d)*S16-10,0,n,0)]
        for a,n,v in drums: ev+= [(off+a*S16,1,n,v),(off+a*S16+S16,0,n,0)]
    ev.append((REP*64*S16+4*TPB,0,0,0))  # tail room
    ev.sort(key=lambda e:(e[0],e[1])); now=0
    for at,on,n,v in ev:
        t.append(mido.Message('note_on' if on else 'note_off',note=n,velocity=v,channel=chn,time=at-now)); now=at
    mf.save(f'soft2/{name}.mid')
print('ok')
