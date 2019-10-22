import numpy as np
import pandas as pd
import math
import time
import timeit
import os
import matplotlib.pyplot as plt
import glob

#from joblib import dump,load
from sklearn.externals import joblib
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
from sklearn.model_selection import learning_curve
from sklearn.feature_selection import SelectKBest
from statistics import mode
from scipy.fftpack import fft

def processData(scaler):
    # Window size is 50 because sampling rate of the sensors from the obtained dataset is 50Hz
    # every 1 second, 50 readings are being taken, so 1 reading takes 1/50 = 0.02 seconds
    # 50% overlap, hence overlap is 25 for window size of 50
    window_size = 50
    overlap = 25

    # Start reading raw data
    features = []
    labels = []
    # raw_df = pd.read_csv('labelled_data2.csv')


    path = r'C:\Users\Siri\Desktop\clean data 2'  # use your path
    all_files = glob.glob(path + "/*.csv")

    li = []

    for filename in all_files:
        df = pd.read_csv(filename)
        li.append(df)

    raw_df = pd.concat(li)

    # Drop unnecessary columns from the dataset
    cleaned_df = raw_df.drop(columns=['current', 'voltage', 'power', 'energy'])

    print('The dataset contains ' + str(cleaned_df.shape[0]) + ' data samples and ' + str(cleaned_df.shape[1]) +
          ' data columns.')
    print('The dataset labels are: ' + '\n' + str(pd.value_counts(cleaned_df['activity'].values)[1]) +
          ' "activity 1" data samples, ' + '\n' + str(pd.value_counts(cleaned_df['activity'].values)[2]) +
          ' "activity 2" data samples, ' + '\n' + str(pd.value_counts(cleaned_df['activity'].values)[3]) +
          ' "activity 3" data samples, ' + '\n' + str(pd.value_counts(cleaned_df['activity'].values)[4]) +
          ' "activity 4" data samples, ' + '\n' + str(pd.value_counts(cleaned_df['activity'].values)[5]) +
          ' "activity 5" data samples.')

    data_array = cleaned_df.values
    # * 0.7 uses 70% of dataset
    sample_size = math.ceil(len(data_array))#* 0.7)

    print('The dataset used in this demo contains the first ' + str(sample_size) + ' data samples.')
    # Split X and Y data where X data is the inputs from accelerometer and gyroscope, and Y is the labels
    X_data = data_array[0:sample_size, 1:]  # data taken from only 2nd column onwards
    y_data = data_array[0:sample_size, 0]  # only 1st column contains labels

    segmented_data = []
    labeled_data = []

    # There are 758341 rows in the full dataset used.
    # the code implements the sliding window algorithm will split these rows into the segmented data empty array.
    # each row in that segmented data array will be a row of X_data from 0:overlap (so 0:50, 25:75, 50:100).
    # So this results in a total of 30333 rows for both the overlapped and X and corresponding Y data.
    for i in range(int(len(X_data) / overlap)):
        segmented_data.append(X_data[i * overlap:((i * overlap) + window_size), 0:])
        labeled_data.append(y_data[i * overlap:((i * overlap) + window_size)])

    # for each row of the segmented data array, and then for each X data within a row of the segmented data,
    # we take all 50 rows of 1 column of data (xyz of acc/gyro)
    # then we use these array of values for features in temp_row.
    # Each data column is multiplied by the number of features you have.
    # So if we have 6 columns, and 7 features, its 42 columns of data.
    for i in range(len(segmented_data)):
        temp_row = []
        for j in range(0, 24):
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

        #   Frequency Domain Feature - Power Spectral Density
            fourier_temp = fft(temp)
            #Freq domain features = Power spectral density, summation |ck|^2
            fourier = np.abs(fourier_temp) ** 2
            value = 0
            for x in range (len(fourier)):
                value = value + (fourier[x] * fourier[x])
            value = value / len(fourier)
            temp_row.append(value)
        #

        # features are added to the features array (as a single dimension).
        features.append(temp_row)

    # Same as above sliding window split but for Y data
    # We use mode because we are overlapping the labels for each window.
    # there might be transition moves in between the windows.
    # so the most frequently occurring label in this window is chosen as the output label for the prediction.
    # In case there is an equal 50:50 split of labels, then we choose the first label as shown in the ‘except’ block.
    for i in range(len(labeled_data)):
        try:
            labels.append(int(mode(labeled_data[i])))
        except:
            labels.append(int(labeled_data[i][0]))

    # Feature Selection
    # selector = SelectKBest()

    # import f_classif, k=33?

    # features = selector.fit_transform(features, labels)

    # Feature Scaling
    # Standardization of features rescales data to have a mean of 0 and a standard deviation of 1 (unit variance).
    # Feed the scaler with the features array, and the transform feature standardizes the features.
    scaler.fit(features)
    features = scaler.transform(features)
    return features, labels


