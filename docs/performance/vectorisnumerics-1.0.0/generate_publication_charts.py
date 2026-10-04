#!/usr/bin/env python3
"""Deterministic, offline publication charts. Only reads package CSV/JSON inputs.
Dependencies: matplotlib 3.11.2, numpy 2.5.3 (recorded source analysis versions).
No network, source-tree access, measurements or benchmark mutations.
"""
from pathlib import Path
import csv,json,hashlib,xml.etree.ElementTree as ET
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
P=Path(__file__).resolve().parent;D=P/'data';C=P/'charts';C.mkdir(exist_ok=True)
plt.rcParams.update({'font.family':'DejaVu Sans','font.size':12,'svg.fonttype':'none','svg.hashsalt':'vectoris-performance-publication-1','axes.spines.top':False,'axes.spines.right':False,'axes.axisbelow':True})
def read(n):
 with (D/n).open() as f:return list(csv.DictReader(f))
head=read('headline-results.csv');foc=read('quaternion-decomposition.csv');batch=read('rotation-amortization.csv');comp=read('compiler-results.csv');dist=read('latency-distribution.csv');verification=json.loads((D/'verification.json').read_text())
ops=['matrix_vector','matrix_matrix','quaternion_rotate','rotation_matrix_vector'];libs=['Vectoris','Eigen','GLM','Raw']
labels={'matrix_vector':'Matrix3 × Vector3','matrix_matrix':'Matrix3 × Matrix3','quaternion_rotate':'Quaternion Rotate','rotation_matrix_vector':'Cached RotationMatrix\n× Vector3'}
colors={'Vectoris':'#1675d1','Eigen':'#b85b16','GLM':'#248264','Raw':'#8768b6'}
configs=['clang-O2','clang-O3','gcc-O2','gcc-O3'];theme={}
meta={}
def lookup(rows,**keys):return next(r for r in rows if all(r[k]==str(v) for k,v in keys.items()))
def start(title,subtitle,size=(12,6),panels=None):
 if panels:fig,axs=plt.subplots(*panels,figsize=size)
 else:fig,axs=plt.subplots(figsize=size)
 fig.set_facecolor(theme['bg'])
 for ax in np.asarray(axs).flat:
  ax.set_facecolor(theme['bg']);ax.tick_params(colors=theme['fg']);ax.xaxis.label.set_color(theme['fg']);ax.yaxis.label.set_color(theme['fg']);ax.title.set_color(theme['fg'])
  for sp in ax.spines.values():sp.set_color(theme['line'])
  ax.grid(axis='y',color=theme['line'],alpha=.28)
 fig.text(.065,.953,title,color=theme['fg'],fontsize=20,fontweight='bold',va='top')
 fig.text(.065,.89,subtitle,color=theme['muted'],fontsize=11,va='top')
 return fig,axs
def save(fig,n,title,subtitle,data,filters,note,rect=(.07,.19,.98,.82)):
 fig.subplots_adjust(left=rect[0],bottom=rect[1],right=rect[2],top=rect[3])
 for ax in fig.axes:
  ax.title.set_color(theme['fg']);ax.xaxis.label.set_color(theme['fg']);ax.yaxis.label.set_color(theme['fg'])
 fig.text(.065,.035,note,color=theme['muted'],fontsize=9,va='bottom')
 suf='' if theme['name']=='light' else '-dark';target=C/(n+suf+'.svg')
 fig.savefig(target,metadata={'Date':None,'Title':title,'Description':subtitle+'; '+note},facecolor=theme['bg'])
 # Explicit accessible title/desc; labels remain editable SVG text.
 s=target.read_text();pos=s.index('>',s.index('<svg'))
 import html
 s=s[:pos]+ ' role="img" aria-labelledby="chart-title chart-desc"'+s[pos:]
 pos=s.index('>',s.index('<svg'))+1
 s=s[:pos]+f'\n<title id="chart-title">{html.escape(title)}</title><desc id="chart-desc">{html.escape(subtitle+"; "+note)}</desc>'+s[pos:]
 target.write_text(s)
 fig.savefig(C/(n+suf+'.png'),dpi=150,metadata={'Software':'Vectoris publication generator'},facecolor=theme['bg'])
 if theme['name']=='light':meta[n+'.svg']=dict(title=title,data_file='data/'+data,filters=filters,environment='macOS Intel i9-9980HK',scalar='double',source_report=('evidence/focused/VectorisNumerics_1.0.0_Quaternion_Rotation_Performance_Investigation.md' if data in ['quaternion-decomposition.csv','rotation-amortization.csv'] else 'evidence/primary/VectorisNumerics_1.0.0_Performance_Report.md'),note=note,zero_baseline=True if data!='verification.json' else 'not applicable: factual cards')
 plt.close(fig)
