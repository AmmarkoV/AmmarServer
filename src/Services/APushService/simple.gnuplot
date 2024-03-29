#Use as :
#     gnuplot -e "filename='temperature_0001.log'" -c simple.gnuplot
#      
set datafile separator "|"  
set term png size 640,480
set autoscale y  
#set xrange [0:100]
set yrange [0:100];
set output "humidity.png"             
set style fill transparent solid 0.3 noborder
set style function filledcurves y1=0
set title "Humidity Data Plot"   
set xdata time
set xtics 30000
set timefmt "%s"
set format x "%d/%m\n%H:%M"
set xlabel "Date/Time"
set ylabel "Percentage"
#+7200 to go from UTC to UTC+2
if (!exists("filename")) filename='temperature_0001.log'
plot filename using ($2+7200):10 t "Humidity" lw 2 lc rgb 'red' with filledcurves x1 
set yrange [0:40];
set title "Temperature Data Plot"   
set output "temperature.png"          
set ylabel "Degrees Celsius "                              
plot filename using ($2+7200):9 t "Temperature" lw 2 lc rgb 'green'  with filledcurves x1