def KFoldCV(X, y, clf):
    # KFold function that returns the split array (10 splits and shuffles).
    kf = KFold(n_splits=10, shuffle=True)
    kf.get_n_splits(X)
    fold_index = 0
    sum_accuracy = 0
    # sum_precision = 0
    # sum_recall = 0

    # split according to the X data in kf.split(X), kf.split (this function also returns the train and test indexes)
    # train index and text index iterated throughout the X dataset (same for the corresponding y labels).
    for train_index, test_index in kf.split(X):
        X_train, X_test = X[train_index], X[test_index]
        y = np.asarray(y)
        y_train, y_test = y[train_index], y[test_index]
        clf.fit(X_train, y_train)
        # predictions = clf.predict(X[test_index])
        accuracy = clf.score(X[test_index], y[test_index])
        # cm = confusion_matrix(y[test_index], predictions)
        # if fold_index == 1 or fold_index == 2 or fold_index == 3 or fold_index == 4 or fold_index == 9:
        print('In the %i fold, the classification accuracy is %f' % (fold_index, accuracy))
        sum_accuracy = sum_accuracy + accuracy
        # sum_precision = sum_precision + precision_score(y_test, clf.predict(X_test))
        # sum_recall = sum_recall + recall_score(y_test, clf.predict(X_test))
        fold_index += 1

    #6904:6905
    sample_data = X[0:1, 0:] # first row of all features in the X dataset (6 sensor readings * 7 functions = 42 features)
    start_time = timeit.default_timer()
    prediction = clf.predict(sample_data)
    total_time = (timeit.default_timer() - start_time) * 1000
    print('The first prediction is ' + str(prediction[0]) + ' and the time taken for one prediction is ' + str(total_time) + ' ms.')

    #  We then fit the classifier with the train data and run the predictions and get accuracy for each split (0-9).
    return sum_accuracy / fold_index

#  pd.series prints the labels and the numbers of the labels as actual and predicted.
#  actual and predicted is cross tabbed (which puts them in a cross matrix style).
def generateConfusionMatrix(y_true, y_pred):
    y_actual = pd.Series(y_true, name='Actual')
    y_predicted = pd.Series(y_pred, name='Predicted')
    labeled_cm = pd.crosstab(y_actual, y_predicted, rownames=['Actual'], colnames=['Predicted'], margins=True)
    print(labeled_cm)


def main():
    scaler = StandardScaler()
    X, y = processData(scaler)
    joblib.dump(scaler, 'scaler.pkl', protocol=2)  # Save scaler

    # df_newtest = X[5:6,:]
    # df_newtest = pd.DataFrame(df_newtest)
    # df_newtest.to_csv('test.csv')
    # exit()

    # split into 80-20 train-test
    # X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=1)
    # split into 60-20-20 train-validate-test
    # X_train, X_val, y_train, y_val = train_test_split(X_train, y_train, test_size=0.2, random_state=1)

    # For RF, number of estimators determines the total runtime and also classification accuracy.
    # Limited to 10 in this dataset for quick training time but can be increased more for increased accuracy
    #  Passing specific random_state=1 (or any other value), then each time you run, you'll get the same result.
    # rf = RandomForestClassifier(n_estimators=10, random_state=1)
    # accuracy = KFoldCV(X, y, rf)
    # y_predicted = rf.predict(X) #test
    # generateConfusionMatrix(y, y_predicted)
    # # rf.fit trains the model with the dataset. We are saving all the data into the model.
    # rf.fit(X, y) #train
    # dump(rf, 'rf.joblib')
    # print('Using the Random Forest Classifier: \n')
    # print("The accuracy using KFold cross validation is {}".format(accuracy))
    # print(classification_report(y, y_predicted, labels=[1, 2, 3, 4, 5]))

    # precision = ratio tp / (tp + fp) where tp is the number of true positives and fp the number of false positives.
    # precision is intuitively the ability of the classifier not to label as positive a sample that is negative.
    # recall = ratio tp / (tp + fn) where tp is the number of true positives and fn the number of false negatives.
    # recall is intuitively the ability of the classifier to find all the positive samples.
    # The F-beta score can be interpreted as a weighted harmonic mean of the precision and recall,
    # where an F-beta score reaches its best value at 1 and worst score at 0.

    # hidden layer(x,y,z). x=no. of input neurons, y=no. of hidden layers, z=no. of connections
    # ‘tanh’, the hyperbolic tan function, returns f(x) = tanh(x).
    # cannot use relu as relu gives only non-negative values, but we need from -1 to 1 for dance moves
    # early_stopping = True > to terminate training when validation score is not improving.
    # max_iter > Maximum number of iterations.
    # The solver iterates until convergence (determined by ‘tol’) or this number of iterations.
    # shuffle > Whether to shuffle samples in each iteration.
    # batch_size > Size of minibatches for stochastic optimizers.
    # verbose > Whether to print progress messages to stdout.
    # tol > Tolerance for the optimization.
    mlp = MLPClassifier(hidden_layer_sizes=(50, 25), early_stopping=True, max_iter=200, shuffle=True,
                        batch_size=100, activation='tanh', verbose=True, tol=0.001, learning_rate='adaptive')
    # mlp = MLPClassifier(early_stopping=True, max_iter=50, shuffle=True,
    #                     batch_size=100, activation='tanh', verbose=True, tol=0.01)
    accuracy = KFoldCV(X, y, mlp)
    y_predicted = mlp.predict(X)
    generateConfusionMatrix(y, y_predicted)

    print('Using the Multi Layer Perceptron: \n')
    print("The accuracy using KFold cross validation is {}".format(accuracy))
    print(classification_report(y, y_predicted, labels=[1, 2, 3, 4, 5]))

    mlp2 = MLPClassifier(hidden_layer_sizes=(50, 25), early_stopping=True, max_iter=200, shuffle=True,
                        batch_size=100, activation='tanh', verbose=True, tol=0.001, learning_rate='adaptive')
    mlp2.fit(X, y)
    #dump(mlp,'mlp.joblib')
    joblib.dump(mlp2, 'mlp.pkl', protocol=2) #Save Model
    print ("Saved\n")
main()
