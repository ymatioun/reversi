from time import time
import numpy as np
import pandas as pd
import warnings, gc, os, random
import matplotlib.pyplot as plt
warnings.filterwarnings('ignore')
t0 = time()
np.random.seed(13)
random.seed(13)



### read part 1
##df = pd.read_csv('sigmas_odd_19.csv', header=None)
##df.columns = ['n','sc','s0','s1','s3','s5','s7','s9','s11','s13','s15','s17','s19']
##for col in df.columns:
##    df[col] = df[col].astype('int32')
##print('loaded data1', df.shape, int(time()-t0), 'sec')
##
### read part 2
##df1 = pd.read_csv('sigmas_even_18.csv', header=None)
##df1.columns = ['n','sc','s0','s2','s4','s6','s8','s10','s12','s14','s16','s18']
##df1.drop(['n','sc','s0'], axis=1, inplace=True)
##for col in df1.columns:
##    df1[col] = df1[col].astype('int32')
### combine
##df = pd.concat([df, df1], axis=1)
##print('loaded data2', df.shape, int(time()-t0), 'sec')
##
### read part 3
##df1 = pd.read_csv('sigmas_even_2.csv', header=None)
##df1.columns = ['n','sc','s0','mob1','mob2','s2']
##df1.drop(['n','sc','s0','s2'], axis=1, inplace=True)
##for col in df1.columns:
##    df1[col] = df1[col].astype('int32')
### combine
##df = pd.concat([df, df1], axis=1)
##
##del df1
##gc.collect()
##print('loaded data3', df.shape, int(time()-t0), 'sec')
##
##
### drop < 20
##df = df.loc[df['n'] >= 20]
##
### cap mobs at 21
##df['mob1'] = np.minimum(21, df['mob1'])
##df['mob2'] = np.minimum(21, df['mob2'])
##
### define differences
##fr = [2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19]
##to = [0,1,2,1,2,3,4,3, 4, 3, 4, 5, 4, 5, 4, 5, 6, 7]
##cols = []
##for i in range(len(fr)):
##    if 's'+str(fr[i]) in df.columns:
##        df['d'+str(fr[i])] = df['s'+str(fr[i])] - df['s'+str(to[i])]
##        cols.append('d'+str(fr[i]))
##        
##df.drop(['sc','s0','s1','s3','s5','s7','s9','s11','s13','s15','s17','s19','s2','s4','s6','s8','s10','s12','s14','s16','s18'], axis=1, inplace=True)
##gc.collect()
##df.to_pickle('df.pkl')


#'n', 'mob1', 'mob2', 'd2', 'd3', 'd4', 'd5', 'd6', 'd7', 'd8', 'd9', 'd10', 'd11', 'd12', 'd13', 'd14', 'd15', 'd16', 'd17', 'd18', 'd19'
df = pd.read_pickle('df.pkl') # 1.8 Gb. All cols are int32. mobs are capped at 21
x = np.load('posb.npy')



# only keep n <= 58
x = x[df['n'] <= 58]
df = df.loc[df['n'] <= 58].reset_index(drop=True)




#df2 = pd.read_csv('pos_sigma.csv')



# shuffle the data: 8 sec
np.random.seed(13)
idx = np.arange(df.shape[0])
np.random.shuffle(idx)
df = df.iloc[idx].reset_index(drop=True)
x = x[idx,:]
#df2 = df2.iloc[idx].reset_index(drop=True)
print('loaded data', df.shape, int(time()-t0), 'sec')



# test: select subset of data
#df = df.iloc[:1000000] # 1M

min_count = 50


# calc side8a patterns
mult = np.array(3**np.arange(8), dtype=np.int16).reshape(1,-1)

tmp = (x[:,[0,1,2,3,4,5,6,7]] * mult).sum(-1)
co2 = (x[:,[7,6,5,4,3,2,1,0]] * mult).sum(-1)
tmp = np.minimum(tmp, co2)

co1 = (x[:,[0,8,16,24,32,40,48,56]] * mult).sum(-1)
co2 = (x[:,[56,48,40,32,24,16,8,0]] * mult).sum(-1)
co1 = np.minimum(co1, co2)
tmp = np.append(tmp.reshape(-1,1), co1.reshape(-1,1), axis=1)

co1 = (x[:,[7,15,23,31,39,47,55,63]] * mult).sum(-1)
co2 = (x[:,[63,55,47,39,31,23,15,7]] * mult).sum(-1)
co1 = np.minimum(co1, co2)
tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

