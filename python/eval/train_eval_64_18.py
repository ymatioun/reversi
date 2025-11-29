import random, sys, gc, warnings, math, os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from time import time
from numba import njit
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2' # 2 = INFO and WARNING messages are not printed
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import Layer
from tensorflow.keras.callbacks import Callback, LearningRateScheduler, EarlyStopping
from tensorflow.keras.optimizers.schedules import ExponentialDecay, CosineDecayRestarts
from tensorflow.keras.layers import Input, Dense, Concatenate, Flatten, Dropout, Embedding, Reshape
from tensorflow.keras.utils import Sequence
from tensorflow.keras.initializers import LecunNormal, RandomNormal, RandomUniform
t0 = time()


### summarize eval
##df = pd.read_csv("../temp/pos_eval.csv")
##df = df.loc[(df['n']<=60) & (df['n']>=12)]
##s = np.round(((df['ev']-df['sc'])**2).mean(), 1)
##print(s) # 66.8
##stop



ver = 1 # global verbose switch ************************

for lc1 in range(-1, 5): # 5
 for lc2 in [0,1,2,3,8,9,10,11,18,19]: # 4, 4*5+1=21, 5 min each -> 1 h 45 min
    x    = np.load('posb.npy') # 1.3 Gb
    mob  = np.load('mob.npy')
    y    = np.load('sc.npy')

    # test - select subset of data ***********************************************
    #s = x.shape[0] // 4
    #x = x[:s,:]
    #mob = mob[:s,:]
    #y = y[:s]
    
    # drop records with n>60 or n<12
    n = np.abs(x).sum(-1).astype(np.uint8)
    idx = (n<=60) & (n>=12)
    x = x[idx,:]
    mob = mob[idx,:]
    y = y[idx]
    del idx
    gc.collect()

    n = np.abs(x).sum(-1).astype(np.uint8)
    if ver: print('read data', x.shape, n.min(), n.max(), int(time()-t0), 'sec')


    min_count = 50 # 50 # drop patterns with count below this. Here 5 vs 50 does not reduce the error. 250 vs 50 makes error worse.

    def tr1(s, d):# transpose* one square 8 ways
        x, y = s%8, s//8
        if d == 0:
            return x + y * 8
        if d == 1:
            return 7 - x + y * 8
        if d == 2:
            return x + (7 - y) * 8
        if d == 3:
            return 7 - x + (7 - y) * 8
        if d == 4:
            return y + x * 8
        if d == 5:
            return 7 - y + x * 8
        if d == 6:
            return y + (7 - x) * 8
        if d == 7:
            return 7 - y + (7 - x) * 8
        else:
            stop

    def tr(x, d): # transpose list
        return [tr1(i,d) for i in x]


    # calc corner10b patterns - side symm, 4 way: 0/1, 2/3, 4/6, 5/7
    patt = [0,1,2,9,10] # 26.0 - simple changes do not improve this.
     # exchange element indexed by lc1[0-9] for lc2[0-...]
    if (lc1 >=0 and lc2 in patt) or (lc1 < 0 and lc2 > 0): # skip if already present
        continue
    if lc1 >= 0:
        patt[lc1] = lc2
    for i in range(5): # add right 5 (0->1)
        patt.append(tr1(patt[i], 1))
    patt.sort()
    tmp = np.zeros([x.shape[0], 4], dtype=np.int32)
    mult = np.array(3**np.arange(len(patt)), dtype=np.int32).reshape(1,-1)
    tmp[:,0] = np.minimum((x[:,tr(patt, 0)] * mult).sum(-1), (x[:,tr(patt, 1)] * mult).sum(-1)) # 0/1
    tmp[:,1] = np.minimum((x[:,tr(patt, 2)] * mult).sum(-1), (x[:,tr(patt, 3)] * mult).sum(-1)) # 2/3
    tmp[:,2] = np.minimum((x[:,tr(patt, 4)] * mult).sum(-1), (x[:,tr(patt, 6)] * mult).sum(-1)) # 4/6
    tmp[:,3] = np.minimum((x[:,tr(patt, 5)] * mult).sum(-1), (x[:,tr(patt, 7)] * mult).sum(-1)) # 5/7
    # drop patterns with count < X (assign them to empty patterns)
    a = pd.Series(tmp.ravel()).value_counts() # count
    a = a.index * (a>=min_count) # zero out small ones
    di = a.to_dict()
    tmp = pd.Series(tmp.ravel()).map(di).to_numpy().reshape(tmp.shape)
    # replace
    s = pd.Series(tmp.ravel())
    l_c10a = list(s.unique())
    l_c10a.sort()
    di = {}
    for i, l1 in enumerate(l_c10a):
        di[l1] = i
    c10a = np.array(s.map(di)).reshape(-1,4).astype(np.int32)
    patt10a = patt.copy() # save
    if ver: print('c10a', c10a.max(), int(time()-t0), 'sec')




    

