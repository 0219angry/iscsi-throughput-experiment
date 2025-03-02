set xrange [0:1000]
set yrange [0:20]
set format y "%.2f"
set xlabel "targetnode stress [Mbps]"
set ylabel "iSCSI writethroughput [MB/s]"
set grid
plot "data.dat" with linespoints linecolor 'black' pt 7