co1 = (x[:,[56,57,58,59,60,61,62,63]] * mult).sum(-1)
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
print('s8a', s8a.max(), int(time()-t0), 'sec')


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
print('s8b', s8b.max(), int(time()-t0), 'sec')



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
print('s8c', s8c.max(), int(time()-t0), 'sec')



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
print('s8d', s8d.max(), int(time()-t0), 'sec')


# calc corner9 patterns
mult = np.array(3**np.arange(9), dtype=np.int16).reshape(1,-1)

cor = (x[:,[0,1,2,8,9,10,16,17,18]] * mult).sum(-1)
co2 = (x[:,[0,8,16,1,9,17,2,10,18]] * mult).sum(-1)
cor = np.minimum(cor, co2)

co1 = (x[:,[56,57,58,48,49,50,40,41,42]] * mult).sum(-1)
co2 = (x[:,[56,48,40,57,49,41,58,50,42]] * mult).sum(-1)
co1 = np.minimum(co1, co2)
cor = np.append(cor.reshape(-1,1), co1.reshape(-1,1), axis=1)

co1 = (x[:,[63,62,61,55,54,53,47,46,45]] * mult).sum(-1)
co2 = (x[:,[63,55,47,62,54,46,61,53,45]] * mult).sum(-1)
co1 = np.minimum(co1, co2)
cor = np.append(cor, co1.reshape(-1,1), axis=1)

co1 = (x[:,[7,6,5,15,14,13,23,22,21]] * mult).sum(-1)
co2 = (x[:,[7,15,23,6,14,22,5,13,21]] * mult).sum(-1)
co1 = np.minimum(co1, co2)
cor = np.append(cor, co1.reshape(-1,1), axis=1)

# drop patterns with count < X (assign them to empty patterns)
a = pd.Series(cor.ravel()).value_counts() # count
a = a.index * (a>=min_count) # zero out small ones
di = a.to_dict()
cor9 = pd.Series(cor.ravel()).map(di).to_numpy().reshape(cor.shape)
del cor
gc.collect()

# replace
s = pd.Series(cor9.ravel())
l_c9 = list(s.unique())
l_c9.sort()
di = {}
for i, l1 in enumerate(l_c9):
    di[l1] = i
cor9 = np.array(s.map(di)).reshape(-1,4).astype(np.int16)
print('cor9', cor9.max(), int(time()-t0), 'sec')


# calc corner10 patterns
mult = np.array(3**np.arange(10), dtype=np.int16).reshape(1,-1)

tmp = (x[:,[0,1,2,3,4,8,9,10,11,12]] * mult).sum(-1)

co1 = (x[:,[0,8,16,24,32,1,9,17,25,33]] * mult).sum(-1)
tmp = np.append(tmp.reshape(-1,1), co1.reshape(-1,1), axis=1)

co1 = (x[:,[7,15,23,31,39,6,14,22,30,38]] * mult).sum(-1)
tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

co1 = (x[:,[7,6,5,4,3,15,14,13,12,11]] * mult).sum(-1)
tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

co1 = (x[:,[56,48,40,32,24,57,49,41,33,25]] * mult).sum(-1)
tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

co1 = (x[:,[56,57,58,59,60,48,49,50,51,52]] * mult).sum(-1)
tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

co1 = (x[:,[63,55,47,39,31,62,54,46,38,30]] * mult).sum(-1)
tmp = np.append(tmp, co1.reshape(-1,1), axis=1)

co1 = (x[:,[63,62,61,60,59,55,54,53,52,51]] * mult).sum(-1)
tmp = np.append(tmp, co1.reshape(-1,1), axis=1)


# drop patterns with count < X (assign them to empty patterns)
a = pd.Series(tmp.ravel()).value_counts() # count
a = a.index * (a>=min_count) # zero out small ones
di = a.to_dict()
tmp = pd.Series(tmp.ravel()).map(di).to_numpy().reshape(tmp.shape)

# replace
s = pd.Series(tmp.ravel())
l_c10 = list(s.unique())
l_c10.sort()
di = {}
for i, l1 in enumerate(l_c10):
    di[l1] = i
c10 = np.array(s.map(di)).reshape(-1,8).astype(np.int16)
del tmp
gc.collect()
print('c10', c10.max(), int(time()-t0), 'sec')



