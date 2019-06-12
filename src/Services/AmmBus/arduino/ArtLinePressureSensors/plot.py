#!/usr/bin/env python
 
#pip3 install pyserial matplotlib --user

from threading import Thread
import serial
import time
import collections
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import time
import struct
import sys



def drawPlot(fig,sampleTime,dataSamplesA,dataSamplesB,dataSamplesC):
    fig.clear()
    #plt.subplot(211)
    plt.plot(sampleTime, dataSamplesA, 'k', sampleTime, dataSamplesB, 'k' , sampleTime, dataSamplesC, 'k')
    #plt.show() 
    plt.draw()
    plt.pause(0.001)

def main():
    timestr = time.strftime("%Y%m%d-%H%M%S")
    print (timestr)
   
    
    fileCSV = open("log_%s.csv"%(timestr),"w") 
    fileCSV.write("Channel1,Channel2,Channel2\n")

    # portName = 'COM5'     # for windows users
    serialPort = '/dev/ttyUSB0'
    serialBaud = 115200
    maxPlotLength = 300 
 
    rawData = bytearray(100)

    print('Trying to connect to: ' + str(serialPort) + ' at ' + str(serialBaud) + ' BAUD.')
    try:
        serialConnection = serial.Serial(serialPort, serialBaud, timeout=4)
        print('Connected to ' + str(serialPort) + ' at ' + str(serialBaud) + ' BAUD.')
    except:
        print("Failed to connect with " + str(serialPort) + ' at ' + str(serialBaud) + ' BAUD.')
    
     
    fig = plt.figure(constrained_layout=True)

    samples=0
    dataSamplesA= collections.deque([0] * maxPlotLength, maxlen=maxPlotLength)
    dataSamplesB= collections.deque([0] * maxPlotLength, maxlen=maxPlotLength)
    dataSamplesC= collections.deque([0] * maxPlotLength, maxlen=maxPlotLength)
    sampleTime=   collections.deque([0] * maxPlotLength, maxlen=maxPlotLength)
    while True:
                rawData = serialConnection.read_until()  
                floats = [float(x) for x in rawData.split()] 
                #print(samples," - ",floats[0]," ",floats[1]," ",floats[2])
                fileCSV.write(str(floats[0]))
                fileCSV.write(",")
                fileCSV.write(str(floats[1]))
                fileCSV.write(",")
                fileCSV.write(str(floats[2]))
                fileCSV.write("\n")
                fileCSV.flush()

                dataSamplesA.append(floats[0])
                dataSamplesB.append(floats[1])
                dataSamplesC.append(floats[2])
                sampleTime.append(samples)

                samples+=1 

                if (samples%50==0): 
                   if plt.get_fignums():
                      # window(s) open 
                      drawPlot(fig,sampleTime,dataSamplesA,dataSamplesB,dataSamplesC)   
                   else:
                      # no windows
                      print("User closed window so closing application") 
                      break             
                    

    serialConnection.close()
    print('Disconnected from arduino...')
    fileCSV.close()  
 
if __name__ == '__main__':
    main()
