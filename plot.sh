gnuplot "live_plot.gnuplot" &

while true; do
    cat "hasil" | tail -10 | cut -f1 > hasil_for_plot;
    sleep 1;
done
