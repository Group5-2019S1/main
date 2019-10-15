import numpy as np
import pandas as pd
import scipy as sc
import math
import timeit

from sklearn.model_selection import KFold
from sklearn import preprocessing
from sklearn.metrics import *
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split
from sklearn.svm import SVC
from sklearn.neighbors import KNeighborsClassifier


def processData():

    # start reading raw data
    raw_df = pd.read_csv('dataset.csv')
    cleaned_df = raw_df.drop(columns=['date', 'time', 'username', 'wrist'])
    print('The dataset contains ' + str(cleaned_df.shape[0]) + ' data samples and ' + str(cleaned_df.shape[1]) +
          ' data columns.')
    print('The dataset contains ' + str(pd.value_counts(cleaned_df['activity'].values)[0]) +
          ' "walk" data samples as well as ' + str(pd.value_counts(cleaned_df['activity'].values)[1]) +
          ' "run" data samples.')

    data_array = cleaned_df.values
    sample_size = math.ceil(len(data_array)*0.90)

    print('The dataset used in this demo contains the first ' + str(sample_size) + ' data samples.')
    X_data = data_array[0:sample_size, 1:]  # data taken from only 2nd column onwards
    y_data = data_array[0:sample_size, 0]  # only 1st column contains labels

    y_data = y_data.ravel()
    y_rows = y_data.shape[0]
    y_shaped = y_data.reshape(y_rows, 1)

    return X_data, y_shaped

    # complete reading raw data

    # start segmenting
    # window_size = 50
    # overlap = math.floor(window_size/2)
    # X_segment = []
    # y_segment = []
    # for i in range(int(len(X_data)/overlap)):
    #       X_segment.append(X_data[i*overlap:(i*overlap)+window_size, 0:])
    #       y_segment.append(y_data[i*overlap:(i*overlap)+window_size])
    #print(X_segment)
    #print(y_segment)
    # complete segmenting


def overlapSegment(df, window_size):
    rows = df.shape[0]
    cols = df.shape[1]
    segment_count = int(rows/window_size)
    overlap_half = math.ceil(window_size/2.0)
    segmented_data = np.empty((segment_count*2-2, window_size, cols))  # Size of total segmented data array is (K-1)*2
    for i in range(segment_count-1):
        window = df[i*window_size:i*window_size+window_size,:]
        segmented_data[i*2] = np.vstack(window)
        window = df[i*window_size+overlap_half:i*window_size+window_size+overlap_half,:]
        segmented_data[i*2+1] = np.vstack(window)
    return segmented_data


def featureLabelExtraction(X_segment, y_segment):
    # start extracting features
    num_features = 5
    features = []
    labels = []
    for i in range(len(X_segment)):
        temp_row = []
        for j in range(num_features):
            temp = X_segment[i][0:, j]
            mean = np.mean(temp)
            median = np.median(temp)
            maximum = np.amax(temp)
            std = np.std(temp)
            q75, q25 = np.percentile(temp, [75, 25])
            iqr = q75 - q25
            temp_row.append(mean)
            temp_row.append(median)
            temp_row.append(maximum)
            temp_row.append(std)
            temp_row.append(iqr)

        # add Frequency domain features

        # prediction threshold algo and confidence level

        # feature selection algorithms

        features.append(temp_row)
    features = preprocessing.normalize(features)

    for i in range(len(y_segment)):
        mean = np.mean(y_segment[i])
        if mean >= 0.666:
            labels.append(int(1))
        else:
            labels.append(int(0))

    print('The ' + str(num_features) + ' features used in this demo are 1. Mean , 2. Median, 3. Maximum ' +
          '4.Standard Deviation, 5. Inter-quartile range.')

    #print(features)
    #print(labels)

    return features, labels

# complete extracting features


def KFoldCV(X, y, clf):
    kf = KFold(n_splits=10)
    kf.get_n_splits(X)
    fold_index = 0
    sum_accuracy = 0
    sum_precision = 0
    sum_recall = 0
    for train_index, test_index in kf.split(X):
        X_train, X_test = X[train_index], X[test_index]
        y = np.asarray(y)
        y_train, y_test = y[train_index], y[test_index]

        clf.fit(X_train, y_train)
        predictions = clf.predict(X[test_index])
        accuracy = clf.score(X[test_index], y[test_index])
        cm = confusion_matrix(y[test_index], predictions)
        if fold_index == 1 or fold_index == 2 or fold_index == 3 or fold_index == 4 or fold_index == 9:
            print('In the %i fold, the classification accuracy is %f' %(fold_index, accuracy))
            print('And the confusion matrix is: ')
            print(cm)
        sum_accuracy = sum_accuracy + accuracy_score(y_test, clf.predict(X_test))
        sum_precision = sum_precision + precision_score(y_test, clf.predict(X_test))
        sum_recall = sum_recall + recall_score(y_test, clf.predict(X_test))
        fold_index = fold_index + 1

    sample_data = X[0:1, 0:]
    start_time = timeit.default_timer()
    clf.predict(sample_data)
    total_time = (timeit.default_timer() - start_time) * 1000
    print('The time taken for one prediction is ' + str(total_time) + ' s.')

    return sum_accuracy/fold_index , sum_precision/fold_index, sum_recall/fold_index


def generateConfusionMatrix(y_true, y_pred):
    cm = confusion_matrix(y_true, y_pred)
    print(cm)


def main():
    window_size = 50
    X_data, y_data = processData()
    X_segment = overlapSegment(X_data, window_size)
    Y_segment = overlapSegment(y_data, window_size)
    X, y = featureLabelExtraction(X_segment, Y_segment)



    # split into 80-20 train-test
    # X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=1)
    # split into 60-20-20 train-validate-test
    # X_train, X_val, y_train, y_val = train_test_split(X_train, y_train, test_size=0.2, random_state=1)

    rf = RandomForestClassifier()
    print('\n')
    print('Using the Random Forest Classifier: \n')
    accuracy, precision, recall = KFoldCV(X, y, rf)
    print('The accuracy using KFold cross validation is ' + str(accuracy) + '.')
    print('The precision using KFold cross validation is ' + str(precision) + '.')
    print('The recall using KFold cross validation is ' + str(recall) + '.')
    rf.fit(X, y)
    generateConfusionMatrix(y, rf.predict(X))

    svm = SVC()
    print('\n')
    print('Using the Support Vector Machine Classifier: \n')
    accuracy, precision, recall = KFoldCV(X, y, svm)
    print('The accuracy using KFold cross validation is ' + str(accuracy) + '.')
    print('The precision using KFold cross validation is ' + str(precision) + '.')
    print('The recall using KFold cross validation is ' + str(recall) + '.')
    svm.fit(X, y)
    generateConfusionMatrix(y, svm.predict(X))

    knn = KNeighborsClassifier(n_neighbors=133)
    print('\n')
    print('Using KNN: \n')
    accuracy, precision, recall = KFoldCV(X, y, knn)
    print('The accuracy using KFold cross validation is ' + str(accuracy) + '.')
    print('The precision using KFold cross validation is ' + str(precision) + '.')
    print('The recall using KFold cross validation is ' + str(recall) + '.')
    knn.fit(X, y)
    generateConfusionMatrix(y, knn.predict(X))


main()


