# construct sigma0
s0 = df.groupby('n')[['d2', 'd3', 'd4', 'd5', 'd6', 'd7', 'd8', 'd9', 'd10', 'd11', 'd12', 'd13', 'd14', 'd15', 'd16', 'd17', 'd18', 'd19']].std()
s0.columns = ['s2', 's3', 's4', 's5', 's6', 's7', 's8', 's9', 's10', 's11', 's12', 's13', 's14', 's15', 's16', 's17', 's18', 's19']

# prepare exclusion mask
s1 = (s0 != 0).astype(np.int8)
s1.columns = ['i2', 'i3', 'i4', 'i5', 'i6', 'i7', 'i8', 'i9', 'i10', 'i11', 'i12', 'i13', 'i14', 'i15', 'i16', 'i17', 'i18', 'i19']
s0 += s0 == 0 # remove zeros
s0 = s0.reset_index()
s1 = s1.reset_index()
for d in range(2, 20):
    col = 'i'+str(d)
    s1[col].loc[s1['n'] > 64 - d] = 0 # anything beyond 64 is excluded
    s1[col].loc[s1['n'] == 63 - d] = 0 # 63 is excluded. New search will not go past 58 pieces
    s1[col].loc[s1['n'] == 62 - d] = 0 # 62 is excluded
    s1[col].loc[s1['n'] == 61 - d] = 0 # 61 is excluded
    s1[col].loc[s1['n'] == 60 - d] = 0 # 60 is excluded
    s1[col].loc[s1['n'] == 59 - d] = 0 # 59 is excluded

    col = 'd'+str(d)
    df[col].loc[df['n'] > 64 - d] = 0 # anything beyond 64 is excluded
    df[col].loc[df['n'] == 63 - d] = 0 # 63 is excluded. New search will not go past 58 pieces
    df[col].loc[df['n'] == 62 - d] = 0 # 62 is excluded
    df[col].loc[df['n'] == 61 - d] = 0 # 61 is excluded
    df[col].loc[df['n'] == 60 - d] = 0 # 60 is excluded
    df[col].loc[df['n'] == 59 - d] = 0 # 59 is excluded

df = df.merge(s0, on='n', how='left')
df = df.merge(s1, on='n', how='left')

y = df[['d2', 'd3', 'd4', 'd5', 'd6', 'd7', 'd8', 'd9', 'd10', 'd11', 'd12', 'd13', 'd14', 'd15', 'd16', 'd17', 'd18', 'd19']].to_numpy().astype(np.int16)
y = y * y # square

df['n'] -= df['n'].min() # remove unused values


x = df[['n', 'mob1', 'mob2']].to_numpy().astype(np.int16)

x = np.concatenate((x, s8a, cor9, s8b, s8c, s8d, c10), axis=-1) # 3 + 4 + 4 + 4 + 4 + 4 + 8 = 31
del s8a, cor9
gc.collect()

x2 = df[['s2', 's3', 's4', 's5', 's6', 's7', 's8', 's9', 's10', 's11', 's12', 's13', 's14', 's15', 's16', 's17', 's18', 's19']].to_numpy().astype(np.float32)
x3 = df[['i2', 'i3', 'i4', 'i5', 'i6', 'i7', 'i8', 'i9', 'i10', 'i11', 'i12', 'i13', 'i14', 'i15', 'i16', 'i17', 'i18', 'i19']].to_numpy().astype(np.int8)
del df
gc.collect()


os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2' # 2 = INFO and WARNING messages are not printed
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import Layer
from tensorflow.keras.callbacks import Callback, LearningRateScheduler, EarlyStopping
from tensorflow.keras.optimizers.schedules import ExponentialDecay, CosineDecayRestarts
from tensorflow.keras.layers import Input, Dense, Concatenate, Flatten, Dropout, Embedding, Reshape
from tensorflow.keras.utils import Sequence
from tensorflow.keras.initializers import LecunNormal, RandomNormal, RandomUniform


# model
NUMB    = 1024*32    # number of samples in a batch. Tr data is 16 Mil positions.
lr0     = 5e-2       # starting learning rate
lr_dec  = 0.85       # 0.8 per epoch = 0.1 after 10 epochs.
L2      = 1e-7
E1      = 16         # 8/16/32 = +-0.0010 

verbose = 2
val_p = 0.2         # proportion of data to be used for validation
i_tr = int(x.shape[0] * (1-val_p))
yp = y[i_tr:]


##pred1 = df2.to_numpy()[:,1:] * x2
##pred1 = pred1 * x3 + (1 - x3) * 1 # remove* unused - set sigma to 1, loss to 0
##pred1 = pred1[i_tr:, :]
##o1 = (np.log(pred1) + yp / 2.0 /  pred1 / pred1).mean()
##print(np.round(o1, 4), int(time()-t0), 'sec') # 0.9917 - close to 0.9900 training


