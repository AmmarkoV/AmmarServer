#!/usr/bin/env python3
 
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

def drawPlot(fig,label,sampleTime,dataSamplesA,dataSamplesB,dataSamplesC):
    fig.clear()
    ax = plt.subplot(111)

    #ax = fig.add_subplot(111)
    #ax.set_title(label, fontdict={'fontsize': 8, 'fontweight': 'medium'})

    plt.title(label, fontsize=18)
    plt.xlabel('Sensor Sample', fontsize=16)
    plt.ylabel('Sensor Pressure value', fontsize=16)


#    plt.plot(sampleTime, dataSamplesA, 'r' , sampleTime, dataSamplesB, 'g' , sampleTime, dataSamplesC, 'b')
    lineA, = plt.plot(sampleTime, dataSamplesA, 'r' , label='Channel A' )
    lineB, = plt.plot(sampleTime, dataSamplesB, 'g' , label='Channel B' )
    lineC, = plt.plot(sampleTime, dataSamplesC, 'b' , label='Channel C' )
    plt.legend(handles=[lineA,lineB,lineC])


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
     
    
    start = time.time()

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
                
                end = time.time()
                #sampleTime.append(1000*(end-start))
                sampleTime.append(samples)

                samples+=1 

                if (samples%50==0): 
                   if plt.get_fignums():
                      # window(s) open 
                      drawPlot(fig,'Realtime log_%s.csv @ %u s' % (timestr,(end-start)),sampleTime,dataSamplesA,dataSamplesB,dataSamplesC)   
                   else:
                      # no windows
                      print("User closed window so closing application") 
                      break             
                    

    serialConnection.close()
    print('Disconnected from arduino...')
    fileCSV.close()  
 
if __name__ == '__main__':
    main()
