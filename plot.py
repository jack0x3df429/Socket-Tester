import matplotlib.pyplot as plt
import numpy as np, operator as op
from itertools import islice
from scipy.interpolate import make_interp_spline
from scipy.signal import savgol_filter
import seaborn as sns,pandas as pd,sys

flierprops = dict(marker='o', markersize=0.5,
              linestyle='none')


files=["sec","brt","dly"];
def customized_box_plot(percentiles, axes, redraw = True, *args, **kwargs):
    """
    Generates a customized boxplot based on the given percentile values
    """
    
    box_plot = axes.boxplot([[-9, -4, 2, 4, 9],]*n_box, *args, **kwargs) 
    # Creates len(percentiles) no of box plots
    
    min_y, max_y = float('inf'), -float('inf')
    
    for box_no, (q1_start, 
                 q2_start,
                 q3_start,
                 q4_start,
                 q4_end,
                 fliers_xy) in enumerate(percentiles):
        
        # Lower cap
        box_plot['caps'][2*box_no].set_ydata([q1_start, q1_start])
        # xdata is determined by the width of the box plot

        # Lower whiskers
        box_plot['whiskers'][2*box_no].set_ydata([q1_start, q2_start])

        # Higher cap
        box_plot['caps'][2*box_no + 1].set_ydata([q4_end, q4_end])

        # Higher whiskers
        box_plot['whiskers'][2*box_no + 1].set_ydata([q4_start, q4_end])

        # Box
        box_plot['boxes'][box_no].set_ydata([q2_start, 
                                             q2_start, 
                                             q4_start,
                                             q4_start,
                                             q2_start])
        
        # Median
        box_plot['medians'][box_no].set_ydata([q3_start, q3_start])

        # Outliers
        if fliers_xy is not None and len(fliers_xy[0]) != 0:
            # If outliers exist
            box_plot['fliers'][box_no].set(xdata = fliers_xy[0],
                                           ydata = fliers_xy[1])
            
            min_y = min(q1_start, min_y, fliers_xy[1].min())
            max_y = max(q4_end, max_y, fliers_xy[1].max())
            
        else:
            min_y = min(q1_start, min_y)
            max_y = max(q4_end, max_y)
                    
        # The y axis is rescaled to fit the new box plot completely with 10% 
        # of the maximum value at both ends
        axes.set_ylim([min_y*1.1, max_y*1.1])

    # If redraw is set to true, the canvas is updated.
    if redraw:
        ax.figure.canvas.draw()
        
    return box_plot
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
def read_big_f(f,p):
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
      percentile=np.percentile(d, (5, 50, 95), method='midpoint')
      print(p,"%.3f\t%.3f\t%.3f\t%.3f"%(dmean,percentile[0],percentile[1],percentile[2]))
      return d,dmean,percentile
