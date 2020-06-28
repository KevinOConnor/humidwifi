#!/usr/bin/env python3
# Script to graph log of MQTT data
#
# Copyright (C) 2020  Kevin O'Connor <kevin@koconnor.net>
#
# This file may be distributed under the terms of the GNU GPLv3 license.
import optparse, json
import matplotlib

# To use this script, create an MQTT log with something like:
#  mosquitto_sub -F '%I;%t;%p' -t 'topic/data' > mylog &

MEASUREMENTS = ['battery', 'temperature', 'pressure', 'humidity']

def parse_log(logname, timestamp_info, min_date, max_date):
    f = open(logname, 'r')
    ts_base = 1. / (24. * 60. * 60. * 1000000.)
    out = []
    for line in f:
        # Parse line
        parts = line.split(';')
        if len(parts) != 3:
            continue
        datestr, topic, value = parts
        topicparts = topic.split('/')
        try:
            d = matplotlib.dates.datestr2num(datestr)
            if d < min_date or d > max_date:
                continue
            pcb = topicparts[-2]
            data = json.loads(value.strip())
        except:
            continue
        # Remove duplicates
        ts = data.get('wake_time', data.get('boot_time'))
        if ts is None:
            continue
        pcb_info = timestamp_info.setdefault(pcb, [None, 1<<63, [], []])
        prev_date, prev_ts, recent_ts, pending = pcb_info
        if ts in recent_ts:
            continue
        del recent_ts[100:]
        recent_ts.insert(0, ts)
        # Calculate host based timestamp
        if data.get('latest'):
            pcb_info[0] = adj_date = d
            pcb_info[1] = ts
            # Add any sensor data that lacked a valid timestamp
            for old_data in pending:
                old_ts = old_data.get('wake_time', old_data.get('boot_time'))
                old_adj_date = d + (old_ts - ts) * ts_base
                for m in MEASUREMENTS:
                    if m in old_data:
                        out.append((old_adj_date, pcb, m, old_data[m]))
            del pending[:]
        elif prev_ts - ts > 36000000000.:
            # Timestamp not valid - add to pending list
            pending.append(data)
            continue
        else:
            adj_date = prev_date + (ts - prev_ts) * ts_base
        # Store sensor data
        for m in MEASUREMENTS:
            if m in data:
                out.append((adj_date, pcb, m, data[m]))
    f.close()
    return out

def plot_data(data):
    graphs = ['battery', 'temperature', 'humidity']
    #graphs.append('pressure')
    labels = ['Volts', 'Temperature (F)', 'Humidity (%)', 'Pressure']
    # Extract data
    bypcb = {}
    for d in data:
        if d[2] not in graphs:
            continue
        bypcb.setdefault(d[1], {}).setdefault(d[2], []).append(d)
    # Build plot
    fig, axes = matplotlib.pyplot.subplots(nrows=len(graphs), sharex=True)
    for pcbname in sorted(bypcb.keys()):
        for gtype, ax, label in zip(graphs, axes, labels):
            pdata = bypcb[pcbname].get(gtype, [])
            times = [p[0] for p in pdata]
            data = [p[3] for p in pdata]
            if gtype == 'temperature':
                data = [d * 1.8 + 32.0 for d in data]
            ax.set_ylabel(label)
            ax.plot_date(times, data, '-', label=pcbname, alpha=0.6)
            ax.grid(True)
    fontP = matplotlib.font_manager.FontProperties()
    fontP.set_size('x-small')
    axes[0].legend(loc='best', prop=fontP)
    axes[0].set_title("Sensor data")
    #axes[-1].set_xlabel('Date')
    return fig

def setup_matplotlib(output_to_file):
    global matplotlib
    if output_to_file:
        matplotlib.rcParams.update({'figure.autolayout': True})
        matplotlib.use('Agg')
    import matplotlib.pyplot, matplotlib.dates, matplotlib.font_manager
    import matplotlib.ticker

def main():
    # Parse command-line arguments
    usage = "%prog [options] <logfile> ..."
    opts = optparse.OptionParser(usage)
    opts.add_option("-o", "--output", type="string", dest="output",
                    default=None, help="filename of output graph")
    opts.add_option("-m", "--min_date", type="string", dest="min_date",
                    default="2000-01-01", help="minimum date (YYYY-MM-DD)")
    opts.add_option("-M", "--max_date", type="string", dest="max_date",
                    default="9998-12-31", help="maximum date (YYYY-MM-DD)")
    options, args = opts.parse_args()
    if len(args) < 1:
        opts.error("Incorrect number of arguments")

    setup_matplotlib(options.output is not None)
    min_date = matplotlib.dates.datestr2num(options.min_date)
    max_date = matplotlib.dates.datestr2num(options.max_date)

    # Parse data
    timestamp_info = {}
    data = []
    for logname in args:
        logdata = parse_log(logname, timestamp_info, min_date, max_date)
        data.extend(logdata)
    if not data:
        return
    data.sort()

    # Draw graph
    fig = plot_data(data)

    # Show graph
    if options.output is None:
        matplotlib.pyplot.show()
    else:
        fig.set_size_inches(8, 6)
        fig.savefig(options.output)

if __name__ == '__main__':
    main()
