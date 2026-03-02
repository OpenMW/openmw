#!/usr/bin/env python3
"""
osg_stats.py is a script to analyze OpenSceneGraph log. It parses given file
and builds timeseries, histograms, plots, calculate statistics for a given
set of keys over given range of frames.
"""

import click
import collections
import json
import matplotlib.pyplot
import numpy
import operator
import os.path
import re
import statistics
import sys
import termtables


@click.command()
@click.option('--print_keys', is_flag=True,
              help='Print a list of all present keys in the input file.')
@click.option('--regexp_match', is_flag=True,
              help='Use all metric that match given key. '
                   'Can be used with stats, timeseries, cumulative_timeseries, hist, hist_threshold')
@click.option('--timeseries', type=str, multiple=True,
              help='Show a graph for given metric over time.')
@click.option('--cumulative_timeseries', type=str, multiple=True,
              help='Show a graph for cumulative sum of a given metric over time.')
@click.option('--timeseries_delta', type=str, multiple=True,
              help='Show a graph for delta between neighbouring frames of a given metric over time.')
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
@click.option('--stats_sum', is_flag=True,
              help='Add a row to stats table for a sum per frame of all given stats metrics.')
@click.option('--stats_sort_by', type=str, default=None, multiple=True,
              help='Sort stats table by given fields (source, key, sum, min, max etc).')
@click.option('--stats_table_format', type=click.Choice(['markdown', 'json']), default='markdown',
              help='Print table with stats in given format.')
@click.option('--precision', type=int,
              help='Format floating point numbers with given precision')
@click.option('--timeseries_sum', is_flag=True,
              help='Add a graph to timeseries for a sum per frame of all given timeseries metrics.')
@click.option('--cumulative_timeseries_sum', is_flag=True,
              help='Add a graph to timeseries for a sum per frame of all given cumulative timeseries.')
@click.option('--timeseries_delta_sum', is_flag=True,
              help='Add a graph to timeseries for a sum per frame of all given timeseries delta.')
@click.option('--begin_frame', type=int, default=0,
              help='Start processing from this frame.')
@click.option('--end_frame', type=int, default=sys.maxsize,
              help='End processing at this frame.')
@click.option('--frame_number_name', type=str, default='FrameNumber',
              help='Frame number metric name.')
@click.option('--hist_threshold', type=str, multiple=True,
              help='Show a histogram for given metric only for frames with threshold_name metric over threshold_value.')
@click.option('--threshold_name', type=str, default='Frame duration',
              help='Frame duration metric name.')
@click.option('--threshold_value', type=float, default=1.05/60,
              help='Threshold for hist_over.')
@click.option('--show_common_path_prefix', is_flag=True,
              help='Show common path prefix when applied to multiple files.')
@click.option('--moving_average_window', type=click.IntRange(min=1),
              help='Transform timeseries values to moving average with window of given number of points.')