#def objective():
with tf.device('/GPU:0'):
    tf.random.set_seed(13)
    np.random.seed(13)
    
    # maximize log likelihood = -ln(s) - d^2 / 2 / s^2
    def logL(y_true, y_pred):
        ll = tf.math.log(y_pred) + y_true / 2.0 / y_pred / y_pred
        return tf.math.reduce_mean(ll, axis=-1)

    class print_lr(Callback):
        def on_epoch_end(self, epoch, logs=None):
            print('epoch', epoch + 1, 'lr', np.round(np.array(self.model.optimizer.learning_rate), 6), ' ', end='')

    class MyRelu(Layer):
        def call(self, x):
            return tf.math.maximum(0.0, x)

    class MySum(Layer):
        def call(self, x):
            return tf.math.reduce_sum(x, axis=-2)

    class MyDense(Layer):
        def call(self, x):
            x1 = tf.reshape(x[0], [-1, E1, 1])
            x2 = tf.reshape(x[1][:,:E1*18], [-1, E1, 18])
            x3 = x[1][:,E1*18:] # 18
            return tf.math.reduce_sum(x1 * x2, axis=-2) + x3
            
    i1 = Input(shape=(x.shape[1],), dtype='int16') # 3
    i2 = Input(shape=(x2.shape[1],), dtype='float32') # y.shape[1]
    i3 = Input(shape=(x3.shape[1],), dtype='int8') # y.shape[1]
    
    e0 = Embedding(input_dim=x[:,0].max()+1, output_dim=E1, mask_zero=False, embeddings_regularizer=tf.keras.regularizers.L2(L2))(i1[:,0]) # E1
    e1 = Embedding(input_dim=x[:,1].max()+1, output_dim=E1, mask_zero=False, embeddings_regularizer=tf.keras.regularizers.L2(L2))(i1[:,1]) # E1
    e2 = Embedding(input_dim=x[:,2].max()+1, output_dim=E1, mask_zero=False, embeddings_regularizer=tf.keras.regularizers.L2(L2))(i1[:,2]) # E1
    e3 = MySum()(Embedding(input_dim=x[:,3:7].max()+1, output_dim=E1, mask_zero=False, embeddings_regularizer=tf.keras.regularizers.L2(L2))(i1[:,3:7])) # E1: s8a
    e4 = MySum()(Embedding(input_dim=x[:,7:11].max()+1, output_dim=E1, mask_zero=False, embeddings_regularizer=tf.keras.regularizers.L2(L2))(i1[:,7:11])) # E1: c9
    e5 = MySum()(Embedding(input_dim=x[:,11:15].max()+1, output_dim=E1, mask_zero=False, embeddings_regularizer=tf.keras.regularizers.L2(L2))(i1[:,11:15])) # E1: s8b
    e6 = MySum()(Embedding(input_dim=x[:,15:19].max()+1, output_dim=E1, mask_zero=False, embeddings_regularizer=tf.keras.regularizers.L2(L2))(i1[:,15:19])) # E1: s8c
    e7 = MySum()(Embedding(input_dim=x[:,19:23].max()+1, output_dim=E1, mask_zero=False, embeddings_regularizer=tf.keras.regularizers.L2(L2))(i1[:,19:23])) # E1: s8d
    e8 = MySum()(Embedding(input_dim=x[:,23:31].max()+1, output_dim=E1, mask_zero=False, embeddings_regularizer=tf.keras.regularizers.L2(L2))(i1[:,23:31])) # E1: c10
    
    f1 = e1 + e2 + e3 + e4 + e0 + e5 + e6 + e7 + e8 # E1
    f2 = MyRelu()(f1) # E1
    #f4 = MyDense()([f2, e0a])
    f4 = Dense(y.shape[1], activation=None, use_bias=False, kernel_regularizer=tf.keras.regularizers.L2(L2))(f2)
    mult = 1 + f4 / 32.
    o = i2 * mult
    o = o * i3 + (1 - i3) * 1 # remove* unused - set sigma to 1, loss to 0

    model = tf.keras.Model(inputs=(i1, i2, i3), outputs=o)
    STEPS_PER_EPOCH = (x.shape[0] * (1 - val_p)) // NUMB
    lr_schedule = ExponentialDecay(lr0, decay_steps=STEPS_PER_EPOCH, decay_rate=lr_dec, staircase=True)
    model.compile(optimizer=keras.optimizers.Adam(lr_schedule), loss=logL)
    if verbose > 0:
        print(model.summary())
    es = EarlyStopping(monitor='val_loss', start_from_epoch=1, patience=5, verbose=min(1, verbose), mode='min', restore_best_weights=True)
    model.fit(x=(x[:i_tr,:], x2[:i_tr,:], x3[:i_tr,:]), y=y[:i_tr,:], epochs=25, validation_data=((x[i_tr:,:], x2[i_tr:,:], x3[i_tr:,:]), y[i_tr:,:]), batch_size=NUMB, verbose=verbose, callbacks=[es]) # , print_lr()
    pred = model.predict((x[i_tr:,:], x2[i_tr:,:], x3[i_tr:,:]), verbose=0, batch_size=NUMB)
    o = (np.log(pred) + yp / 2.0 /  pred / pred).mean()
    if verbose > 0:
        print(np.round(o, 4), int(time()-t0), 'sec')
    #return np.round(o, 4)