def bars(n,title,selected,size=(12,6.8)):
 subtitle='macOS Intel · Clang O3 · double · dependency chain · wall-time'
 fig,ax=start(title,subtitle,size);x=np.arange(len(selected));width=.19;maxv=0
 for i,lib in enumerate(libs):
  vals=[float(lookup(head,operation=op,library=lib)['median_ns']) for op in selected];maxv=max(maxv,max(vals))
  ax.bar(x+(i-1.5)*width,vals,width,color=colors[lib],label=lib)
  for xx,v in zip(x+(i-1.5)*width,vals):ax.text(xx,v+.025*max(vals),f'{v:.3f}',ha='center',va='bottom',fontsize=9,color=theme['fg'])
 ax.set_xticks(x,[labels[op].replace(' × ','\n× ') if len(selected)>3 else labels[op] for op in selected]);ax.set_ylim(0,maxv*1.24);ax.set_ylabel('Median ns/op · lower is better');ax.legend(ncol=4,loc='upper left',frameon=False,labelcolor=theme['fg'])
 save(fig,n,title,subtitle,'headline-results.csv',dict(configuration='clang-O3',operations=selected,libraries=libs,workload='dependency_chain',statistic='median',clock='wall'), '30 repetition means per scenario · includes input/observation overhead · core/clock/thermal state not locked.\nLocal workload measurements; not universal rankings or WCET.')