@click.argument('path', type=click.Path(), nargs=-1)
def main(print_keys, regexp_match, timeseries, hist, hist_ratio, stdev_hist, plot, stats, precision,
         timeseries_sum, stats_sum, begin_frame, end_frame, path,
         cumulative_timeseries, cumulative_timeseries_sum, frame_number_name,
         hist_threshold, threshold_name, threshold_value, show_common_path_prefix, stats_sort_by,
         timeseries_delta, timeseries_delta_sum, stats_table_format, moving_average_window):
    sources = {v: list(read_data(v)) for v in path} if path else {'stdin': list(read_data(None))}
    if not show_common_path_prefix and len(sources) > 1:
        longest_common_prefix = os.path.commonprefix(list(sources.keys()))
        sources = {k.removeprefix(longest_common_prefix): v for k, v in sources.items()}
    keys = collect_unique_keys(sources)
    frames, begin_frame, end_frame = collect_per_frame(
        sources=sources, keys=keys, begin_frame=begin_frame,
        end_frame=end_frame, frame_number_name=frame_number_name,
    )
    if print_keys:
        for v in keys:
            print(v)
    def matching_keys(patterns):
        if regexp_match:
            return [key for pattern in patterns for key in keys if re.search(pattern, key)]
        return patterns
    convolve = None
    if moving_average_window is not None:
        convolve = numpy.ones(moving_average_window) / float(moving_average_window)
    if timeseries:
        draw_timeseries(sources=frames, keys=matching_keys(timeseries), add_sum=timeseries_sum, convolve=convolve,
                        begin_frame=begin_frame, end_frame=end_frame)
    if cumulative_timeseries:
        draw_cumulative_timeseries(sources=frames, keys=matching_keys(cumulative_timeseries),
                                   add_sum=cumulative_timeseries_sum, convolve=convolve, begin_frame=begin_frame,
                                   end_frame=end_frame)
    if timeseries_delta:
        draw_timeseries_delta(sources=frames, keys=matching_keys(timeseries_delta), add_sum=timeseries_delta_sum,
                              convolve=convolve, begin_frame=begin_frame, end_frame=end_frame)
    if hist:
        draw_hists(sources=frames, keys=matching_keys(hist))
    if hist_ratio:
        draw_hist_ratio(sources=frames, pairs=hist_ratio)
    if stdev_hist:
        draw_stdev_hists(sources=frames, stdev_hists=stdev_hist)
    if plot:
        draw_plots(sources=frames, plots=plot)
    if stats:
        print_stats(sources=frames, keys=matching_keys(stats), stats_sum=stats_sum, precision=precision,
                    sort_by=stats_sort_by, table_format=stats_table_format)
    if hist_threshold:
        draw_hist_threshold(sources=frames, keys=matching_keys(hist_threshold), begin_frame=begin_frame,
                            threshold_name=threshold_name, threshold_value=threshold_value)
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


def collect_per_frame(sources, keys, begin_frame, end_frame, frame_number_name):
    assert begin_frame < end_frame
    result = collections.defaultdict(lambda: collections.defaultdict(list))
    begin_frame = max(begin_frame, min(v[0][frame_number_name] for v in sources.values()))
    end_frame = min(end_frame, max(v[-1][frame_number_name] for v in sources.values()) + 1)
    for name in sources.keys():
        for key in keys:
            result[name][key] = [None] * (end_frame - begin_frame)
    for name, frames in sources.items():
        max_index = 0
        for frame in frames:
            number = frame[frame_number_name]
            if begin_frame <= number < end_frame:
                index = number - begin_frame
                max_index = max(max_index, index)
                for key in keys:
                    if key in frame:
                        result[name][key][index] = frame[key]
        for key in keys:
            values = result[name][key][:max_index + 1]
            result[name][key] = numpy.array(values)
    return result, begin_frame, end_frame


def collect_unique_keys(sources):
    result = set()
    for frames in sources.values():
        for frame in frames:
            for key in frame.keys():
                result.add(key)
    return sorted(result)


def draw_timeseries(sources, keys, add_sum, convolve, begin_frame, end_frame):
    fig, ax = matplotlib.pyplot.subplots()
    x = numpy.array(range(begin_frame, end_frame))
    for name, frames in sources.items():
        for key in keys:
            y = maybe_convolve(frames[key], convolve)
            ax.plot(x[:len(y)], y, label=f'{key}:{name}')
        if add_sum:
            y = sum_arrays_with_none([maybe_convolve(frames[k], convolve) for k in keys])
            ax.plot(x[:len(y)], y, label=f'sum:{name}', linestyle='--')
    ax.grid(True)
    ax.legend()
    fig.canvas.manager.set_window_title('timeseries')


def draw_cumulative_timeseries(sources, keys, add_sum, convolve, begin_frame, end_frame):
    fig, ax = matplotlib.pyplot.subplots()
    x = numpy.array(range(begin_frame, end_frame))
    for name, frames in sources.items():
        for key in keys:
            y = cumsum_with_none(maybe_convolve(frames[key], convolve))
            ax.plot(x[:len(y)], y, label=f'{key}:{name}')
        if add_sum:
            y = sum_arrays_with_none([cumsum_with_none(maybe_convolve(frames[k], convolve)) for k in keys])
            ax.plot(x[:len(y)], y, label=f'sum:{name}', linestyle='--')
    ax.grid(True)
    ax.legend()
    fig.canvas.manager.set_window_title('cumulative_timeseries')


