#!/usr/bin/env python3

import click
import json
import termtables


@click.command()
@click.argument('first', type=click.Path(exists=True))
@click.argument('second', type=click.Path(exists=True))
def main(first, second):
    stats = tuple(read_stats(v) for v in (first, second))
    keys = set()
    for v in stats:
        keys.update(v.keys())
    keys = sorted(keys - {'total'})

    result = list()
    for k in keys:
        first_size = stats[0].get(k, 0)
        second_size = stats[1].get(k, 0)
        diff = second_size - first_size
        if diff != 0:
            result.append((k, first_size, diff, (second_size / first_size - 1) * 100 if first_size != 0 else 100))

    result.sort(key=lambda v: tuple(reversed(v)))

    diff = stats[1]['total'] - stats[0]['total']
    first_total = stats[0]['total']
    result.append(('total', first_total, diff, (stats[1]['total'] / first_total - 1) * 100) if first_total != 0 else 100)

    print(f'Preprocessed code size diff between {first} and {second}:\n')

    termtables.print(
        [(v[0], v[1], f'{v[2]:+}', f'{v[3]:+}') for v in result],
        header=['file', 'size, bytes', 'diff, bytes', 'diff, %'],
        style=termtables.styles.markdown,
    )


def read_stats(path):
    with open(path) as stream:
        return json.load(stream)

if __name__ == '__main__':
    main()
