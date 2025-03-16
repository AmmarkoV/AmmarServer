import csv
import matplotlib.pyplot as plt
import datetime

def parse_log_file(filename):
    timestamps = []
    temperatures = []
    humidities = []
    alarms = []
    
    with open(filename, 'r') as file:
        reader = csv.reader(file, delimiter='|')
        for row in reader:
            if len(row) < 11:
                continue  # Skip malformed rows
            
            timestamp = int(row[1])
            temp = float(row[8])
            humidity = float(row[9])
            alarm = float(row[10])*100
            
            timestamps.append(datetime.datetime.fromtimestamp(timestamp))
            temperatures.append(temp)
            humidities.append(humidity)
            alarms.append(alarm)
    
    return timestamps, temperatures, humidities, alarms

def plot_data(timestamps, temperatures, humidities,alarms):
    plt.figure(figsize=(12, 6))
    
    plt.plot(timestamps, temperatures, label='Temperature (°C)', color='red') #, marker='o'
    plt.plot(timestamps, humidities, label='Humidity (%)', color='blue') ##, marker='s'
    plt.plot(timestamps, alarms, label='Alarms (%)', color='green') #, marker='s'
    
    plt.xlabel('Time')
    plt.ylabel('Values')
    plt.title('Temperature and Humidity Over Time')
    plt.legend()
    plt.xticks(rotation=45)
    plt.grid(True)
    
    plt.show()

if __name__ == "__main__":
    filename = "temperature_0001.log"
    timestamps, temperatures, humidities,alarms = parse_log_file(filename)
    plot_data(timestamps, temperatures, humidities,alarms)

