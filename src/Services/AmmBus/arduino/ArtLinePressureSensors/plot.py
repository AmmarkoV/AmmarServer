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

class serialPlot:
    def __init__(self, fileCSV, serialPort = '/dev/ttyUSB0', serialBaud = 115200, plotLength = 100):
        self.port = serialPort
        self.baud = serialBaud
        self.plotMaxLength = plotLength 
        self.rawData = bytearray(100)
        self.data = collections.deque([0] * plotLength, maxlen=plotLength)
        self.dataB = collections.deque([0] * plotLength, maxlen=plotLength)
        self.isRun = True
        self.isReceiving = False
        self.thread = None
        self.plotTimer = 0
        self.previousTimer = 0
        self.fileCSV = fileCSV

        print('Trying to connect to: ' + str(serialPort) + ' at ' + str(serialBaud) + ' BAUD.')
        try:
            self.serialConnection = serial.Serial(serialPort, serialBaud, timeout=4)
            print('Connected to ' + str(serialPort) + ' at ' + str(serialBaud) + ' BAUD.')
        except:
            print("Failed to connect with " + str(serialPort) + ' at ' + str(serialBaud) + ' BAUD.')

    def readSerialStart(self):
        if self.thread == None:
            self.thread = Thread(target=self.backgroundThread)
            self.thread.start()
            # Block till we start receiving values
            while self.isReceiving != True:
                time.sleep(0.0001)

    def getSerialData(self, frame, lines, lineValueText, lineLabel, timeText):
        currentTimer = time.perf_counter()
        self.plotTimer = int((currentTimer - self.previousTimer) * 1000)     # the first reading will be erroneous
        self.previousTimer = currentTimer
        timeText.set_text('Plot Interval = ' + str(self.plotTimer) + 'ms')
        #value,  = struct.unpack('f', self.rawData)    # use 'h' for a 2 byte integer
        floats = [float(x) for x in self.rawData.split()] 
        #print(floats[0]," ",floats[1]," ",floats[2])
        self.fileCSV.write(str(floats[0]))
        self.fileCSV.write(",")
        self.fileCSV.write(str(floats[1]))
        self.fileCSV.write(",")
        self.fileCSV.write(str(floats[2]))
        self.fileCSV.write("\n")
        

 

        self.data.append(floats[0])    # we get the latest data point and append it to our array
        lines.set_data(range(self.plotMaxLength), self.data)
        lineValueText.set_text('[' + lineLabel + '] = ' + str(floats[0]))

        self.dataB.append(floats[0])    # we get the latest data point and append it to our array
        lines.set_data(range(self.plotMaxLength), self.dataB)
        lineValueText.set_text('[' + lineLabel + '] = ' + str(floats[1]))


        # self.csvData.append(self.data[-1])

    def backgroundThread(self):    # retrieve data
        #time.sleep(0.1)  # give some buffer time for retrieving data
        self.serialConnection.reset_input_buffer()
        while (self.isRun):
            #self.serialConnection.readinto(self.rawData)
            self.rawData = self.serialConnection.read_until()
            self.isReceiving = True
            #print(self.rawData)

    def close(self):
        self.isRun = False
        self.thread.join()
        self.serialConnection.close()
        print('Disconnected...')
        self.fileCSV.close() 


def main():
    timestr = time.strftime("%Y%m%d-%H%M%S")
    print (timestr)
   
    
    fileCSV = open("log_%s.csv"%(timestr),"w") 
    fileCSV.write("Channel1,Channel2,Channel2\n")

    # portName = 'COM5'     # for windows users
    portName = '/dev/ttyUSB0'
    baudRate = 115200
    maxPlotLength = 200
    dataNumBytes = 4        # number of bytes of 1 data point
    s = serialPlot(fileCSV, portName, baudRate, maxPlotLength )   # initializes all required variables
    s.readSerialStart()                                               # starts background thread

    # plotting starts below
    pltInterval = 1    # Period at which the plot animation updates [ms]
    xmin = 0
    xmax = maxPlotLength
    ymin = -(1)
    ymax = 1000
    fig = plt.figure()
    ax = plt.axes(xlim=(xmin, xmax), ylim=(float(ymin - (ymax - ymin) / 10), float(ymax + (ymax - ymin) / 10)))
    ax.set_title('Arduino Analog Read')
    ax.set_xlabel("time")
    ax.set_ylabel("AnalogRead Value")


    bx = plt.axes(xlim=(xmin, xmax), ylim=(float(ymin - (ymax - ymin) / 10), float(ymax + (ymax - ymin) / 10)))
    bx.set_title('Arduino Analog Read')
    bx.set_xlabel("time")
    bx.set_ylabel("AnalogRead Value")

    lineLabel = 'Potentiometer Value'
    timeText = ax.text(0.50, 0.95, '', transform=ax.transAxes)
    lines = ax.plot([], [], label=lineLabel)[0]
    #lines.append(bx.plot([], [], label=lineLabel)[0])
    lineValueText = ax.text(0.50, 0.90, '', transform=ax.transAxes)


    anim = animation.FuncAnimation(fig, s.getSerialData, fargs=(lines, lineValueText, lineLabel, timeText), interval=pltInterval)    # fargs has to be a tuple
 
    plt.legend(loc="upper left")
    plt.show()
 
    s.close()
    fileCSV.close()
 
if __name__ == '__main__':
    main()
