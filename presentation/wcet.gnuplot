#set terminal jpeg medium
#set output "techniques.jpeg"
set terminal epslatex
set output "wcet.tex"
set size 0.7,1
set boxwidth 1.0 absolute
set style fill pattern noborder
#set style fill solid 1.0 border
#set style line 1 lt -1 linecolor rgb "blue" lw 3 
#set style line 2 lt -1 linecolor rgb "red" lw 3 
#set style line 3 lt -1 linecolor rgb "orange" lw 3
set style line 1 lt -1 lw 3 
set style line 2 lt -1 lw 3 
set style line 3 lt -1 lw 3
#set style histogram rowstacked
set style data histograms
#set mxtics 2
#set mytics 2
set xtics 10
#set ytics 2
set xrange [0:100]
set yrange [0:20]

#plot [85:100] exp(-(x-85))
plot [0:100] exp(0.00036*(x)**2), exp(-(0.3*(x-93)))+1
#set label 1 "percentage of control points" at graph -0.125, graph 0.5 center rotate
#set xlabel "Techniques"

#plot 'gnuplot/data' using 3 t "stronger" ls 1, '' using 4 t "weaker" ls 2 fs pattern 10, '' using 5:xtic(1) t "uncomparable" ls 3 fs pattern 6
#plot 'gnuplot/data' using 3 t "$\\subsetneq$" fs solid 0.3 ls 1, '' using 4 t "$\\supsetneq$" fs pattern 3 ls 2, '' using 5:xtic(1) t "uncomparable"  fs pattern 13 ls 3