##    # calc corner10a patterns - no symm, 8 way
##    #np.random.seed(int(1000000000*(time()%1))) # truly random start - need this to run parallel games
##    #patt = list(np.random.choice(np.arange(28)[1:], size=9, replace=False)) # random 10 bits; limit to 28 ***************
##    #patt.append(0) # always include 0
##
##    #patt = [0,2,4,6,9,10,12,13,14,19] # 10 best, for 26.4
##    patt = [0,2,4,5,6,9,10,12,13,14] # 10 best, for 26.2
##    # exchange element indexed by lc1[0-9] for lc2[0-23]
##    if lc2 in patt: # skip if already present
##        continue
##    patt[lc1] = lc2
##    
##    patt.sort()
##    tmp = np.zeros([x.shape[0], 8], dtype=np.int32)
##    mult = np.array(3**np.arange(len(patt)), dtype=np.int32).reshape(1,-1)
##    for i in range(8):
##        tmp[:,i] = (x[:,tr(patt, i)] * mult).sum(-1)
##    # drop patterns with count < X (assign them to empty patterns)
##    a = pd.Series(tmp.ravel()).value_counts() # count
##    a = a.index * (a>=min_count) # zero out small ones
##    di = a.to_dict()
##    tmp = pd.Series(tmp.ravel()).map(di).to_numpy().reshape(tmp.shape)
##    # replace
##    s = pd.Series(tmp.ravel())
##    l_c10a = list(s.unique())
##    l_c10a.sort()
##    di = {}
##    for i, l1 in enumerate(l_c10a):
##        di[l1] = i
##    c10a = np.array(s.map(di)).reshape(-1,8).astype(np.int32)
##    patt10a = patt.copy() # save
##    if ver: print('c10a', c10a.max(), int(time()-t0), 'sec')


    

    # calc corner10 patterns
    patt = [0,1,2,3,4,8,9,10,11,12]
    tmp = np.zeros([x.shape[0], 8], dtype=np.int32)
    mult = np.array(3**np.arange(len(patt)), dtype=np.int32).reshape(1,-1)
    for i in range(8):
        tmp[:,i] = (x[:,tr(patt, i)] * mult).sum(-1)
    # drop patterns with count < X (assign them to empty patterns)
    a = pd.Series(tmp.ravel()).value_counts() # count
    a = a.index * (a>=min_count) # zero out small ones
    di = a.to_dict()
    tmp = pd.Series(tmp.ravel()).map(di).to_numpy().reshape(tmp.shape)
    # replace
    s = pd.Series(tmp.ravel())
    l_c8 = list(s.unique())
    l_c8.sort()
    di = {}
    for i, l1 in enumerate(l_c8):
        di[l1] = i
    c8 = np.array(s.map(di)).reshape(-1,8).astype(np.int32)
    if ver: print('c8', c8.max(), int(time()-t0), 'sec')


    # calc corner9 patterns
    patt = [0,1,2,8,9,10,16,17,18]
    tmp = np.zeros([x.shape[0], 4], dtype=np.int32)
    mult = np.array(3**np.arange(len(patt)), dtype=np.int32).reshape(1,-1)
    for i in range(4):
        tmp[:,i] = np.minimum((x[:,tr(patt, i)] * mult).sum(-1), (x[:,tr(patt, i + 4)] * mult).sum(-1))
    # drop patterns with count < X (assign them to empty patterns)
    a = pd.Series(tmp.ravel()).value_counts() # count
    a = a.index * (a>=min_count) # zero out small ones
    di = a.to_dict()
    cor9 = pd.Series(tmp.ravel()).map(di).to_numpy().reshape(tmp.shape)
    # replace
    s = pd.Series(cor9.ravel())
    l_c9 = list(s.unique())
    l_c9.sort()
    di = {}
    for i, l1 in enumerate(l_c9):
        di[l1] = i
    cor9 = np.array(s.map(di)).reshape(-1,4).astype(np.int16)
    del tmp
    gc.collect()
    if ver: print('cor9', cor9.max(), int(time()-t0), 'sec')


    # calc side8a patterns
    mult = np.array(3**np.arange(8), dtype=np.int16).reshape(1,-1)

    tmp = (x[:,[0,1,2,3,4,5,6,7]] * mult).sum(-1) # top
    co2 = (x[:,[7,6,5,4,3,2,1,0]] * mult).sum(-1)
    tmp = np.minimum(tmp, co2)

    co1 = (x[:,[0,8,16,24,32,40,48,56]] * mult).sum(-1) # left
    co2 = (x[:,[56,48,40,32,24,16,8,0]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp.reshape(-1,1), co1.reshape(-1,1), axis=1)

    co1 = (x[:,[7,15,23,31,39,47,55,63]] * mult).sum(-1) # right
    co2 = (x[:,[63,55,47,39,31,23,15,7]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

    co1 = (x[:,[56,57,58,59,60,61,62,63]] * mult).sum(-1) # bottom
    co2 = (x[:,[63,62,61,60,59,58,57,56]] * mult).sum(-1)

    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

    # drop patterns with count < X (assign them to empty patterns)
    a = pd.Series(tmp.ravel()).value_counts() # count
    a = a.index * (a>=min_count) # zero out small ones
    di = a.to_dict()
    tmp = pd.Series(tmp.ravel()).map(di).to_numpy().reshape(tmp.shape)

    # replace
    s = pd.Series(tmp.ravel())
    l_s8a = list(s.unique())
    l_s8a.sort()
    di = {}
    for i, l1 in enumerate(l_s8a):
        di[l1] = i
    s8a = np.array(s.map(di)).reshape(-1,4).astype(np.int16)
    del tmp
    gc.collect()
    if ver: print('s8a', s8a.max(), int(time()-t0), 'sec')



    # calc side8b patterns
    mult = np.array(3**np.arange(8), dtype=np.int16).reshape(1,-1)

    tmp = (x[:,[8,9,10,11,12,13,14,15]] * mult).sum(-1)
    co2 = (x[:,[15,14,13,12,11,10,9,8]] * mult).sum(-1)
    tmp = np.minimum(tmp, co2)

    co1 = (x[:,[48,49,50,51,52,53,54,55]] * mult).sum(-1)
    co2 = (x[:,[55,54,53,52,51,50,49,48]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp.reshape(-1,1), co1.reshape(-1,1), axis=1)

    co1 = (x[:,[1,9,17,25,33,41,49,57]] * mult).sum(-1)
    co2 = (x[:,[57,49,41,33,25,17,9,1]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

    co1 = (x[:,[6,14,22,30,38,46,54,62]] * mult).sum(-1)
    co2 = (x[:,[62,54,46,38,30,22,14,6]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

    # drop patterns with count < X (assign them to empty patterns)
    a = pd.Series(tmp.ravel()).value_counts() # count
    a = a.index * (a>=min_count) # zero out small ones
    di = a.to_dict()
    tmp = pd.Series(tmp.ravel()).map(di).to_numpy().reshape(tmp.shape)

    # replace
    s = pd.Series(tmp.ravel())
    l_s8b = list(s.unique())
    l_s8b.sort()
    di = {}
    for i, l1 in enumerate(l_s8b):
        di[l1] = i
    s8b = np.array(s.map(di)).reshape(-1,4).astype(np.int16)
    del tmp
    gc.collect()
    if ver: print('s8b', s8b.max(), int(time()-t0), 'sec')



    # calc side8c patterns
    mult = np.array(3**np.arange(8), dtype=np.int16).reshape(1,-1)

    tmp = (x[:,[16,17,18,19,20,21,22,23]] * mult).sum(-1)
    co2 = (x[:,[23,22,21,20,19,18,17,16]] * mult).sum(-1)
    tmp = np.minimum(tmp, co2)

    co1 = (x[:,[40,41,42,43,44,45,46,47]] * mult).sum(-1)
    co2 = (x[:,[47,46,45,44,43,42,41,40]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp.reshape(-1,1), co1.reshape(-1,1), axis=1)

    co1 = (x[:,[2,10,18,26,34,42,50,58]] * mult).sum(-1)
    co2 = (x[:,[58,50,42,34,26,18,10,2]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

    co1 = (x[:,[5,13,21,29,37,45,53,61]] * mult).sum(-1)
    co2 = (x[:,[61,53,45,37,29,21,13,5]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

    # drop patterns with count < X (assign them to empty patterns)
    a = pd.Series(tmp.ravel()).value_counts() # count
    a = a.index * (a>=min_count) # zero out small ones
    di = a.to_dict()
    tmp = pd.Series(tmp.ravel()).map(di).to_numpy().reshape(tmp.shape)

    # replace
    s = pd.Series(tmp.ravel())
    l_s8c = list(s.unique())
    l_s8c.sort()
    di = {}
    for i, l1 in enumerate(l_s8c):
        di[l1] = i
    s8c = np.array(s.map(di)).reshape(-1,4).astype(np.int16)
    del tmp
    gc.collect()
    if ver: print('s8c', s8c.max(), int(time()-t0), 'sec')



    # calc side8d patterns
    mult = np.array(3**np.arange(8), dtype=np.int16).reshape(1,-1)

    tmp = (x[:,[24,25,26,27,28,29,30,31]] * mult).sum(-1)
    co2 = (x[:,[31,30,29,28,27,26,25,24]] * mult).sum(-1)
    tmp = np.minimum(tmp, co2)

    co1 = (x[:,[32,33,34,35,36,37,38,39]] * mult).sum(-1)
    co2 = (x[:,[39,38,37,36,35,34,33,32]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp.reshape(-1,1), co1.reshape(-1,1), axis=1)

    co1 = (x[:,[3,11,19,27,35,43,51,59]] * mult).sum(-1)
    co2 = (x[:,[59,51,43,35,27,19,11,3]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

    co1 = (x[:,[4,12,20,28,36,44,52,60]] * mult).sum(-1)
    co2 = (x[:,[60,52,44,36,28,20,12,4]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

    # drop patterns with count < X (assign them to empty patterns)
    a = pd.Series(tmp.ravel()).value_counts() # count
    a = a.index * (a>=min_count) # zero out small ones
    di = a.to_dict()
    tmp = pd.Series(tmp.ravel()).map(di).to_numpy().reshape(tmp.shape)

    # replace
    s = pd.Series(tmp.ravel())
    l_s8d = list(s.unique())
    l_s8d.sort()
    di = {}
    for i, l1 in enumerate(l_s8d):
        di[l1] = i
    s8d = np.array(s.map(di)).reshape(-1,4).astype(np.int16)
    del tmp
    gc.collect()
    if ver: print('s8d', s8d.max(), int(time()-t0), 'sec')



    # calc d7 patterns
    mult = np.array(3**np.arange(7), dtype=np.int16).reshape(1,-1)

    tmp = (x[:,[8,17,26,35,44,53,62]] * mult).sum(-1)
    co2 = (x[:,[62,53,44,35,26,17,8]] * mult).sum(-1)
    tmp = np.minimum(tmp, co2)

    co1 = (x[:,[1,10,19,28,37,46,55]] * mult).sum(-1)
    co2 = (x[:,[55,46,37,28,19,10,1]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp.reshape(-1,1), co1.reshape(-1,1), axis=1)

    co1 = (x[:,[6,13,20,27,34,41,48]] * mult).sum(-1)
    co2 = (x[:,[48,41,34,27,20,13,6]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

    co1 = (x[:,[15,22,29,36,43,50,57]] * mult).sum(-1)
    co2 = (x[:,[57,50,43,36,29,22,15]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

    # drop patterns with count < X (assign them to empty patterns)
    a = pd.Series(tmp.ravel()).value_counts() # count
    a = a.index * (a>=min_count) # zero out small ones
    di = a.to_dict()
    tmp = pd.Series(tmp.ravel()).map(di).to_numpy().reshape(tmp.shape)

    # replace
    s = pd.Series(tmp.ravel())
    l_d7 = list(s.unique())
    l_d7.sort()
    di = {}
    for i, l1 in enumerate(l_d7):
        di[l1] = i
    d7 = np.array(s.map(di)).reshape(-1,4).astype(np.int16)
    del tmp
    gc.collect()
    if ver: print('d7', d7.max(), int(time()-t0), 'sec')




    # calc d6 patterns
    mult = np.array(3**np.arange(6), dtype=np.int16).reshape(1,-1)

    tmp = (x[:,[16,25,34,43,52,61]] * mult).sum(-1)
    co2 = (x[:,[61,52,43,34,25,16]] * mult).sum(-1)
    tmp = np.minimum(tmp, co2)

    co1 = (x[:,[2,11,20,29,38,47]] * mult).sum(-1)
    co2 = (x[:,[47,38,29,20,11,2]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp.reshape(-1,1), co1.reshape(-1,1), axis=1)

    co1 = (x[:,[5,12,19,26,33,40]] * mult).sum(-1)
    co2 = (x[:,[40,33,26,19,12,5]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

    co1 = (x[:,[23,30,37,44,51,58]] * mult).sum(-1)
    co2 = (x[:,[58,51,44,37,30,23]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

    # drop patterns with count < X (assign them to empty patterns)
    a = pd.Series(tmp.ravel()).value_counts() # count
    a = a.index * (a>=min_count) # zero out small ones
    di = a.to_dict()
    tmp = pd.Series(tmp.ravel()).map(di).to_numpy().reshape(tmp.shape)

    # replace
    s = pd.Series(tmp.ravel())
    l_d6 = list(s.unique())
    l_d6.sort()
    di = {}
    for i, l1 in enumerate(l_d6):
        di[l1] = i
    d6 = np.array(s.map(di)).reshape(-1,4).astype(np.int16)
    del tmp
    gc.collect()
    if ver: print('d6', d6.max(), int(time()-t0), 'sec')



    # calc d5 patterns
    mult = np.array(3**np.arange(5), dtype=np.int16).reshape(1,-1)

    tmp = (x[:,[3,12,21,30,39]] * mult).sum(-1)
    co2 = (x[:,[39,30,21,12,3]] * mult).sum(-1)
    tmp = np.minimum(tmp, co2)

    co1 = (x[:,[4,11,18,25,32]] * mult).sum(-1)
    co2 = (x[:,[32,25,18,11,4]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp.reshape(-1,1), co1.reshape(-1,1), axis=1)

    co1 = (x[:,[24,33,42,51,60]] * mult).sum(-1)
    co2 = (x[:,[60,51,42,33,24]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

    co1 = (x[:,[31,38,45,52,59]] * mult).sum(-1)
    co2 = (x[:,[59,52,45,38,31]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

    # drop patterns with count < X (assign them to empty patterns)
    a = pd.Series(tmp.ravel()).value_counts() # count
    a = a.index * (a>=min_count) # zero out small ones
    di = a.to_dict()
    tmp = pd.Series(tmp.ravel()).map(di).to_numpy().reshape(tmp.shape)

    # replace
    s = pd.Series(tmp.ravel())
    l_d5 = list(s.unique())
    l_d5.sort()
    di = {}
    for i, l1 in enumerate(l_d5):
        di[l1] = i
    d5 = np.array(s.map(di)).reshape(-1,4).astype(np.int16)
    del tmp
    gc.collect()
    if ver: print('d5', d5.max(), int(time()-t0), 'sec')



    # calc d4 patterns
    mult = np.array(3**np.arange(4), dtype=np.int16).reshape(1,-1)

    tmp = (x[:,[3,10,17,24]] * mult).sum(-1)
    co2 = (x[:,[24,17,10,3]] * mult).sum(-1)
    tmp = np.minimum(tmp, co2)

    co1 = (x[:,[4,13,22,31]] * mult).sum(-1)
    co2 = (x[:,[31,22,13,4]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp.reshape(-1,1), co1.reshape(-1,1), axis=1)

    co1 = (x[:,[32,41,50,59]] * mult).sum(-1)
    co2 = (x[:,[59,50,41,32]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

    co1 = (x[:,[39,46,53,60]] * mult).sum(-1)
    co2 = (x[:,[60,53,46,39]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

    # drop patterns with count < X (assign them to empty patterns)
    a = pd.Series(tmp.ravel()).value_counts() # count
    a = a.index * (a>=min_count) # zero out small ones
    di = a.to_dict()
    tmp = pd.Series(tmp.ravel()).map(di).to_numpy().reshape(tmp.shape)

    # replace
    s = pd.Series(tmp.ravel())
    l_d4 = list(s.unique())
    l_d4.sort()
    di = {}
    for i, l1 in enumerate(l_d4):
        di[l1] = i
    d4 = np.array(s.map(di)).reshape(-1,4).astype(np.int16)
    del tmp
    gc.collect()
    if ver: print('d4', d4.max(), int(time()-t0), 'sec')



    # calc d8 patterns
    mult = np.array(3**np.arange(8), dtype=np.int16).reshape(1,-1)

    tmp = (x[:,[0,9,18,27,36,45,54,63]] * mult).sum(-1)
    co2 = (x[:,[63,54,45,36,27,18,9,0]] * mult).sum(-1)
    tmp = np.minimum(tmp, co2)

    co1 = (x[:,[7,14,21,28,35,42,49,56]] * mult).sum(-1)
    co2 = (x[:,[56,49,42,35,28,21,14,7]] * mult).sum(-1)
    co1 = np.minimum(co1, co2)
    tmp = np.append(tmp.reshape(-1,1), co1.reshape(-1,1), axis=1)

    # drop patterns with count < X (assign them to empty patterns)
    a = pd.Series(tmp.ravel()).value_counts() # count
    a = a.index * (a>=min_count) # zero out small ones
    di = a.to_dict()
    tmp = pd.Series(tmp.ravel()).map(di).to_numpy().reshape(tmp.shape)

    # replace
    s = pd.Series(tmp.ravel())
    l_d8 = list(s.unique())
    l_d8.sort()
    di = {}
    for i, l1 in enumerate(l_d8):
        di[l1] = i
    d8 = np.array(s.map(di)).reshape(-1,2).astype(np.int16)
    del tmp, co2, di, co1
    gc.collect()
    if ver: print('d8', d8.max(), int(time()-t0), 'sec')


    # cap mobs to have at least 100 samples
    mob[:,0] = np.minimum(mob[:,0], 23)
    mob[:,1] = np.minimum(mob[:,1], 23)

    
    #mob = mob.astype(np.int16) + (n-12).astype(np.int16).reshape(-1,1) * 24 # mob[24] x n[49]
    #d4 = d4 * 49 + (n-12).astype(np.int16).reshape(-1,1) # 45,49

    # c10a: 2 adjacent sides at a time
##    c10a = c10a * 0 # remove c10a
##    s8a = s8a.astype(np.int32)
##    ss = 1 + s8a.max()
##    mult = np.array(3**np.arange(8), dtype=np.int16).reshape(1,-1)
##
##    c10a[:,0] = ss * (x[:,[7,6,5,4,3,2,1,0]] * mult).sum(-1) + (x[:,[0,8,16,24,32,40,48,56]] * mult).sum(-1) # top+left
##    c10a[:,1] = ss * (x[:,[0,1,2,3,4,5,6,7]] * mult).sum(-1) + (x[:,[7,15,23,31,39,47,55,63]] * mult).sum(-1) # top+right
##    c10a[:,2] = ss * (x[:,[63,62,61,60,59,58,57,56]] * mult).sum(-1) + (x[:,[56,48,40,32,24,16,8,0]] * mult).sum(-1) # bottom+left
##    c10a[:,3] = ss * (x[:,[56,57,58,59,60,61,62,63]] * mult).sum(-1) + (x[:,[63,55,47,39,31,23,15,7]] * mult).sum(-1) # bottom+right
##
##    # drop patterns with count < X (assign them to empty patterns)
##    a = pd.Series(c10a.ravel()).value_counts() # count
##    a = a.index * (a>=min_count) # zero out small ones
##    di = a.to_dict()
##    tmp = pd.Series(c10a.ravel()).map(di).to_numpy().reshape(c10a.shape)
##    # replace
##    s = pd.Series(tmp.ravel())
##    l_c10a = list(s.unique())
##    l_c10a.sort()
##    di = {}
##    for i, l1 in enumerate(l_c10a):
##        di[l1] = i
##    c10a = np.array(s.map(di)).reshape(-1,4).astype(np.int32)
##    patt10a = patt.copy() # save
##    if ver: print('c10a', c10a.max(), int(time()-t0), 'sec')

    


    E1 = 64 # ****************************************************************************
    x = (n-n.min()).astype(np.int16).reshape(-1,1) # 1

    class data_gen2(Sequence): # data generator - for unshuffled splits in 2.
        def __init__(self, i1, i2, batch_size): # constructor: save all data locally
            self.i1, self.i2, self.batch_size = i1, i2, batch_size
            super().__init__(max_queue_size=10000)
            return

        def __len__(self): # returns number of batches
            return math.ceil((self.i2 - self.i1) / self.batch_size)

        def __getitem__(self, b): # returns one batch
            i1a = self.i1 + self.batch_size * b
            i2a = min(x.shape[0], i1a + self.batch_size)
            return ((x[i1a:i2a,:], mob[i1a:i2a,:], cor9[i1a:i2a,:], c8[i1a:i2a,:], s8a[i1a:i2a,:], s8b[i1a:i2a,:], s8c[i1a:i2a,:], s8d[i1a:i2a,:], d7[i1a:i2a,:], d6[i1a:i2a,:], d5[i1a:i2a,:], d4[i1a:i2a,:], d8[i1a:i2a,:], c10a[i1a:i2a,:]), y[i1a:i2a])


    # model
    NUMB    = 1024*4     # number of samples in a batch. Tr data is 16 Mil positions.
    lr0     = 5e-3       # starting learning rate
    L2      = 7e-5       # L2, incr to improve val score

    val_p = 0.2         # proportion of data to be used for validation
    i_tr = int(x.shape[0] * (1-val_p))
    yp = y[i_tr:]

    tf.random.set_seed(13)
    np.random.seed(13)
    with tf.device('/GPU:0'):
        class MyLayer(Layer):
            def call(self, x):
                x = tf.math.reduce_sum(x, axis=-1)
                return tf.math.maximum(0.0, x)

        class MySum(Layer):
            def call(self, x):
                return tf.math.reduce_sum(x, axis=-2, keepdims=True)
            
        class MySum2(Layer):
            def call(self, x):
                return tf.math.reduce_sum(x[:,:E1//2], axis=-1) - tf.math.reduce_sum(x[:,E1//2:], axis=-1) # add/sub

        class Tr23(Layer):
            def call(self, x):
                return tf.transpose(x, [0, 2, 1])

        class Resh(Layer):
            def call(self, x):
                return tf.reshape(x, [-1, E1, 1])

        class Parity(Layer):
            def __init__(self):
                super().__init__()
                self.w = self.add_weight(shape=(1,), initializer='zeros', trainable=True)
            def call(self, x):
                return tf.math.reduce_sum(x * self.w, axis=-1)

        class print_lr(Callback):
            def on_epoch_end(self, epoch, logs=None):
                print('epoch', epoch + 1, 'lr', np.round(np.array(self.model.optimizer.learning_rate), 6), ' ', end='')

        # N; init to 1 - to give them the same sign and stability
        i1 = Input(shape=(x.shape[1],), dtype='int16') # N: 1
        d1 = 1 + Embedding(input_dim=x.max()+1, output_dim=E1, mask_zero=False, name='Nm', embeddings_initializer=tf.keras.initializers.Zeros(), embeddings_regularizer=tf.keras.regularizers.L2(4*L2))(i1) # 1xE1
        d1 = Tr23()(d1) # E1x1

        # mobs, encoded separately
        i2 = Input(shape=(mob.shape[1],), dtype='int16')
        d2a = Flatten()(Embedding(input_dim=mob[:,0].max()+1, output_dim=E1, mask_zero=False, name='mob1', embeddings_regularizer=tf.keras.regularizers.L2(L2))(i2[:,0])) # E1
        d2b = Flatten()(Embedding(input_dim=mob[:,1].max()+1, output_dim=E1, mask_zero=False, name='mob2', embeddings_regularizer=tf.keras.regularizers.L2(L2))(i2[:,1])) # E1
        d3a = d1 * Resh()(d2a) # E1x1
        d3b = d1 * Resh()(d2b) # E1x1
        
        i3  = Input(shape=(cor9.shape[1],), dtype='int16') # cor9
        d2c = Embedding(input_dim=cor9.max()+1, output_dim=E1, mask_zero=False, name='c9', embeddings_regularizer=tf.keras.regularizers.L2(L2))(i3) # 4xE1
        d3c = d1 * Tr23()(MySum()(d2c)) # E1x1

        i3a  = Input(shape=(c8.shape[1],), dtype='int32') # cor8[10]
        d2c8 = Embedding(input_dim=c8.max()+1, output_dim=E1, mask_zero=False, name='c8', embeddings_regularizer=tf.keras.regularizers.L2(L2))(i3a) # 8xE1
        d3c8 = d1 * Tr23()(MySum()(d2c8)) # E1x1
        
        i4  = Input(shape=(s8a.shape[1],), dtype='int16') # s8a
        d2d = Embedding(input_dim=s8a.max()+1, output_dim=E1, mask_zero=False, name='s8a', embeddings_regularizer=tf.keras.regularizers.L2(L2))(i4) # 4xE1
        d3d = d1 * Tr23()(MySum()(d2d)) # E1x1

        i5  = Input(shape=(s8b.shape[1],), dtype='int16') # s8b
        d2e = Embedding(input_dim=s8b.max()+1, output_dim=E1, mask_zero=False, name='s8b', embeddings_regularizer=tf.keras.regularizers.L2(L2))(i5) # 4xE1
        d3e = d1 * Tr23()(MySum()(d2e)) # E1x1

        i6  = Input(shape=(s8c.shape[1],), dtype='int16') # s8c
        d2f = Embedding(input_dim=s8c.max()+1, output_dim=E1, mask_zero=False, name='s8c', embeddings_regularizer=tf.keras.regularizers.L2(L2))(i6) # 4xE1
        d3f = d1 * Tr23()(MySum()(d2f)) # E1x1

        i7  = Input(shape=(s8d.shape[1],), dtype='int16') # s8d
        d2g = Embedding(input_dim=s8d.max()+1, output_dim=E1, mask_zero=False, name='s8d', embeddings_regularizer=tf.keras.regularizers.L2(L2))(i7) # 4xE1
        d3g = d1 * Tr23()(MySum()(d2g)) # E1x1

        i8  = Input(shape=(d7.shape[1],), dtype='int16') # d7
        d2h = Embedding(input_dim=d7.max()+1, output_dim=E1, mask_zero=False, name='d7', embeddings_regularizer=tf.keras.regularizers.L2(L2))(i8) # 4xE1
        d3h = d1 * Tr23()(MySum()(d2h)) # E1x1

        i9  = Input(shape=(d6.shape[1],), dtype='int16') # d6
        d2i = Embedding(input_dim=d6.max()+1, output_dim=E1, mask_zero=False, name='d6', embeddings_regularizer=tf.keras.regularizers.L2(L2))(i9) # 4xE1
        d3i = d1 * Tr23()(MySum()(d2i)) # E1x1

        i10 = Input(shape=(d5.shape[1],), dtype='int16') # d5
        d2j = Embedding(input_dim=d5.max()+1, output_dim=E1, mask_zero=False, name='d5', embeddings_regularizer=tf.keras.regularizers.L2(L2))(i10) # 4xE1
        d3j = d1 * Tr23()(MySum()(d2j)) # E1x1

        i11 = Input(shape=(d4.shape[1],), dtype='int16') # d4
        d2k = Embedding(input_dim=d4.max()+1, output_dim=E1, mask_zero=False, name='d4', embeddings_regularizer=tf.keras.regularizers.L2(L2))(i11) # 4xE1
        d3k = d1 * Tr23()(MySum()(d2k)) # E1x1

        i12 = Input(shape=(d8.shape[1],), dtype='int16') # d8
        d2l = Embedding(input_dim=d8.max()+1, output_dim=E1, mask_zero=False, name='d8', embeddings_regularizer=tf.keras.regularizers.L2(L2))(i12) # 4xE1
        d3l = d1 * Tr23()(MySum()(d2l)) # E1x1

        i13 = Input(shape=(c10a.shape[1],), dtype='int16') # c10a
        d2m = Embedding(input_dim=c10a.max()+1, output_dim=E1, mask_zero=False, name='c10a', embeddings_regularizer=tf.keras.regularizers.L2(L2))(i13) # 8xE1
        d3m = d1 * Tr23()(MySum()(d2m)) # E1x1

        # This order defines the order of coeffs: c9, s8abcd, d76548, m1, m2, c8 
        d3 = Concatenate()([d3c, d3d, d3e, d3f, d3g, d3h, d3i, d3j, d3k, d3l, d3a, d3b, d3c8, d3m]) # E1xYYY
        
        d4a = MyLayer()(d3) # E1
        o = MySum2()(d4a) # +-
        
        model = tf.keras.Model(inputs=(i1, i2, i3, i3a, i4, i5, i6, i7, i8, i9, i10, i11, i12, i13), outputs=o)

        STEPS_PER_EPOCH = (x.shape[0] * (1 - val_p)) // NUMB 
        lr_schedule = CosineDecayRestarts(initial_learning_rate=lr0, first_decay_steps=6)
        model.compile(optimizer=keras.optimizers.Adam(lr_schedule), loss='MSE')
        if ver: print(model.summary())
        es = EarlyStopping(monitor='val_loss', start_from_epoch=1, patience=250, verbose=1*ver, mode='min', restore_best_weights=True)

        # fit model epochs=210 *****
        tr = data_gen2(0, i_tr, NUMB) # train: first part
        va = data_gen2(i_tr, x.shape[0], NUMB) # valid: second part
        model.fit(x=tr, epochs=210, validation_data=va, verbose=2*ver, callbacks=[es]) # print_lr()
        pred = model.predict((x[i_tr:,:], mob[i_tr:,:], cor9[i_tr:,:], c8[i_tr:,:], s8a[i_tr:,:], s8b[i_tr:,:], s8c[i_tr:,:], s8d[i_tr:,:], d7[i_tr:,:], d6[i_tr:,:], d5[i_tr:,:], d4[i_tr:,:], d8[i_tr:,:], c10a[i_tr:,:]), batch_size=NUMB//2, verbose=0).ravel()
##        pred = np.zeros(x.shape[0]-i_tr, dtype=np.float32)
##        N = 1024*4*4
##        if ver: print('predict', int(time()-t0), 'sec')
##        for i in range(pred.shape[0]//N + 1):
##            i1 = i_tr + i * N
##            i2 = min(x.shape[0], i1 + N)
##            i1a = i * N
##            i2a = min(pred.shape[0], i1a + N)
##            pred[i1a:i2a] = model.predict((x[i1:i2,:], mob[i1:i2,:], cor9[i1:i2,:], c8[i1:i2,:], s8a[i1:i2,:], s8b[i1:i2,:], s8c[i1:i2,:], s8d[i1:i2,:], d7[i1:i2,:], d6[i1:i2,:], d5[i1:i2,:], d4[i1:i2,:], d8[i1:i2,:]), verbose=0).ravel()
        s = np.round(((pred-yp)**2).mean(), 1)
    print(lc1, lc2, patt10a, s,  int(time()-t0), 'sec')
    break
 break

# save
w = model.weights
#l_c10a
np.savetxt('l_c10a.txt', np.array(l_c10a), fmt='%i') # 17173
#l_c8
np.savetxt('l_c8.txt', np.array(l_c8), fmt='%i') # 29181 for c10 (need to add 1 to these values) 
#l_c9
np.savetxt('l_c9.txt', np.array(l_c9), fmt='%i') # 5153 4552
#l_s8a
np.savetxt('l_s8a.txt', np.array(l_s8a), fmt='%i') # 3004 3003
#l_s8b
np.savetxt('l_s8b.txt', np.array(l_s8b), fmt='%i') # 3320 3320
#l_s8c
np.savetxt('l_s8c.txt', np.array(l_s8c), fmt='%i') # 3320 3129
#l_s8d
np.savetxt('l_s8d.txt', np.array(l_s8d), fmt='%i') # 1484 1356
#l_d7
np.savetxt('l_d7.txt', np.array(l_d7), fmt='%i') # 728 660
#l_d6
np.savetxt('l_d6.txt', np.array(l_d6), fmt='%i') # 377 374
#l_d5
np.savetxt('l_d5.txt', np.array(l_d5), fmt='%i') # 134 134
#l_d4
np.savetxt('l_d4.txt', np.array(l_d4), fmt='%i') # 44 44
#l_d8
np.savetxt('l_d8.txt', np.array(l_d8), fmt='%i') # 1404 1244

files = ['c_c9.txt', 'c_s8a.txt', 'c_s8b.txt', 'c_s8c.txt', 'c_s8d.txt', 'c_d7.txt', 'c_d6.txt', 'c_d5.txt', 'c_d4.txt', 'c_d8.txt', 'c_mob1.txt', 'c_mob2.txt', 'c_c8.txt', 'c_c10a.txt']
mm = 0
for cc in range(len(files)):
    vv = np.round(w[cc+1].numpy() * 128, 0).astype(np.int32).ravel()
    np.savetxt(files[cc], vv, fmt='%i')
    print(vv.shape[0]//E1, files[cc], vv.min(), vv.max())
    mm = max(mm, np.abs(vv).max())
print('range', mm, 'margin', np.round(2**15 / mm, 1)) # range 434 margin 75.5

vv = 1 + np.round(w[0].numpy(), 5) # (49, 64) 3.1
np.savetxt('c_n.txt', vv, fmt='%.5f')
print('c_n', vv.shape, np.round(np.abs(vv).max(), 1))

#****************************************************************************************************************************************
# baseline                                  28.5, 236 sec
# c10b, [0, 1, 2, 3, 4, 5, 6, 7, 13, 14]    27.7 [-0.8]
# E1=64, L2x2                               27.1 [52]
# L2x2.5                                    27.0 [52]
# 1.5e-5                                    26.9 [52] = v14 - keep increasing L2? to do ...
# 2.5e-5                                    26.6 [52] - keep increasing

# 5e-5                                      26.6 [105]
# incr L2 on N (x4)                         26.5 [209] = v15 - ELO=+185, accept
# 7e-5                                      26.5 [105] - not better
# L2(n)x16                                      26.8 - undo
# add parity to score(1)                    26.5 [105] - not better; parity score = 2.1. Lower Nm variability and range - keep
# L2(n)x8                                       26.7 - undo
# rerun                                     26.6 [105] = v16 ELO=+185 - a wash
# add Na                                    26.5 - not better. parity=-1.92 = v17. ELO=+183
# no Na, parity. Diff Nm for each           ...


# remove N mult                                 29.3 (+2.7) - significant impact!
# add parity(64x2), before N mult               26.6 - not better
# drop d4                                       26.8 (+0.3) - undo
# all embeddings use even vs odd N              28.0 (+1.5) [52] - worse! And Nm is still very seasonal! Undo 

# more factors using N - ...

# add c10b, side symm: 26.9[2.9M] -> 26.1[4.0M] after 26 epochs test this in gameplay? to do 25.9[52] [v18] +199 - good!






# combine 2 sides: 26.9[12.4M] - not better
# do it right: 26.8[13.4M] - still not better