for theme in [dict(name='light',bg='#ffffff',fg='#172a3a',muted='#43596b',line='#718292'),dict(name='dark',bg='#101d2b',fg='#f1f5f9',muted='#c1cfdb',line='#8193a5')]:
 # 00: requested summary cards, extra to the nine technical figures.
 fig,ax=start('Benchmark at a Glance','macOS Intel · GCC / Clang · O2 / O3 · double',(12,5.4));ax.axis('off')
 cards=[('24,480','Primary benchmark\nmeasurements'),('3,840','Quaternion investigation\nmeasurements'),('6,528','Independent reference checks\nper build configuration'),('~6.42e−16','Max observed component-scaled\nrelative error\nin the tested dataset')]
 for i,(v,t) in enumerate(cards):
  xx=.02+i*.25;ax.text(xx,.64,v,color=theme['fg'],fontsize=26,fontweight='bold',transform=ax.transAxes);ax.text(xx,.22,t,color=theme['muted'],fontsize=11,linespacing=1.6,transform=ax.transAxes)
 save(fig,'00-benchmark-at-a-glance','Benchmark at a Glance','macOS Intel · GCC / Clang · O2 / O3 · double','verification.json',dict(statistic='factual counts and observed error, not comparable measures'),'816 primary and 128 focused scenarios · 30 repetitions each · no aggregate performance score.',rect=(.065,.24,.98,.77))
 bars('01-fixed-size-latency','Fixed-Size Operation Latency',ops)
 bars('02-matrix-hot-path','Fixed-Size Matrix Hot Paths',[ops[0],ops[1],ops[3]])
 title='Quaternion Rotation Cost Decomposition';sub='Focused harness · macOS Intel · Clang O3 · double · CPU-time'
 fig,ax=start(title,sub,(12,7.6));variants=['Public','Checked_reference','Minimal','Cached_matrix','Conversion_per_call'];names=['Public API','Checked reference*','Minimal unchecked','Cached RotationMatrix','Convert every call'];vals=[float(lookup(foc,variant=v)['median_ns']) for v in variants]
 y=np.arange(5);ax.barh(y,vals,color=[colors['Vectoris'],colors['Eigen'],colors['Raw'],colors['GLM'],'#7294b4']);ax.set_yticks(y,names);ax.invert_yaxis();ax.set_xlim(0,max(vals)*1.18);ax.set_xlabel('Median ns/op · lower is better');ax.grid(False);ax.grid(axis='x',color=theme['line'],alpha=.28)
 for yy,v in zip(y,vals):ax.text(v+.6,yy,f'{v:.3f}',va='center',color=theme['fg'],fontsize=12)
 save(fig,'03-quaternion-cost-decomposition',title,sub,'quaternion-decomposition.csv',dict(configuration='clang-O3',variants=variants,workload='dependency_chain',statistic='median',clock='cpu'), '*Relevant finite-input protections; full-domain semantic equivalence is not established.\nDifferences are aggregate measurements, not isolated costs of individual safety checks.',rect=(.255,.24,.96,.81))
 for norm,n,title in [(False,'04-direct-vs-cached-rotation','Direct vs Cached Rotation: Total Batch Time'),(True,'05-amortized-rotation-cost','Amortized Rotation Cost')]:
  sub='Focused harness · macOS Intel · double · CPU-time · one orientation per batch';fig,axs=start(title,sub,(12,9),panels=(2,2));ns=[1,2,4,8,16,32,64,128]
  for ax,cfg in zip(axs.flat,configs):
   for var,label,col in [('Direct','Direct quaternion rotation',colors['Vectoris']),('Convert_once','Convert once + cached matrix',colors['GLM'])]:
    rows=[lookup(batch,configuration=cfg,variant=var,vectors=n) for n in ns];vs=[float(r['median_ns'])/(n if norm else 1) for r,n in zip(rows,ns)];lo=[float(r['p10_ns'])/(n if norm else 1) for r,n in zip(rows,ns)];hi=[float(r['p90_ns'])/(n if norm else 1) for r,n in zip(rows,ns)]
    ax.plot(ns,vs,'o-',label=label,color=col,lw=2,ms=4);ax.fill_between(ns,lo,hi,color=col,alpha=.12)
   ax.set_xscale('log',base=2);ax.set_xticks(ns,[str(n) for n in ns]);ax.set_ylim(bottom=0);ax.set_title(cfg.replace('clang','Clang').replace('gcc','GCC').replace('-',' '),fontsize=13);ax.set_yticks(sorted(set([0.0]+[float(t) for t in ax.get_yticks() if 0 <= t <= ax.get_ylim()[1]])));ax.set_xlabel('Vectors rotated with the same orientation',fontsize=10);ax.set_ylabel('Median ns/vector' if norm else 'Total median ns/batch',fontsize=10);ax.tick_params(labelsize=10)
  axs[0,0].legend(fontsize=9,frameon=False,labelcolor=theme['fg'],loc='upper left' if not norm else 'upper right')
  fig.subplots_adjust(hspace=.45,wspace=.27)
  save(fig,n,title,sub,'rotation-amortization.csv',dict(configurations=configs,vectors=ns,variants=['Direct','Convert_once'],workload='batch',statistic='median with p10-p90 band',clock='cpu',conversion_included=True,normalization='divide total batch median by N' if norm else 'none'),'30 repetitions per batch · shading: p10–p90 of repetition means · conversion included inside every cached batch.\nFirst measured break-even is host/workload dependent; not an API rule or WCET.',rect=(.09,.17,.96,.80))
 title='Compiler / Optimization Comparison';sub='Primary benchmark · macOS Intel · Vectoris · double · dependency chain · wall-time';fig,axs=start(title,sub,(12,7.5),panels=(2,2))
 pal=[colors['Vectoris'],'#548cac',colors['Eigen'],colors['Raw']]
 for ax,op in zip(axs.flat,ops):
  vals=[float(lookup(comp,configuration=cfg,operation=op)['median_ns']) for cfg in configs];ax.bar(range(4),vals,color=pal);ax.set_xticks(range(4),['Clang O2','Clang O3','GCC O2','GCC O3'],fontsize=10);ax.set_title(labels[op].replace('\n',' '),fontsize=12);ax.set_ylabel('Median ns/op',fontsize=10);ax.set_ylim(0,max(vals)*1.2)
  for i,v in enumerate(vals):ax.text(i,v+.02*max(vals),f'{v:.3f}',ha='center',fontsize=9,color=theme['fg'])
 fig.subplots_adjust(hspace=.58,wspace=.25)
 save(fig,'06-compiler-optimization-comparison',title,sub,'compiler-results.csv',dict(configurations=configs,operations=ops,library='Vectoris',statistic='median',clock='wall'), 'Same host and matching flags across libraries within each configuration · no fast-math, contraction or LTO.\nO3 is not assumed universally faster than O2.',rect=(.085,.19,.97,.79))
 title='Latency Distribution';sub='Primary benchmark · macOS Intel · Clang O3 · Vectoris · double · dependency chain';fig,ax=start(title,sub,(12,6.5));sel=['quaternion_rotate','rotation_matrix_vector','matrix_vector'];x=np.arange(3)
 for i,(field,name,col) in enumerate([('median_ns','Median',colors['Vectoris']),('p90_ns','p90',colors['Eigen']),('p99_ns','p99',colors['GLM'])]):
  vals=[float(lookup(dist,operation=op)[field]) for op in sel];ax.bar(x+(i-1)*.24,vals,.24,label=name,color=col)
  for xx,v in zip(x+(i-1)*.24,vals):ax.text(xx,v+.75,f'{v:.3f}',ha='center',fontsize=9,color=theme['fg'])
 ax.set_xticks(x,[labels[op] for op in sel]);ax.set_ylim(0,max(float(r['p99_ns']) for r in dist)*1.22);ax.set_ylabel('Wall-time ns/op · lower is better');ax.legend(ncol=3,frameon=False,labelcolor=theme['fg'])
 save(fig,'07-latency-distribution',title,sub,'latency-distribution.csv',dict(configuration='clang-O3',operations=sel,library='Vectoris',statistics=['median','p90','p99'],clock='wall',percentile_method='linear interpolation over 30 repetition means'),'Observed percentiles of 30 repetition means, not individual-call tail latency.\np99 is not worst-case execution time (WCET); host state was not locked.')
 title='Correctness Alongside Speed';sub='Recorded validation evidence · macOS Intel · four build configurations';fig,ax=start(title,sub,(12,6.2));ax.axis('off')
 info=[('6,528','Independent reference-result\nchecks','per build configuration'),('79','Supported boundary checks','per build configuration'),('~6.42 × 10⁻¹⁶','Maximum observed component-scaled\nrelative error','Vectoris · tested dataset')]
 for i,(v,t,s) in enumerate(info):
  x=.02+i*.34;ax.text(x,.68,v,fontsize=25,fontweight='bold',color=theme['fg'],transform=ax.transAxes);ax.text(x,.46,t,fontsize=11,color=theme['fg'],transform=ax.transAxes);ax.text(x,.30,s,fontsize=11,color=theme['muted'],transform=ax.transAxes)
 ax.text(.02,.02,'Quaternion investigation controls: PASS · O0 · FP contraction disabled · ASan + UBSan',fontsize=12,color=theme['fg'],transform=ax.transAxes)
 save(fig,'08-correctness-evidence',title,sub,'verification.json',dict(configurations=configs,statistic='factual counts and maximum observed Vectoris component-scaled relative error'), 'Checks and error magnitude use different units: no accuracy score or normalized ranking.\nFinite sampling is not a global error bound, formal verification or independent certification.',rect=(.065,.20,.98,.78))
 bars('09-social-performance-summary','VectorisNumerics 1.0: Measured Performance',[ops[0],ops[2],ops[3]],(12,6.8))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for n,m in meta.items():
 m['derived_data_sha256']=sha(P/m['data_file'])
 m['raw_data_files']=['evidence/focused/raw.csv'] if m['data_file'] in ['data/quaternion-decomposition.csv','data/rotation-amortization.csv'] else ['evidence/primary/raw.csv']
 if m['data_file']=='data/verification.json':m['raw_data_files']=['evidence/primary/accuracy.csv','evidence/primary/raw/*/validation.jsonl','evidence/primary/raw/*/reliability.jsonl']
 m['variants']=[n,n.replace('.svg','-dark.svg')];m['rendered_preview']=n.replace('.svg','.png')
provenance=dict(publication='VectorisNumerics 1.0 Performance Publication #1',release_sha='be678b17a9ba58c9be5bca5ab59112fe74d9a83b',tag='v1.0.0',platform='macOS Intel',charts=meta,primary_statistic='wall-time',focused_statistic='CPU-time',scalar='double',charts_generator='generate_publication_charts.py',raw_crosscheck='data/verification.json',original_source_hash_snapshot='qa/source-hashes-before.json',dependencies={'matplotlib':matplotlib.__version__,'numpy':np.__version__},percentiles='linear interpolation, 30 repetition means; not per-call percentiles or WCET',network_access='none',statistical_scope='descriptive local measurements; no core/frequency/thermal lock')
(D/'provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
print('Generated',len(meta),'charts, two themes, SVG + PNG; provenance written.')