def sort_d(d,sec,limit=None,scale=1): 
      r={}
      for k,v in d.items():
          slen=int(sec[k][-1]//scale)
          r[k]=[0]*slen
          print(k)
          for s in range(slen):
            r[k][s]=np.mean(d[k][sec[k]//scale==s])
      return r
def limit_proc(data,sec,limit=[None,None],dmean=None,percentile=None):
  for i in data.keys():
    if limit[1] is not None:
      sec[i]=sec[i][data[i]<limit[1]]
      data[i]=data[i][data[i]<limit[1]]
    elif dmean is not None:
      sec[i]=sec[i][data[i]<dmean[i]*5]
      data[i]=data[i][data[i]<dmean[i]*5]
    elif percentile is not None:
      sec[i]=sec[i][data[i]<percentile[i][2]]
      data[i]=data[i][data[i]<percentile[i][2]]
    if limit[0] is not None:
      sec[i]=sec[i][data[i]>limit[0]]
      data[i]=data[i][data[i]>limit[0]]
      
  return data,sec
def sec_limit_proc(data,sec,limit):
  for i in data.keys():
    if limit[1] is not None:
      
      data[i]=data[i][sec[i]<limit[1]]
      sec[i]=sec[i][sec[i]<limit[1]]
    if limit[0] is not None:
      
      data[i]=data[i][sec[i]>limit[0]]
      sec[i]=sec[i][sec[i]>limit[0]]
  return data,sec
def data_proc(dfile):
    data,dmean,percentile,sec,sdata={},{},{},{},{}
    for i in priority:
        data[i],dmean[i],percentile[i]  =read_big_f('./%s/t_%s'%(dfile,sys.argv[1]) ,i)
        sec[i]                          =read_big_f('./%s/t_sec'%dfile              ,i)[0]

    if sys.argv[1]=='brt':
        data,sec=limit_proc(data,sec,dmean=dmean)
    else:
        data,sec=limit_proc(data,sec,percentile=percentile)
    data,sec=sec_limit_proc(data,sec,limit)
    sdata=sort_d(data,sec,scale=1000000000)
    return data,dmean,percentile,sec,sdata
#sns.displot(data)
#sns.kdeplot(data)


##files=['test345','test345_be']
##data,sec,dmean,percentile,sdata=[],[],[],[],[]
##limit=[5000000000,300000000000]
'''for idx,dfile in enumerate(files):
    d=data_proc(dfile)
    data.append(        d[0])
    dmean.append(       d[1])
    percentile.append(  d[2])
    sec.append(         d[3])
    sdata.append(       d[4])'''


#######################################
def secplot(sdata,ylabel='Latency(ns)'):
    colors=[{   '6':"#0072BD",  '5':"#D95319",  '4':"#EDB120",  '3':"#7E2F8E","BE":"#7F7F7F",
                'u6':"#77AC30", 'u5':"#4DBEEE", 'u4':"#4297AF", 'u3':"#FF00FF"  },
        {'6':"#77AC30",'5':"#4DBEEE",'4':"#A2142F",'3':"#FF00FF"},
        {'6':"#FF0000",'5':"#00FF00",'4':"#0000FF",'3':"#00FFFF"}
        ]
    plot_arg={0:["dyn"],1:["be"],2:["no-dyn"]}
    for idx,value in enumerate(sdata):
        for i in value.keys():
            #print(idx,i,value[i],colors[idx][i])
            plt.plot(value[i],color=colors[idx][str(i)],label="%s"%(i))#,alpha=arg[0])
    plt.legend(loc='center left', bbox_to_anchor=(0.95, 0.5),
           fancybox=True, shadow=True, ncol=1)
    plt.ticklabel_format(axis='y',style='plain')
    plt.xlabel('Time(n)', fontsize=10)
    plt.ylabel(ylabel, fontsize=10)
    plt.show()
    plt.close()


#######################################
def boxplot(data,priority):
    colors_pl=['r','g','b','y']
    fig, ax = plt.subplots()
    print("Box")
    bps={}
    c={}
    plot_arg=[0,1,2]
    names=['Dyn','BE',"Non-Dyn"]
    for idx in range(len(data)):
      bps[idx]=ax.boxplot([data[idx][p] for p in priority],
                    positions=idx+np.arange(0,len(priority)*len(plot_arg),len(plot_arg)),
                    notch=True,
                    widths=0.35, 
                    patch_artist=True,
                    boxprops=dict(facecolor=colors_pl[idx]),
                    showfliers=False)#,showmeans=True)#,showmeans=True)
    ax.set_xticks(len(plot_arg)*np.arange(len(priority))+0.5)
    ax.set_xticklabels(priority)
    ax.ticklabel_format(axis='y',style='plain')
    ax.legend([bp["boxes"][0] for bp in bps.values()],names[:len(data)],loc='upper center', bbox_to_anchor=(0.5, 1.0),
           fancybox=True, shadow=True, ncol=len(plot_arg))
    ax.set_xlim(-1,len(priority)*len(plot_arg))
    plt.show()

#######################################
def beplot(data,priority,ylabel='Latency(ns)'):
    data=data[0]
    colors_pl=['r','g','b','y']
    colors=[    {'6':"#0072BD",'5':"#D95319",'4':"#EDB120",'3':"#7E2F8E","BE":"#7F7F7F",
                 'u6':"#77AC30",'u5':"#4DBEEE",'u4':"#A2142F",'u3':"#FF00FF"},
                {'6':"#77AC30",'5':"#4DBEEE",'4':"#A2142F",'3':"#FF00FF"},
                {'6':"#FF0000",'5':"#00FF00",'4':"#0000FF",'3':"#00FFFF"}
        ]
    fig, ax = plt.subplots()
    print("Box")
    bps={}
    c={}
    pos=0
    plot_arg=[0,1,2]
    names=['Dyn','BE',"Non-Dyn"]
    for p in priority:
        bps[p]=ax.boxplot([data[p]],
        positions=[pos],
        notch=True,
        widths=0.35, 
        patch_artist=True,
        boxprops=dict(facecolor=colors[0][p]),
        showfliers=False)#,showmeans=True)#,showmeans=True)'''
        pos+=1
    ax.set_xticklabels(priority)
    '''ax.legend([bp["boxes"][0] for bp in bps.values()],priority,loc='upper center', bbox_to_anchor=(0.5, 1.1),
           fancybox=True, shadow=True, ncol=len(plot_arg))'''
    plt.ylabel(ylabel, fontsize=10)
    ax.yaxis.set_tick_params(labelsize=10)
    ax.xaxis.set_tick_params(labelsize=15)
    ax.ticklabel_format(axis='y',style='plain')
    #ax.ticklabel_format(axis='y',useOffset=False)
    ax.set_xlim(-1,len(priority))
    plt.show()
'''
7 25000
6 180000
5 45000
7 25000
6 180000
4 22500
0 1 2 3 4 5 6 7 22500
'''
