import pandas as pd
import numpy as np
import time

from sklearn.model_selection import train_test_split
from sklearn.model_selection import cross_val_score
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import confusion_matrix
from sklearn.metrics import accuracy_score
from sklearn.ensemble import RandomForestClassifier
#from sklearn.neighbors import KNeighborsClassifier
from sklearn.svm import SVC


dataset = pd.read_csv('dataset.csv')

# Split Dataset
X = dataset.iloc[:, 5:11]
y = dataset.iloc[:, 4]
X_train, X_test, y_train, y_test = train_test_split(X, y, random_state=0, test_size=0.2)

# Feature Scaling
sc_X = StandardScaler()
X_train = sc_X.fit_transform(X_train)
X_test = sc_X.transform(X_test)

# Define the model: Init SVM
#classifier = SVC()
classifier = RandomForestClassifier(n_estimators=100, random_state=1)
#classifier = KNeighborsClassifier(n_neighbors=3)

# Fit Model
classifier.fit(X_train, y_train)

# Cross Validation Score
startCvTime = time.time()
score_dt = cross_val_score(classifier, X_train, y_train, cv=10)
endCvTime = time.time()
timeTakenCv = endCvTime - startCvTime

# Predict the test set results
startPredTime = time.time()
y_pred = classifier.predict(X_test)
endPredTime = time.time()
timeTakenPred = endPredTime - startPredTime

# Evaluate Model (Confusion matrix)
cm = confusion_matrix(y_test, y_pred)

# Printing of results
print('10-Fold Cross Validation Scores: ', score_dt)
print('Accuracy Score: ', accuracy_score(y_test, y_pred))
print('Confusion Matrix:')
print(cm)
print('Cross Validation Time: ', timeTakenCv)
print('Prediction Time: ', timeTakenPred)
