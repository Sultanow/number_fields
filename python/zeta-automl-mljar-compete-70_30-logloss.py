#!/usr/bin/env python
# coding: utf-8

# In[1]:


#pip install mljar-supervised
#pip install rfpimp


# In[2]:


import pandas as pd
from sklearn.model_selection import train_test_split
from supervised.automl import AutoML
from sklearn.metrics import accuracy_score

# In[3]:


df = pd.read_csv("real_quad_fields_1_2.csv")


# In[4]:


print(df.class_number.value_counts()[:2])


# In[ ]:


primes_up_to_1000 = [1, 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541, 547, 557, 563, 569, 571, 577, 587, 593, 599, 601, 607, 613, 617, 619, 631, 641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701, 709, 719, 727, 733, 739, 743, 751, 757, 761, 769, 773, 787, 797, 809, 811, 821, 823, 827, 829, 839, 853, 857, 859, 863, 877, 881, 883, 887, 907, 911, 919, 929, 937, 941, 947, 953, 967, 971, 977, 983, 991, 997]
#list_primes_and_powers = [1, 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541, 547, 557, 563, 569, 571, 577, 587, 593, 599, 601, 607, 613, 617, 619, 631, 641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701, 709, 719, 727, 733, 739, 743, 751, 757, 761, 769, 773, 787, 797, 809, 811, 821, 823, 827, 829, 839, 853, 857, 859, 863, 877, 881, 883, 887, 907, 911, 919, 929, 937, 941, 947, 953, 967, 971, 977, 983, 991, 997]
#first_terms = [2, 3, 4, 5, 6, 7, 8, 9, 10, 11]

#for i in primes_up_to_1000[1:12]:
#    for j in first_terms :
#        if (i**j) <= 1000:
#            list_primes_and_powers.append(i**j)
#list_primes_and_powers.sort()

#Training Columns
col_train=[]
for i in primes_up_to_1000:
    col_train.append(i + 5)

#Column of the feature to predict
predict_col = 1

#notice that here we fix the test_size and not the training size.
df_train, df_test = train_test_split(df, test_size=0.3, stratify=df.iloc[:, predict_col])
y_train = df_train.iloc[:, predict_col]
y_test = df_test.iloc[:, predict_col]
X_train = df_train.iloc[:, col_train]
X_test = df_test.iloc[:, col_train]




#Compete Mode : To be used for Machine Learning competitions
#each model will be trained model_time_limit= #min * #seconds
automl_compete = AutoML(mode="Compete", model_time_limit=30*60, start_random_models=10,hill_climbing_steps=5,
top_models_to_improve=5, golden_features=True, features_selection=True,stack_models=True,
train_ensemble=True, explain_level=2, eval_metric="logloss")
automl_compete.fit(X_train, y_train)

predictions = automl_compete.predict_all(X_test)
print(predictions.head())
print("Test accuracy:", accuracy_score(y_test, predictions["label"].astype(int)))