### param grid search ****************************************************************************************************************************************************
##di = {'lr_dec':[.9, .85, .95], 'L2':[3e-8, 1e-7, 3e-7, 9e-7], 'NUMB':[1024*32, 1024*16, 1024*8], 'lr0':[5e-2, 3e-2, 1e-1]}
##for p in range(2): # N passes *****
##    for k in di.keys(): # loop over params *****
##        s0, v0 = 1000, 0
##        vv = di[k]      
##        for v in vv: # loop over values *****
##            exec(k+' = '+str(v))
##            t0 = time() # reset
##            s = objective()
##            print('     ', k, v, s, int(time() - t0), 'sec')
##            if s < s0:
##                s0, v0 = s, v
##        exec(k+' = '+str(v0)) # use best value
##        print('best value', k, v0, s0, p, int(time() - t0), 'sec***********************')
##stop

r = (pred / x2[i_tr:,:]).ravel()
r = r[x3[i_tr:,:].ravel() == 1] # drop unused
print(np.round(r.min(), 2), np.round(r.max(), 1)) # print range
h = plt.hist(r, bins=300, log=True, alpha=0.5)
plt.show()

w = model.weights
for w1 in w:
    print(w1.numpy().shape)

# save:
#l_c9
np.savetxt('l_c9.txt', np.array(l_c9), fmt='%i') # 4553
#l_s8a
np.savetxt('l_s8a.txt', np.array(l_s8a), fmt='%i') # 3004
#l_s8b
np.savetxt('l_s8b.txt', np.array(l_s8b), fmt='%i') # 3321
#l_s8c
np.savetxt('l_s8c.txt', np.array(l_s8c), fmt='%i') # 3130
#l_s8d
np.savetxt('l_s8d.txt', np.array(l_s8d), fmt='%i') # 1356
#l_c10
np.savetxt('l_c10.txt', np.array(l_c10), fmt='%i') # 28936


files = ['c_mob1.txt', 'c_mob2.txt', 'c_s8a.txt', 'c_c9.txt', 'c_n.txt', 'c_s8b.txt', 'c_s8c.txt', 'c_s8d.txt', 'c_c10.txt']
mm = 0
for cc in range(len(files)):
    vv = np.round(w[cc].numpy() * 512, 0).astype(np.int32).ravel()
    np.savetxt(files[cc], vv, fmt='%i')
    print(vv.shape[0]//E1, cc, files[cc], vv.min(), vv.max())
    mm = max(mm, np.abs(vv).max())
print('range', mm, 'margin', np.round(2**15 / mm, 1)) # range 2442 margin 13.4

vv = w[-1].numpy() / 512. / 32. # undo mult of coeffs and mult of formula
np.savetxt('c_dd.txt', vv, fmt='%.5e')
print('c_dd', vv.shape, np.round(np.abs(vv).mean(), 7), np.round(np.abs(vv).max(), 5)) # 0.0001411 0.00109

# n, mobs               1.5753
# s8a                   1.5592
# c9                    1.5558
# s8a+c9                1.5505
# n is mult, not add    1.5501 1.549
# last=real D(18)       1.5479 1.547
# bias                              1.547 - no impr, undo        
# [n]=add               1.5496 1.5475

# remove unused         0.9964 0.9948
# tune params           0.9955 0.9946
# add s8bcd             0.9935 0.9925 [-21]
# add c10               0.9915 0.9900 [-25] sigma_v3: test it out ...
