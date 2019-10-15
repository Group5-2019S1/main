import numpy as np
import pandas as pd
import math
import time
import timeit
import os

from sklearn.model_selection import KFold
from sklearn import preprocessing
from sklearn import metrics
from sklearn.metrics import *
from sklearn.ensemble import RandomForestClassifier
from sklearn.neural_network import MLPClassifier
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import classification_report
from sklearn.metrics import confusion_matrix
from statistics import mode

def processData(scaler):
    # start reading raw data
    features = []
    labels = []
    raw_df = pd.read_csv('labelled_dataset.csv')
    cleaned_df = raw_df.drop(columns=['sample no', 'experiment', 'user id'])
    print('The dataset contains ' + str(cleaned_df.shape[0]) + ' data samples and ' + str(cleaned_df.shape[1]) +
          ' data columns.')
    print('The dataset labels are: ' + '\n' + str(pd.value_counts(cleaned_df['activity'].values)[1]) +
          ' "WALKING" data samples, ' + '\n' + str(pd.value_counts(cleaned_df['activity'].values)[2]) +
          ' "WALKING_UPSTAIRS" data samples, ' + '\n' + str(pd.value_counts(cleaned_df['activity'].values)[3]) +
          ' "WALKING_DOWNSTAIRS" data samples, ' + '\n' + str(pd.value_counts(cleaned_df['activity'].values)[4]) +
          ' "SITTING" data samples, ' + '\n' + str(pd.value_counts(cleaned_df['activity'].values)[5]) +
          ' "STANDING" data samples, ' + '\n' + str(pd.value_counts(cleaned_df['activity'].values)[6]) +
          ' "LAYING" data samples, ' + '\n' + str(pd.value_counts(cleaned_df['activity'].values)[7]) +
          ' "STAND_TO_SIT" data samples, ' + '\n' + str(pd.value_counts(cleaned_df['activity'].values)[8]) +
          ' "SIT_TO_STAND" data samples, ' + '\n' + str(pd.value_counts(cleaned_df['activity'].values)[9]) +
          ' "SIT_TO_LIE" data samples, ' + '\n' + str(pd.value_counts(cleaned_df['activity'].values)[10]) +
          ' "LIE_TO_SIT" data samples, ' + '\n' + str(pd.value_counts(cleaned_df['activity'].values)[11]) +
          ' "STAND_TO_LIE" data samples, ' + '\n' + str(pd.value_counts(cleaned_df['activity'].values)[12]) +
          ' "LIE_TO_STAND" data samples.')

    data_array = cleaned_df.values
    # * 0.7 uses 70% of dataset
    sample_size = math.ceil(len(data_array))#* 0.7)

    print('The dataset used in this demo contains the first ' + str(sample_size) + ' data samples.')
    X_data = data_array[0:sample_size, 1:]  # data taken from only 2nd column onwards
    y_data = data_array[0:sample_size, 0]  # only 1st column contains labels

    window_size = 50
    overlap = 25
    segmented_data = []
    labeled_data = []
    for i in range(int(len(X_data) / overlap)):
        segmented_data.append(X_data[i * overlap:((i * overlap) + window_size), 0:])
        # print("new")
        labeled_data.append(y_data[i * overlap:((i * overlap) + window_size)])

    #print(int(len(labeled_data)))

    for i in range(len(segmented_data)):
        temp_row = []
        for j in range(0, 6):
            temp = segmented_data[i][0:, j]
            mean = np.mean(temp)
            median = np.median(temp)
            maximum = np.amax(temp)
            minimum = np.amin(temp)
            rms = np.sqrt(np.mean(temp ** 2))
            std = np.std(temp)
            q75, q25 = np.percentile(temp, [75, 25])
            iqr = q75 - q25
            temp_row.append(mean)
            temp_row.append(median)
            temp_row.append(maximum)
            temp_row.append(std)
            temp_row.append(iqr)
            temp_row.append(minimum)
            temp_row.append(rms)
        features.append(temp_row)

    for i in range(len(labeled_data)):
        try:
            # print(int(mode(labeled_data[i])))
            labels.append(int(mode(labeled_data[i])))
        except:
            # print(int(labeled_data[i][0]))
            labels.append(int(labeled_data[i][0]))

    scaler.fit(features)
    features = scaler.transform(features)
    return features, labels

def KFoldCV(X, y, clf):
    kf = KFold(n_splits=10, shuffle=True)
    kf.get_n_splits(X)
    fold_index = 0
    sum_accuracy = 0
    # sum_precision = 0
    # sum_recall = 0
    for train_index, test_index in kf.split(X):
        X_train, X_test = X[train_index], X[test_index]
        y = np.asarray(y)
        y_train, y_test = y[train_index], y[test_index]
        clf.fit(X_train, y_train)
        predictions = clf.predict(X[test_index])
        accuracy = clf.score(X[test_index], y[test_index])
        # cm = confusion_matrix(y[test_index], predictions)
        # if fold_index == 1 or fold_index == 2 or fold_index == 3 or fold_index == 4 or fold_index == 9:
        print('In the %i fold, the classification accuracy is %f' % (fold_index, accuracy))
            # print('And the confusion matrix is: ')
            # print(cm)
        sum_accuracy = sum_accuracy + accuracy
        # sum_precision = sum_precision + precision_score(y_test, clf.predict(X_test))
        # sum_recall = sum_recall + recall_score(y_test, clf.predict(X_test))
        fold_index += 1

    sample_data = X[0:1, 0:]
    start_time = timeit.default_timer()
    clf.predict(sample_data)
    total_time = (timeit.default_timer() - start_time) * 1000
    print('The time taken for one prediction is ' + str(total_time) + ' s.')

    return sum_accuracy / fold_index


def generateConfusionMatrix(y_true, y_pred):
    y_actual = pd.Series(y_true, name='Actual')
    y_predicted = pd.Series(y_pred, name='Predicted')
    labeled_cm = pd.crosstab(y_actual, y_predicted, rownames=['Actual'], colnames=['Predicted'], margins=True)
    print(labeled_cm)


def main():

    scaler = StandardScaler()
    X, y = processData(scaler)

    # split into 80-20 train-test
    # X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=1)
    # split into 60-20-20 train-validate-test
    # X_train, X_val, y_train, y_val = train_test_split(X_train, y_train, test_size=0.2, random_state=1)

    rf = RandomForestClassifier(n_estimators=10, random_state=1)
    accuracy = KFoldCV(X, y, rf)
    y_predicted = rf.predict(X)
    generateConfusionMatrix(y, y_predicted)
    rf.fit(X, y)
    print('Using the Random Forest Classifier: \n')
    print("The accuracy using KFold cross validation is {}".format(accuracy))
    print(classification_report(y, y_predicted, labels=[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]))

    mlp = MLPClassifier(hidden_layer_sizes=(100, 100, 10), early_stopping=True, max_iter=50, shuffle=True,
                        batch_size=100, activation='tanh', verbose=True, tol=0.01)
    accuracy = KFoldCV(X, y, mlp)
    y_predicted = mlp.predict(X)
    generateConfusionMatrix(y, y_predicted)
    mlp.fit(X, y)
    print('Using the Multi Layer Perceptron: \n')
    print("The accuracy using KFold cross validation is {}".format(accuracy))
    print(classification_report(y, y_predicted, labels=[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]))
main()
