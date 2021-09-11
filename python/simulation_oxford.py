#!/usr/bin/env python
# coding: utf-8

# In[ ]:


get_ipython().system('pip install git+https://github.com/keras-team/keras-tuner.git@1.0.3')
get_ipython().system('pip install autokeras ')


# In[ ]:


import pandas as pd
import numpy as np
import tensorflow as tf
import autokeras as ak
from autokeras import StructuredDataClassifier
from autokeras import StructuredDataRegressor
from sklearn.model_selection import train_test_split


# In[ ]:


dataset=pd.read_csv('../input/number-field-h1vs2/n2_class_number_1vs2.csv')


# In[ ]:


from sklearn.preprocessing import minmax_scale
 
#dataset_scaled = minmax_scale(dataset[['class_number']], feature_range=(0,1))
 
#dataset['class_number']=dataset_scaled[:,0]
dataset['class_number'].hist()


# In[ ]:


primes_up_to_1000=[1,2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541, 547, 557, 563, 569, 571, 577, 587, 593, 599, 601, 607, 613, 617, 619, 631, 641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701, 709, 719, 727, 733, 739, 743, 751, 757, 761, 769, 773, 787, 797, 809, 811, 821, 823, 827, 829, 839, 853, 857, 859, 863, 877, 881, 883, 887, 907, 911, 919, 929, 937, 941, 947, 953, 967, 971, 977, 983, 991, 997]
col_train=[]
for i in primes_up_to_1000:
  col_train.append(i+12)
x=dataset.iloc[:,col_train].values
y=dataset.iloc[:,3].values
print(x,y)



# In[ ]:


x_train, x_rem, y_train, y_rem = train_test_split(x,y, train_size=0.3, random_state=1)

x_valid, x_test, y_valid, y_test = train_test_split(x_rem,y_rem, test_size=0.7,random_state=1)
#x_train, x_test, y_train, y_test = train_test_split(x, y, test_size = 0.7, random_state = 1)


# In[ ]:


#from sklearn.preprocessing import StandardScaler
#sc = StandardScaler() #can also use MinMaxScaler
#x_train = sc.fit_transform(x_train)
#x_test = sc.transform(x_test)
#x_train = x_train.reshape(-1, 1)
#x_train=sc.fit_transform(x_train)
#x_test = x_test.reshape(-1, 1)
#x_test = sc.transform(x_test)


# In[ ]:


searched_model = ak.StructuredDataClassifier(max_trials = 300,overwrite=True)


# In[ ]:


searched_model.fit(x=x_train, y=y_train,validation_data=(x_valid,y_valid))


# In[ ]:


#Evaluate the classifier on test data
_, acc = searched_model.evaluate(x_test, y_test)
print("Accuracy = ", (acc * 100.0), "%")


# In[ ]:


# get the final best performing model
model = searched_model.export_model()
print(type(model))  # <class 'tensorflow.python.keras.engine.training.Model'>
print(model.summary())
model.get_config()


# In[ ]:


try:
    model.save("model_autokeras_1vsAll_prime_zeta", save_format="tf")
except:
    model.save("model_autokeras_1vsAll_prime_zeta.h5")


# In[ ]:


#model.save("model_autokeras_1vsAll_prime_zeta.h5")
#model.save_weights('model_autokeras_1vsAll_prime_zeta_weights.h5')


# In[ ]:


from tensorflow.keras.models import load_model
loaded_model=load_model("model_autokeras_1vsAll_prime_zeta",custom_objects=ak.CUSTOM_OBJECTS)
#loaded_model.load_weights('model_autokeras_1vsAll_prime_zeta_weights.h5')


# In[ ]:


print(loaded_model.summary())
print(loaded_model.get_config())


# In[ ]:


loaded_model.predict(x_test)

