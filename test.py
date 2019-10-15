import joblib
import pandas as pd
import timeit


clf = joblib.load('mlp.joblib')

sample_data = pd.read_csv('test.csv')  # first row of all features in the X dataset (6 sensor readings * 7 functions = 42 features)
start_time = timeit.default_timer()
prediction = clf.predict(sample_data)
total_time = (timeit.default_timer() - start_time) * 1000
print('The first prediction is ' + str(prediction[0]) + ' and the time taken for one prediction is ' + str(
    total_time) + ' ms.')


clf = joblib.load('rf.joblib')

sample_data = pd.read_csv('test.csv')  # first row of all features in the X dataset (6 sensor readings * 7 functions = 42 features)
start_time = timeit.default_timer()
prediction = clf.predict(sample_data)
total_time = (timeit.default_timer() - start_time) * 1000
print('The first prediction is ' + str(prediction[0]) + ' and the time taken for one prediction is ' + str(
    total_time) + ' ms.')
