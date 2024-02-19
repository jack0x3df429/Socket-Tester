import matplotlib.pyplot as plt
import numpy as np, operator as op
from itertools import islice
from scipy.interpolate import make_interp_spline
from scipy.signal import savgol_filter
import seaborn as sns,pandas as pd,sys
from importlib import reload
flierprops = dict(marker='o', markersize=0.5,
              linestyle='none')


files=["sec","brt","dly"];
def add_median_labels(ax, fmt='.1f'):
    lines = ax.get_lines()
    boxes = [c for c in ax.get_children() if type(c).__name__ == 'PathPatch']
    lines_per_box = int(len(lines) / len(boxes))
    for median in lines[4:len(lines):lines_per_box]:
        x, y = (data.mean() for data in median.get_data())
        # choose value depending on horizontal or vertical plot orientation
        value = x if (median.get_xdata()[1] - median.get_xdata()[0]) == 0 else y
        text = ax.text(x, y, f'{value:{fmt}}', ha='center', va='center',
                       fontweight='bold', color='white')
        # create median-colored border around white text for contrast
        text.set_path_effects([
            path_effects.Stroke(linewidth=3, foreground=median.get_color()),
            path_effects.Normal(),
        ])
def edge(data):
    return np.max(data),np.min(data)
'''def read_big_f(f,p):
  print(f+str(p))
  with open(f+str(p)) as pf:
      count=0
      d=np.array([])
      while True:
          next_n_line = np.array(list(islice(pf,int(1e8))))
          d=np.append(d,next_n_line.astype(float))
          if len(next_n_line) ==0:
              break
          count+=len(next_n_line)
          print(count,end='\r',flush=True)
      #d=np.delete(d,np.s_[:100000])
      dmean=np.mean(d)
      percentile=np.percentile(d, (1.25, 50, 98.75), method='midpoint')
      if not f.endswith("sec"):
        print(p,"%.3f\t\t%.3f\t\t%.3f\t\t%.3f"%(dmean,percentile[0],percentile[1],percentile[2]))
      return d,dmean,percentile'''
def sort_d(d,sec,limit=None,scale=1): 
      r={}
      for k,v in d.items():
          if len(sec[k])<=0:
            r[k]=np.array([])
            continue
          slen=int(sec[k][-1]//scale)
          r[k]=np.zeros(slen)
          for s in range(slen):
            #print(slen,d[k],sec[k][-1])
            r[k][s]=np.mean(d[k][sec[k]//scale==s])
      return r
def limit_proc(dly,brt,sec,limit=None,dlypercentile=None,brtpercentile=None,scale=1000000000):
  for i in sec.keys():
      keep=np.logical_and(dly[i]<dlypercentile[i][2],brt[i]<brtpercentile[i][2])
      keep=np.logical_and(keep, np.logical_and(brt[i]>0,dly[i]>0))
      keep=np.logical_and(keep,sec[i]//scale>limit[0])
      keep=np.logical_and(keep,sec[i]//scale<limit[1])
      #print(sec[i].shape,keep.shape)
      sec[i]=sec[i][keep]
      dly[i]=dly[i][keep]
      brt[i]=brt[i][keep]
  return dly,brt,sec
def read_file(f,p):
    d = np.loadtxt(f+str(p), dtype=np.int64).astype(np.int64)
    dmean=np.mean(d)
    percentile=np.percentile(d, (0, 50, 99), method='midpoint')
    #if not f.endswith("sec"):
    #    print(p,"\t\t%f\t\t%d\t\t%d\t\t%d"%(dmean,percentile[0],percentile[1],percentile[2]))
    return d,dmean,percentile
def data_proc(dfile,protocol='t'):
    limit=[000,500]
    dly,dlymean,dlypercentile,dlysdata={},{},{},{}
    brt,brtmean,brtpercentile,brtsdata={},{},{},{}
    sec={}
    for i in priority:
        dly[i],dlymean[i],dlypercentile[i]  =read_file('./%s/%s_dly'%(dfile,protocol) ,i)
        brt[i],brtmean[i],brtpercentile[i]  =read_file('./%s/%s_brt'%(dfile,protocol) ,i)
        sec[i]                              =read_file('./%s/%s_sec'%(dfile,protocol) ,i)[0]
        if i=='6':
            sec[i]+=100000000000
        elif i=='u6':
            sec[i]+=300000000000
        elif i=='u5' or i=='u4' or i=='u3':
            sec[i]+=200000000000
    #dly,brt,sec=limit_proc(dly,brt,sec,limit,dlypercentile=dlypercentile,brtpercentile=brtpercentile)
    printinfo(dly,brt)
    dlysdata=sort_d(dly,sec,scale=1000000000)
    brtsdata=sort_d(brt,sec,scale=1000000000)
    return dly,dlysdata,brt,brtsdata
def printinfo(dly,brt):
    for p in priority:
        if len(dly[p])<=0: continue
        print(p,"\t\t%f\t\t%d\t\t%d"%(np.mean(dly[p]),np.max(dly[p]),np.min(dly[p])))
        print(p,"\t\t%f\t\t%d\t\t%d"%(np.mean(brt[p]),np.max(brt[p]),np.min(brt[p])))



files=['test_mix'];                            priority=[3]
#files=['test_mix'];                            priority=[6,'u6',5,'u5',4,'u4',3,'u3']
priority=[str(p) for p in priority]
dly,dlysdata,brt,brtsdata=[],[],[],[]
for dfile in files:
    d=data_proc(dfile)
    dly.append(d[0])
    dlysdata.append(d[1])
    brt.append(d[2])
    brtsdata.append(  d[3])

import plot as p
p.secplot(dlysdata)
p.beplot(dly,priority)
p.secplot(brtsdata,ylabel='Bitrates(MBps)')
p.beplot(brt,priority,ylabel='Bitrates(MBps)')