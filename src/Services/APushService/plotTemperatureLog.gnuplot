

set datafile separator "|"
set title "Temperature Data Plot"
unset multiplot
set xdata time
set style data lines  
set term png
set timefmt "%Y|%m|%d|%H|%M|%S"
set format x "%m-%d\n%H:%M"
set xlabel "Time"
set ylabel "Traffic" 
set xrange ["2019-01-16 16:00":"2019-01-16 23:59"]
set autoscale y  
set output "datausage.png" 
plot "temperatures.log" using 1:2 t "inbound" w lines 
                                                                                                                                                     
