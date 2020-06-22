#!/usr/bin/env python3
# Script to graph log of MQTT data
#
# Copyright (C) 2020  Kevin O'Connor <kevin@koconnor.net>
#
# This file may be distributed under the terms of the GNU GPLv3 license.
import optparse, json, datetime
import matplotlib

# To use this script, create an MQTT log with something like:
#  mosquitto_sub -F '%I;%t;%p' -t 'topic/data' > mylog &

MEASUREMENTS = [
    'battery', 'temperature', 'pressure', 'humidity', 'last_sleep_time',
]

def parse_log(logname, timestamp_info, min_date, max_date):
    f = open(logname, 'r')
    ts_base = 1. / 1000000.
    out = []
    for line in f:
        # Parse line
        parts = line.split(';')
        if len(parts) != 3:
            continue
        datestr, topic, value = parts
        topicparts = topic.split('/')
        try:
            d = datetime.datetime.fromisoformat(datestr.split('+')[0])
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
                secs = datetime.timedelta(seconds=(old_ts - ts) * ts_base)
                old_adj_date = d + secs
                for m in MEASUREMENTS:
                    if m in old_data:
                        out.append((old_adj_date, pcb, m, old_ts, old_data[m]))
            del pending[:]
        elif prev_ts - ts > 36000000000.:
            # Timestamp not valid - add to pending list
            pending.append(data)
            continue
        else:
            secs = datetime.timedelta(seconds=(ts - prev_ts) * ts_base)
            adj_date = prev_date + secs
        # Store sensor data
        for m in MEASUREMENTS:
            if m in data:
                out.append((adj_date, pcb, m, ts, data[m]))
    f.close()
    return out

def calc_wake_time(pdata):
    smooth_samples = 32
    times = []
    data = []
    cumulative_ticks = []
    total = 0
    # Calculate time awake for each network upload
    for i in range(len(pdata)-1):
        diff = pdata[i+1][4] - pdata[i][3]
        awake_time = diff / 1000000.
        if awake_time < 0. or awake_time > 10.:
            #print("skip", pdata[i][1], pdata[i][0], pdata[i][3], diff/1000000.)
            continue
        if awake_time < .5:
            continue
        total += diff
        cumulative_ticks.append(total)
        ccount = len(cumulative_ticks)
        if ccount > smooth_samples:
            elaps_ticks = total - cumulative_ticks[ccount - smooth_samples - 1]
            data.append(elaps_ticks / (smooth_samples * 1000000.))
            times.append(pdata[i][0])
    return times, data

def plot_data(data, graphs):
    labels = {'battery': 'Volts', 'temperature': 'Temperature (F)',
              'pressure': 'Pressure', 'humidity': 'Humidity (%)',
              'last_sleep_time': 'Upload time'}
    # Extract data
    bypcb = {}
    for d in data:
        if d[2] not in graphs:
            continue
        bypcb.setdefault(d[1], {}).setdefault(d[2], []).append(d)
    # Build plot
    fig, axes = matplotlib.pyplot.subplots(nrows=len(graphs), sharex=True)
    for pcbname in sorted(bypcb.keys()):
        for gtype, ax in zip(graphs, axes):
            pdata = bypcb[pcbname].get(gtype, [])
            if gtype == 'last_sleep_time':
                times, data = calc_wake_time(pdata)
            else:
                times = [p[0] for p in pdata]
                data = [p[4] for p in pdata]
                if gtype == 'temperature':
                    data = [d * 1.8 + 32.0 for d in data]
            ax.set_ylabel(labels[gtype])
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
    opts.add_option("-f", "--fields", type="string", dest="fields",
                    default="battery,temperature,humidity",
                    help="sensor fields to graph")
    opts.add_option("-m", "--min_date", type="string", dest="min_date",
                    default="2000-01-01", help="minimum date (YYYY-MM-DD)")
    opts.add_option("-M", "--max_date", type="string", dest="max_date",
                    default="9998-12-31", help="maximum date (YYYY-MM-DD)")
    options, args = opts.parse_args()
    if len(args) < 1:
        opts.error("Incorrect number of arguments")

    setup_matplotlib(options.output is not None)
    min_date = datetime.datetime.fromisoformat(options.min_date)
    max_date = datetime.datetime.fromisoformat(options.max_date)

    graphs = [gn.strip() for gn in options.fields.split(',')]
    for g in graphs:
        if g not in MEASUREMENTS:
            opts.error("Invalid field '%s' (available: %s)"
                       % (g, ", ".join(MEASUREMENTS)))

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
    fig = plot_data(data, graphs)

    # Show graph
    if options.output is None:
        matplotlib.pyplot.show()
    else:
        fig.set_size_inches(8, 6)
        fig.savefig(options.output)

if __name__ == '__main__':
    main()