def draw_timeseries_delta(sources, keys, add_sum, convolve, begin_frame, end_frame):
    fig, ax = matplotlib.pyplot.subplots()
    x = numpy.array(range(begin_frame + 1, end_frame))
    for name, frames in sources.items():
        for key in keys:
            y = diff_with_none(maybe_convolve(frames[key], convolve))
            ax.plot(x[:len(y)], y, label=f'{key}:{name}')
        if add_sum:
            y = sum_arrays_with_none([diff_with_none(maybe_convolve(frames[k], convolve)) for k in keys])
            ax.plot(x[:len(y)], y, label=f'sum:{name}', linestyle='--')
    ax.grid(True)
    ax.legend()
    fig.canvas.manager.set_window_title('timeseries_delta')


def draw_hists(sources, keys):
    fig, ax = matplotlib.pyplot.subplots()
    bins = numpy.linspace(
        start=min(min(min(v) for k, v in f.items() if k in keys) for f in sources.values()),
        stop=max(max(max(v) for k, v in f.items() if k in keys) for f in sources.values()),
        num=20,
    )
    for name, frames in sources.items():
        for key in keys:
            ax.hist(frames[key], bins=bins, label=f'{key}:{name}', alpha=1 / (len(keys) * len(sources)))
    ax.set_xticks(bins)
    ax.grid(True)
    ax.legend()
    fig.canvas.manager.set_window_title('hists')


def draw_hist_ratio(sources, pairs):
    fig, ax = matplotlib.pyplot.subplots()
    bins = numpy.linspace(
        start=min(min(min(a / b for a, b in zip(f[a], f[b])) for a, b in pairs) for f in sources.values()),
        stop=max(max(max(a / b for a, b in zip(f[a], f[b])) for a, b in pairs) for f in sources.values()),
        num=20,
    )
    for name, frames in sources.items():
        for a, b in pairs:
            ax.hist(frames[a] / frames[b], bins=bins, label=f'{a} / {b}:{name}', alpha=1 / (len(pairs) * len(sources)))
    ax.set_xticks(bins)
    ax.grid(True)
    ax.legend()
    fig.canvas.manager.set_window_title('hists_ratio')


def draw_stdev_hists(sources, stdev_hists):
    for key, scale in stdev_hists:
        scale = float(scale)
        fig, ax = matplotlib.pyplot.subplots()
        first_frames = next(v for v in sources.values())
        median = statistics.median(first_frames[key])
        stdev = statistics.stdev(first_frames[key])
        start = median - stdev / 2 * scale
        stop = median + stdev / 2 * scale
        bins = numpy.linspace(start=start, stop=stop, num=9)
        for name, frames in sources.items():
            values = [v for v in frames[key] if start <= v <= stop]
            ax.hist(values, bins=bins, label=f'{key}:{name}', alpha=1 / (len(stdev_hists) * len(sources)))
        ax.set_xticks(bins)
        ax.grid(True)
        ax.legend()
        fig.canvas.manager.set_window_title('stdev_hists')


def draw_plots(sources, plots):
    fig, ax = matplotlib.pyplot.subplots()
    for name, frames in sources.items():
        for x_key, y_key, agg in plots:
            if agg is None:
                ax.plot(frames[x_key], frames[y_key], label=f'x={x_key}, y={y_key}:{name}')
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
                    label=f'x={x_key}, y={y_key}, agg={agg}:{name}',
                )
    ax.grid(True)
    ax.legend()
    fig.canvas.manager.set_window_title('plots')


