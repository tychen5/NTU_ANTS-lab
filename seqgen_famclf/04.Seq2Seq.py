import os,shutil,pickle,tqdm,sys,random,re,string,pause, datetime
# os.environ["CUDA_DEVICE_ORDER"]="PCI_BUS_ID"
# # The GPU id to use, usually either "0" or "1"
# os.environ["CUDA_VISIBLE_DEVICES"]="0"
import keras
import sent2vec
import seq2seq
from seq2seq.models import AttentionSeq2Seq
from seq2seq.models import Seq2Seq

import tensorflow as tf
import tensorboard as tb

import numpy as np
import pandas as pd
from tqdm import tqdm
from random import shuffle
from math import log, floor

from keras.utils import multi_gpu_model

# from keras import backend as K
from keras.models import *
from keras.layers import *
from keras.activations import *
from keras.callbacks import *
from keras.utils import *
from keras.layers.advanced_activations import *
from keras import *
from keras.engine.topology import *
from keras.optimizers import *

import gensim
from gensim.models.word2vec import *
from keras.preprocessing.text import *
from keras.preprocessing.sequence import *

from sklearn.model_selection import *
from sklearn.decomposition import *
from sklearn.cluster import *
from sklearn.metrics.pairwise import *

# from collections import Counter
from keras.utils.generic_utils import *
from keras import regularizers
import unicodedata as udata
from keras.applications import *
from keras.preprocessing.image import *

from keras import backend
# from python.keras import backend
# Embedding(10,20)

train_emb, train_emb_api, train_fam_ans, train_rep_ans = pickle.load(open('data/tree-rep-profiles-partial/TRAIN_vec.pkl','rb'))
valid_emb, valid_emb_api,valid_fam_ans,valid_rep_ans = pickle.load(open('data/tree-rep-profiles-partial/DEV_vec.pkl','rb'))
test_emb, test_emb_api,test_fam_ans,test_rep_ans = pickle.load(open('data/tree-rep-profiles-partial/TEST_vec.pkl','rb'))
# print('train of sent2vec vector:',train_emb.shape,train_emb_api.shape,train_fam_ans.shape,train_rep_ans.shape)
# print('valid of sent2vec vector:',valid_emb.shape,valid_emb_api.shape,valid_fam_ans.shape,valid_rep_ans.shape)
train_rep_ans = np.expand_dims(train_rep_ans,axis=-1)
valid_rep_ans = np.expand_dims(valid_rep_ans,axis=-1)
test_rep_ans = np.expand_dims(test_rep_ans,axis=-1)
print('test of sent2vec vector:',test_emb.shape,test_emb_api.shape,test_fam_ans.shape,test_rep_ans.shape)

def _shuffle(X, X2 ,X3,X4):
    randomize = np.arange(len(X))
    np.random.shuffle(randomize)
#     print(X.shape, Y.shape)
    return (X[randomize], X2[randomize],X3[randomize],X4[randomize])

train_emb, train_emb_api, train_fam_ans, train_rep_ans = _shuffle(train_emb, train_emb_api, train_fam_ans, train_rep_ans)
valid_emb, valid_emb_api,valid_fam_ans,valid_rep_ans = _shuffle(valid_emb, valid_emb_api,valid_fam_ans,valid_rep_ans)
# test_emb, test_emb_api,test_fam_ans,test_rep_ans  = _shuffle(test_emb,test_emb_api,test_fam_ans,test_rep_ans)
print('train of sent2vec vector:',train_emb.shape,train_emb_api.shape,train_fam_ans.shape,train_rep_ans.shape)
print('valid of sent2vec vector:',valid_emb.shape,valid_emb_api.shape,valid_fam_ans.shape,valid_rep_ans.shape)
# print('test of sent2vec vector:',test_emb.shape,test_emb_api.shape,test_fam_ans.shape,test_rep_ans.shape)

# scale = 'no'

def scaling(trainX,validX,testX,scale='min_max'):
#     if scale == 'min_max':
    max_value = max([np.max(trainX) , np.max(validX),np.max(testX)])
    min_value = min([np.min(trainX),np.min(validX),np.min(testX)])

    trainX = (trainX - min_value) / (max_value - min_value)
    validX = (validX - min_value) / (max_value - min_value )
    testX = (testX - min_value) / (max_value - min_value )
    print(np.max(trainX),np.max(validX))
    return trainX,validX,testX , max_value , min_value

