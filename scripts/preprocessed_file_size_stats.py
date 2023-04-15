#!/usr/bin/env python3

import clang.cindex
import click
import json
import os
import os.path
import sys


@click.command()
@click.option('--remove_prefix', type=str, default='')
@click.argument('build_dir', type=click.Path(exists=True))
def main(build_dir, remove_prefix):
    libclang = os.environ.get('LIBCLANG')
    if libclang is not None:
        clang.cindex.Config.set_library_file(libclang)
    db = clang.cindex.CompilationDatabase.fromDirectory(build_dir)
    files = dict()
    total = 0
    for command in db.getAllCompileCommands():
        try:
            size = os.stat(os.path.join(command.directory, get_output_path(command.arguments))).st_size
            files[command.filename.removeprefix(remove_prefix)] = size
            total += size
        except Exception as e:
            print(f'Failed to process command for {command.filename}: {e}', file=sys.stderr)
            pass
    files['total'] = total
    json.dump(files, sys.stdout)


def get_output_path(arguments):
    return_next = False
    for v in arguments:
        if return_next:
            return v
        elif v == '-o':
            return_next = True

if __name__ == '__main__':
    main()
