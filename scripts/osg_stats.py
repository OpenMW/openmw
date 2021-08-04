#!/usr/bin/env python3
"""
osg_stats.py is a script to analyze OpenSceneGraph log. It parses given file
and builds timeseries, histograms, plots, calculate statistics for a given
set of keys over given range of frames.
"""

import click
import collections
import matplotlib.pyplot
import numpy
import statistics
import sys
import termtables

@click.command()
@click.option('--print_keys', is_flag=True,
              help='Print a list of all present keys in the input file.')
@click.option('--timeseries', type=str, multiple=True,
              help='Show a graph for given metric over time.')
@click.option('--hist', type=str, multiple=True,
              help='Show a histogram for all values of given metric.')
@click.option('--hist_ratio', nargs=2, type=str, multiple=True,
              help='Show a histogram for a ratio of two given metric (first / second). '
                   'Format: --hist_ratio <first_metric> <second_metric>.')
@click.option('--stdev_hist', nargs=2, type=str, multiple=True,
              help='Show a histogram for a standard deviation of a given metric at given scale (number). '
                   'Format: --stdev_hist <metric> <scale>.')
@click.option('--plot', nargs=3, type=str, multiple=True,
              help='Show a 2D plot for relation between two metrix (first is axis x, second is y)'
                   'using one of aggregation functions (mean, median). For example show a relation '
                   'between Physics Actors and physics_time_taken. Format: --plot <x> <y> <function>.')
@click.option('--stats', type=str, multiple=True,
              help='Print table with stats for a given metric containing min, max, mean, median etc.')
@click.option('--timeseries_sum', is_flag=True,
              help='Add a graph to timeseries for a sum per frame of all given timeseries metrics.')
@click.option('--stats_sum', is_flag=True,
              help='Add a row to stats table for a sum per frame of all given stats metrics.')
@click.option('--begin_frame', type=int, default=0,
              help='Start processing from this frame.')
@click.option('--end_frame', type=int, default=sys.maxsize,
              help='End processing at this frame.')
@click.argument('path', default='', type=click.Path())
def main(print_keys, timeseries, hist, hist_ratio, stdev_hist, plot, stats,
         timeseries_sum, stats_sum, begin_frame, end_frame, path):
    data = list(read_data(path))
    keys = collect_unique_keys(data)
    frames = collect_per_frame(data=data, keys=keys, begin_frame=begin_frame, end_frame=end_frame)
    if print_keys:
        for v in keys:
            print(v)
    if timeseries:
        draw_timeseries(frames=frames, keys=timeseries, timeseries_sum=timeseries_sum)
    if hist:
        draw_hists(frames=frames, keys=hist)
    if hist_ratio:
        draw_hist_ratio(frames=frames, pairs=hist_ratio)
    if stdev_hist:
        draw_stdev_hists(frames=frames, stdev_hists=stdev_hist)
    if plot:
        draw_plots(frames=frames, plots=plot)
    if stats:
        print_stats(frames=frames, keys=stats, stats_sum=stats_sum)
    matplotlib.pyplot.show()


def read_data(path):
    with open(path) if path else sys.stdin as stream:
        frame = dict()
        camera = 0
        for line in stream:
            if line.startswith('Stats Viewer'):
                if frame:
                    camera = 0
                    yield frame
                _, _, key, value = line.split(' ')
                frame = {key: int(value)}
            elif line.startswith('Stats Camera'):
                camera += 1
            elif line.startswith('    '):
                key, value = line.strip().rsplit(maxsplit=1)
                if camera:
                    key = f'{key} Camera {camera}'
                frame[key] = to_number(value)


def collect_per_frame(data, keys, begin_frame, end_frame):
    result = collections.defaultdict(list)
    for frame in data:
        for key in keys:
            if key in frame:
                result[key].append(frame[key])
            else:
                result[key].append(None)
    for key, values in result.items():
        result[key] = numpy.array(values[begin_frame:end_frame])
    return result


def collect_unique_keys(frames):
    result = set()
    for frame in frames:
        for key in frame.keys():
            result.add(key)
    return sorted(result)


