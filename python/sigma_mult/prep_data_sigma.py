import random, sys, gc, warnings, math, os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from time import time
t0 = time()



# this is how this data is defined in C *****************************
#typedef struct{
#	unsigned int pos32[4];	        //16. Here if i make it UINT64, size increases to 24 (multiple of 8).
#	unsigned char mob1;		//1, my mobility
#	unsigned char mob2;		//1, opponent's mobility
#	char sc;			//1
#	unsigned char tmp;	        //1.
#} posit;



from ctypes import *
class YourStruct(Structure):
    _fields_ = [('p0', c_uint32), ('p2', c_uint32), ('p1', c_uint32), ('p3', c_uint32),
                ('m1', c_uint8),  ('m2', c_uint8),
                ('sc', c_int8),   ('t', c_int8)]
posb = np.zeros([20460453, 64], dtype=np.int8) # hardcoded file size
mob = np.zeros([20460453, 2], dtype=np.uint8)
sc  = np.zeros(20460453, dtype=np.int8)
with open('../temp/pos_bin0.bin', 'rb') as file: # 409,209,060 = 20,460,453 positions
    i = 0
    x = YourStruct()
    while file.readinto(x) == sizeof(x):
        for j in range(32):
            posb[i, j] = (x.p0&1) - (x.p1&1)
            x.p0 = x.p0 // 2
            x.p1 = x.p1 // 2

            posb[i, j+32] = (x.p2&1) - (x.p3&1)
            x.p2 = x.p2 // 2
            x.p3 = x.p3 // 2
        
        mob[i,0] = x.m1
        mob[i,1] = x.m2
        sc[i]    = x.sc
        i += 1
        if i%1000000 == 0:
            print(i, int(time()-t0))
print('read data', i, int(time()-t0), 'sec')



# only keep n = 20 to 60
n = np.abs(posb).sum(-1) # 0 to 64
idx = (n>=20) & (n<=60)
posb = posb[idx,:]
mob = mob[idx,:]
sc = sc[idx]

np.save('posb.npy', posb)
np.save('mob.npy', mob)
np.save('sc.npy', sc)
print('saved', int(time()-t0), 'sec') # around 500 sec