train_emb,valid_emb,test_emb , max_value,min_value = scaling(train_emb,valid_emb,test_emb)
print(valid_emb.shape)

#parameter
# opt=Adam(decay=1e-20,amsgrad=False)
opt=Nadam()
batchSize=64#256
patien=15
epoch=300
hidden_dims=350
io_dim=700
input_lengths=train_emb.shape[1] #profile_Q3
output_lengths= train_rep_ans.shape[1]#rep_max size
depths=2
dp = 0.05
saveP = '/DATA/r06725035/model/NotScaleAttentionSeq2Seq_Param_'+str(io_dim)+'_'+str(input_lengths)+'_'+str(output_lengths)+'_'+str(batchSize)+'_'+str(hidden_dims)+'_'+str(depths)+'_'+str(dp)+'.h5'
logD = '/DATA/r06725035/logs/NotScaleAS2S_Param/'+str(batchSize)+'_'+str(hidden_dims)+'_'+str(depths)
history = History()
print("input:",train_emb.shape[1],'output_length:',train_rep_ans.shape[1])

# 模型參數
l2_reg_penalty = 1e-4#1e-4
embedding_dropout = 0.6
transformer_dropout = 0.1
use_universal_transformer = True #true=>ACT

transformer_depth = 2
vocabulary_size = 26 #api name種類
max_seq_length = test_emb.shape[1] # profile最大長度
word_embedding_size = test_emb.shape[2]#被除數，跟Sen2Vec最終維度相同
num_heads = 2#除數，要整除
fam_num = test_fam_ans.shape[1]#test_fam_ans.shape[1] MML
batch_size = 256 #128

CONFIDENCE_PENALTY = 0.1

model = AttentionSeq2Seq(input_dim=io_dim, input_length=input_lengths, hidden_dim=hidden_dims,
                         output_length=output_lengths, output_dim=1, depth=depths, dropout=dp)
# model = multi_gpu_model(model,gpus=3)

emb = model.input
byte = model.output

l2_regularizer = (regularizers.l2(l2_reg_penalty) if l2_reg_penalty else None)

family_input = Multiply()([emb,byte])
family_input = BatchNormalization()(family_input)
family_prediction = Bidirectional(LSTM(fam_num,kernel_regularizer=l2_regularizer,
                                            recurrent_regularizer=l2_regularizer,return_sequences=True,
                                           dropout=transformer_dropout, #stateful=True,
                                            recurrent_dropout=transformer_dropout,name='fam_feature'))(family_input)
family_prediction = Concatenate()([family_prediction,family_input])
family_prediction = BatchNormalization()(family_prediction)
family_prediction = (
        Bidirectional(GRU(fam_num,name='fam_clf'))(family_prediction) #int((word_embedding_size+vocabulary_size)/4)
) # 2nd stage，變成bidirectional加dense?，變成兩層?加L2
family_prediction = Dense(fam_num,activation='sigmoid',name='family')(family_prediction)
model = Model(inputs=model.input,outputs=family_prediction)
"""
# model = multi_gpu_model(model,gpus=2)
# model.add(LSTM(2))
# rnn = LSTM(64,input_shape =(502,128) )(model)
mm = Sequential()
mm.add(GRU(33,input_shape=model.output_shape[1:]))
kk= Sequential()
kk.add(GRU(66,input_shape=model.output_shape[1:]))
model = Model(inputs=model.input,outputs=[mm(model.output),kk(model.output)])
# rnn = LSTM(64,input_shape =(502,128) )(model)
# model.add(LSTM(2))
"""
model.summary()
model.load_weights('/tmp/NotScaleAttentionSeq2Seq_Param_700_213_213_64_350_2_0.05.h5_all.h5_single')
model = multi_gpu_model(model,gpus=2)