def draw_timeseries(frames, keys, timeseries_sum):
    fig, ax = matplotlib.pyplot.subplots()
    x = numpy.array(range(max(len(v) for k, v in frames.items() if k in keys)))
    for key in keys:
        ax.plot(x, frames[key], label=key)
    if timeseries_sum:
        ax.plot(x, numpy.sum(list(frames[k] for k in keys), axis=0), label='sum')
    ax.grid(True)
    ax.legend()
    fig.canvas.set_window_title('timeseries')


def draw_hists(frames, keys):
    fig, ax = matplotlib.pyplot.subplots()
    bins = numpy.linspace(
        start=min(min(v) for k, v in frames.items() if k in keys),
        stop=max(max(v) for k, v in frames.items() if k in keys),
        num=20,
    )
    for key in keys:
        ax.hist(frames[key], bins=bins, label=key, alpha=1 / len(keys))
    ax.set_xticks(bins)
    ax.grid(True)
    ax.legend()
    fig.canvas.set_window_title('hists')


def draw_hist_ratio(frames, pairs):
    fig, ax = matplotlib.pyplot.subplots()
    bins = numpy.linspace(
        start=min(min(a / b for a, b in zip(frames[a], frames[b])) for a, b in pairs),
        stop=max(max(a / b for a, b in zip(frames[a], frames[b])) for a, b in pairs),
        num=20,
    )
    for a, b in pairs:
        ax.hist(frames[a] / frames[b], bins=bins, label=f'{a} / {b}', alpha=1 / len(pairs))
    ax.set_xticks(bins)
    ax.grid(True)
    ax.legend()
    fig.canvas.set_window_title('hists')


def draw_stdev_hists(frames, stdev_hists):
    for key, scale in stdev_hists:
        scale = float(scale)
        fig, ax = matplotlib.pyplot.subplots()
        median = statistics.median(frames[key])
        stdev = statistics.stdev(frames[key])
        start = median - stdev / 2 * scale
        stop = median + stdev / 2 * scale
        bins = numpy.linspace(start=start, stop=stop, num=9)
        values = [v for v in frames[key] if start <= v <= stop]
        ax.hist(values, bins=bins, label=key, alpha=1 / len(stdev_hists))
        ax.set_xticks(bins)
        ax.grid(True)
        ax.legend()
        fig.canvas.set_window_title('stdev_hists')


def draw_plots(frames, plots):
    fig, ax = matplotlib.pyplot.subplots()
    for x_key, y_key, agg in plots:
        if agg is None:
            ax.plot(frames[x_key], frames[y_key], label=f'x={x_key}, y={y_key}')
        elif agg:
            agg_f = dict(
                mean=statistics.mean,
                median=statistics.median,
            )[agg]
            grouped = collections.defaultdict(list)
            for x, y in zip(frames[x_key], frames[y_key]):
                grouped[x].append(y)
            aggregated = sorted((k, agg_f(v)) for k, v in grouped.items())
            ax.plot(
                numpy.array([v[0] for v in aggregated]),
                numpy.array([v[1] for v in aggregated]),
                label=f'x={x_key}, y={y_key}, agg={agg}',
            )
    ax.grid(True)
    ax.legend()
    fig.canvas.set_window_title('plots')


def print_stats(frames, keys, stats_sum):
    stats = [make_stats(key=key, values=filter_not_none(frames[key])) for key in keys]
    if stats_sum:
        stats.append(make_stats(key='sum', values=sum_multiple(frames, keys)))
    metrics = list(stats[0].keys())
    max_key_size = max(len(tuple(v.values())[0]) for v in stats)
    termtables.print(
        [list(v.values()) for v in stats],
        header=metrics,
        style=termtables.styles.markdown,
    )


def filter_not_none(values):
    return [v for v in values if v is not None]


def sum_multiple(frames, keys):
    result = collections.Counter()
    for key in keys:
        values = frames[key]
        for i, value in enumerate(values):
            if value is not None:
                result[i] += float(value)
    return numpy.array([result[k] for k in sorted(result.keys())])


def make_stats(key, values):
    return collections.OrderedDict(
        key=key,
        number=len(values),
        min=min(values),
        max=max(values),
        mean=statistics.mean(values),
        median=statistics.median(values),
        stdev=statistics.stdev(values),
        q95=numpy.quantile(values, 0.95),
    )


def to_number(value):
    try:
        return int(value)
    except ValueError:
        return float(value)

if __name__ == '__main__':
    main()
