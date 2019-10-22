
import numpy as np
import pandas as pd
from scipy.fftpack import fft
from sklearn.externals import joblib
from sklearn import preprocessing
from sklearn.preprocessing import StandardScaler
from sklearn.preprocessing import MinMaxScaler

def extraction(readings):
    features = []
    data = np.array(readings)


    temp_row = []
    for j in range(0,24):
        temp = data[0:, j]
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
    features.append(temp_row)
   # print(features)
    X = np.array(features)
    scaler = joblib.load('scaler.pkl')
    features = scaler.transform(features)
   # print(features)
    return features
