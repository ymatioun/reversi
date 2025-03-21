import random, sys, gc, warnings, math, os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from time import time
warnings.filterwarnings('ignore')

t0 = time()



# read part 1
df = pd.read_csv('sigmas_odd_19.csv', header=None)
df.columns = ['n','sc','s0','s1','s3','s5','s7','s9','s11','s13','s15','s17','s19']
for col in df.columns:
    df[col] = df[col].astype('int32')
print('loaded data1', df.shape, int(time()-t0), 'sec')

# read part 2
df1 = pd.read_csv('sigmas_even_18.csv', header=None)
df1.columns = ['n','sc','s0','s2','s4','s6','s8','s10','s12','s14','s16','s18']
df1.drop(['n','sc','s0'], axis=1, inplace=True)
for col in df1.columns:
    df1[col] = df1[col].astype('int32')
# combine
df = pd.concat([df, df1], axis=1)

del df1
gc.collect()
print('loaded data2', df.shape, int(time()-t0), 'sec')





# drop <16
df = df.loc[df['n'] >= 16]






    
# define differences
fr = [2,3,4,5,6,7,8,9,10,11,12,13,14,15,15,15,16,16,16,17,17,17,18,18,18,19,19,19,20,20,20]
to = [0,1,2,1,2,3,4,3, 4, 3, 4, 5, 4, 3, 5, 7, 4, 6, 8, 3, 5, 7, 4, 6, 8, 3, 5, 7, 4, 6, 8]
for i in range(len(fr)):
    if 's'+str(fr[i]) in df.columns:
        df['d_'+str(to[i])+'_'+str(fr[i])] = df['s'+str(fr[i])] - df['s'+str(to[i])]





# vs perfect score
for i in range(25):
    if 's'+str(i) in df.columns:
        df['e_'+str(i)] = df['s'+str(i)] - df['sc']
        print(i, np.round((df['e_'+str(i)]**2).mean(), 2))





# test - regress on things ...
#df = df.loc[df['n']==52] # select 1 value of n - for now

##from scipy.optimize import minimize
##def obj(x):
##    s = x[0] + df['mob1'] * x[1] + df['mob2'] * x[2] + df['pmob1'] * x[3] + df['s0'] * x[4] # sigma
##    # s2 vs s0: sigma = 4.51
##    l = np.log(s).mean() + ((df['s2'] - df['s0'])**2 / 2 / s / s).mean()
##    return l * 1000000 # mult to increase precision: 2.007 -> 1.991
##
##x = [6.5, -0.29, -0.17, 0.11, 0.016]
##print('start opt', int(time()-t0), 'sec')
##res = minimize(obj, x, options={'maxiter':10000})
##print('end opt', int(time()-t0), 'sec')
##print(res)
##stop

##for v in ['s0', 'mob1', 'mob2', 'pmob1', 'pmob2']:
##    df2 = df.groupby(v)[['d10']].agg(['std','size'])
##    df2.columns = ['std','size']
##    df2 = df2.loc[df2['size']>1000]
##    print(v)
##    print(df2)
##    plt.plot(df2['std'])
##    plt.show()
##stop







drops = ['sc','s0','s1','s2','s3','s4','s5','s6','s7','s8','s9','s10','s11','s12','s13','s14','s15','s16','s17','s18','s19','s20','mob1','mob2','pmob1','pmob2']
drops = list(set(df.columns).intersection(set(drops)))
df.drop(drops, axis=1, inplace=True)
df2 = df.groupby('n').std()
df2 = np.round(df2 * 20, 0)
print('done', df2.shape, int(time()-t0), 'sec')

cols = [c for c in df.columns if c[0] == 'd']
print(df2[cols])
cols = [c for c in df.columns if c[0] == 'e']
print(df2[cols])

plt.plot(df2)
plt.show()
df2 = df2.fillna(0) # need this to avoid data shifts!
df2.to_csv('df2.csv')