def f1_metric(y_true, y_pred):
    def recall(y_true, y_pred):
        """Recall metric.

        Only computes a batch-wise average of recall.

        Computes the recall, a metric for multi-label classification of
        how many relevant items are selected.
        """
        true_positives = K.sum(K.round(K.clip(y_true * y_pred, 0, 1)))
        possible_positives = K.sum(K.round(K.clip(y_true, 0, 1)))
        recall = true_positives / (possible_positives + K.epsilon())
        return recall

    def precision(y_true, y_pred):
        """Precision metric.

        Only computes a batch-wise average of precision.

        Computes the precision, a metric for multi-label classification of
        how many selected items are relevant.
        """
        true_positives = K.sum(K.round(K.clip(y_true * y_pred, 0, 1)))
        predicted_positives = K.sum(K.round(K.clip(y_pred, 0, 1)))
        precision = true_positives / (predicted_positives + K.epsilon())
        return precision

    precision = precision(y_true, y_pred)
    recall = recall(y_true, y_pred)
    return 2 * ((precision * recall) / (precision + recall + K.epsilon()))


def binary_focal_loss(gamma=2., alpha=.25):
    """
    Binary form of focal loss.
      FL(p_t) = -alpha * (1 - p_t)**gamma * log(p_t)
      where p = sigmoid(x), p_t = p or 1 - p depending on if the label is 1 or 0, respectively.
    References:
        https://arxiv.org/pdf/1708.02002.pdf
    Usage:
     model.compile(loss=[binary_focal_loss(alpha=.25, gamma=2)], metrics=["accuracy"], optimizer=adam)
    """

    def binary_focal_loss_fixed(y_true, y_pred):
        """
        :param y_true: A tensor of the same shape as `y_pred`
        :param y_pred:  A tensor resulting from a sigmoid
        :return: Output tensor.
        """
        pt_1 = tf.where(tf.equal(y_true, 1), y_pred, tf.ones_like(y_pred))
        pt_0 = tf.where(tf.equal(y_true, 0), y_pred, tf.zeros_like(y_pred))

        epsilon = K.epsilon()
        # clip to prevent NaN's and Inf's
        pt_1 = K.clip(pt_1, epsilon, 1. - epsilon)
        pt_0 = K.clip(pt_0, epsilon, 1. - epsilon)

        return -K.sum(alpha * K.pow(1. - pt_1, gamma) * K.log(pt_1)) \
               - K.sum((1 - alpha) * K.pow(pt_0, gamma) * K.log(1. - pt_0))

    return binary_focal_loss_fixed


def f1(y_true, y_pred):
    y_pred = K.round(y_pred)
    tp = K.sum(K.cast(y_true * y_pred, 'float'), axis=0)
    tn = K.sum(K.cast((1 - y_true) * (1 - y_pred), 'float'), axis=0)
    fp = K.sum(K.cast((1 - y_true) * y_pred, 'float'), axis=0)
    fn = K.sum(K.cast(y_true * (1 - y_pred), 'float'), axis=0)

    p = tp / (tp + fp + K.epsilon())
    r = tp / (tp + fn + K.epsilon())

    f1 = 2 * p * r / (p + r + K.epsilon())
    f1 = tf.where(tf.is_nan(f1), tf.zeros_like(f1), f1)
    return K.mean(f1)


def f1_loss(y_true, y_pred):
    tp = K.sum(K.cast(y_true * y_pred, 'float'), axis=0)
    tn = K.sum(K.cast((1 - y_true) * (1 - y_pred), 'float'), axis=0)
    fp = K.sum(K.cast((1 - y_true) * y_pred, 'float'), axis=0)
    fn = K.sum(K.cast(y_true * (1 - y_pred), 'float'), axis=0)

    p = tp / (tp + fp + K.epsilon())
    r = tp / (tp + fn + K.epsilon())

    f1 = 2 * p * r / (p + r + K.epsilon())
    f1 = tf.where(tf.is_nan(f1), tf.zeros_like(f1), f1)
    return 1 - K.mean(f1)



model.compile(optimizer=opt, loss=f1_loss, metrics=[f1])#binary_focal_loss(gamma=2., alpha=.25)
callback=[
    ReduceLROnPlateau(monitor='loss', factor=0.5, patience=int(patien/1.5),min_lr=1e-4,mode='min' ),
    EarlyStopping(patience=patien,monitor='val_loss',verbose=1),
    ModelCheckpoint(saveP,monitor='val_f1',mode='max',verbose=1,save_best_only=True, save_weights_only=True),
    TensorBoard(log_dir=logD),
    history,
]
model.fit(train_emb, train_fam_ans,
                epochs=epoch,
                batch_size=batchSize,
                shuffle=True,
                validation_data=(valid_emb, valid_fam_ans),
                callbacks=callback,
                class_weight='auto'
                )
model.save(saveP+"_all.h5")
