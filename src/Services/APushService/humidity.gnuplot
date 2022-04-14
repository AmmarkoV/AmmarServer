#Use as :
#     gnuplot -e "filename='temperature_0001.log'" -c simple.gnuplot > humidity.png
#    or 
#     tail -n 288 temperature_0001.log | gnuplot -c temperature.gnuplot > humidity.png
#      
set datafile separator "|" 
set term png size 640,480
set autoscale y  
#set xrange [0:100]
set yrange [0:100];    
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
if (!exists("filename")) filename='/dev/stdin'
plot filename using ($2+7200):10 t "Humidity" lw 2 lc rgb 'red' with filledcurves x1  