def print_stats(sources, keys, stats_sum, precision, sort_by, table_format):
    stats = list()
    for name, frames in sources.items():
        for key in keys:
            stats.append(make_stats(source=name, key=key, values=filter_not_none(frames[key]), precision=precision))
        if stats_sum:
            stats.append(make_stats(source=name, key='sum', values=sum_multiple(frames, keys), precision=precision))
    metrics = list(stats[0].keys())
    if sort_by:
        stats.sort(key=operator.itemgetter(*sort_by))
    if table_format == 'markdown':
        termtables.print(
            [list(v.values()) for v in stats],
            header=metrics,
            style=termtables.styles.markdown,
        )
    elif table_format == 'json':
        table = [dict(zip(metrics, row.values())) for row in stats]
        print(json.dumps(table))
    else:
        print(f'Unsupported table format: {table_format}')


def draw_hist_threshold(sources, keys, begin_frame, threshold_name, threshold_value):
    for name, frames in sources.items():
        indices = [n for n, v in enumerate(frames[threshold_name]) if v > threshold_value]
        numbers = [v + begin_frame for v in indices]
        x = [v for v in range(0, len(indices))]
        fig, ax = matplotlib.pyplot.subplots()
        ax.set_title(f'Frames with "{threshold_name}" > {threshold_value} ({len(indices)})')
        ax.bar(x, [frames[threshold_name][v] for v in indices], label=threshold_name, color='black', alpha=0.2)
        prev = 0
        for key in keys:
            values = [frames[key][v] for v in indices]
            ax.bar(x, values, bottom=prev, label=key)
            prev = values
        ax.hlines(threshold_value, x[0] - 1, x[-1] + 1, color='black', label='threshold', linestyles='dashed')
        ax.xaxis.set_major_locator(matplotlib.pyplot.FixedLocator(x))
        ax.xaxis.set_major_formatter(matplotlib.pyplot.FixedFormatter(numbers))
        ax.grid(True)
        ax.legend()
        fig.canvas.manager.set_window_title(f'hist_threshold:{name}')


def filter_not_none(values):
    return [v for v in values if v is not None]


def fixed_float(value, precision):
    return '{v:.{p}f}'.format(v=value, p=precision) if precision else value


def sum_multiple(frames, keys):
    result = collections.Counter()
    for key in keys:
        values = frames[key]
        for i, value in enumerate(values):
            if value is not None:
                result[i] += float(value)
    return numpy.array([result[k] for k in sorted(result.keys())])


def make_stats(source, key, values, precision):
    return collections.OrderedDict(
        source=source,
        key=key,
        number=len(values),
        min=fixed_float(min(values), precision) if values else '-',
        max=fixed_float(max(values), precision) if values else '-',
        sum=fixed_float(sum(values), precision) if values else '-',
        mean=fixed_float(statistics.mean(values), precision) if values else '-',
        median=fixed_float(statistics.median(values), precision) if values else '-',
        stdev=fixed_float(statistics.stdev(float(v) for v in values), precision) if values else '-',
        q95=fixed_float(numpy.quantile(values, 0.95), precision) if values else '-',
    )


def to_number(value):
    try:
        return int(value)
    except ValueError:
        return float(value)


def cumsum_with_none(values):
    cumsum = None
    result = list()
    for v in values:
        if v is None:
            result.append(None)
        elif cumsum is None:
            cumsum = v
            result.append(cumsum)
        else:
            cumsum += v
            result.append(cumsum)
    return numpy.array(result)


def diff_with_none(values):
    if len(values) < 2:
        return numpy.array([])
    prev = values[0]
    result = list()
    for v in values[1:]:
        if prev is None:
            result.append(v)
            prev = v
        elif v is None:
            result.append(v)
        else:
            result.append(v - prev)
            prev = v
    return numpy.array(result)


def sum_arrays_with_none(arrays):
    size = max(len(v) for v in arrays)
    result = list()
    for i in range(size):
        not_none_values = [v[i] for v in arrays if v[i] is not None]
        if not_none_values:
            result.append(sum(not_none_values))
        else:
            result.append(None)
    return numpy.array(result)


def maybe_convolve(values, convolve):
    if convolve is None:
        return values
    return numpy.convolve([v or 0 for v in values], convolve, 'valid')


if __name__ == '__main__':
    main()
