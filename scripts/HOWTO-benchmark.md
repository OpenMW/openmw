OpenMW ships with its own benchmarking tool. This document describes how to collect performance data and vizualise it.

Collecting data
===============

The data collected is the same as displayed by repeatedly pressing F3 and/or F4 while in the game (correspondance is shown below).
We can select what should be collected by setting the `OPENMW_OSG_STATS_LIST` environment variables to a semi-colon separated list of metrics to collect. There can be any combination and order doesn't matter. Note that data collection can **significantly reduce** performance

| Metric to collect | Equivalent in-game profiler                                       |
|-------------------|-------------------------------------------------------------------|
| `frame_rate`      | Frame rate                                                        |
| `engine`          | OpenMW specifics metric (white bars)                              |
| `event`           | Event traversal bar                                               |
| `update`          | Update traversal bar                                              |
| `rendering`       | Draw bar                                                          |
| `gpu`             | GPU bar                                                           |
| `times`           | Alias for `event;update;rendering;engine;gpu`, that is all graphs |
| `cameraobjects`   | Table shown when pressing F3 3 times                              |
| `viewerobjects`   | Table shown when pressing F3 4 times                              |
| `resource`        | Table shown when pressing F4                                      |

It is necessary to write the collected metrics to a file which needs to be defined by the `OPENMW_OSG_STATS_FILE` environment variable.

Example
-------

Posix shell
```sh
OPENMW_OSG_STATS_FILE=/tmp/stats OPENMW_OSG_STATS_LIST="resource;engine" /usr/local/bin/openmw
```

Windows PowerShell
```powershell
$env:OPENMW_OSG_STATS_FILE="c:\stats"
$env:OPENMW_OSG_STATS_LIST="resource;engine"
openmw
```


Analyzing results
=================

`scripts/osg_stats.py` is a python script that can perform statistical analysis and generate graphs from a trace. It doesn't need to be run on the same machine as the trace was taken.

`osg_stats.py --help` list the available options.

Examples
--------

### Print the available metrics in a given trace file

`osg_stats.py --print_keys /tmp/stats`

Sample output
>>>
gui_time_begin
gui_time_end
gui_time_taken
input_time_begin
input_time_end
input_time_taken
lua_time_begin
lua_time_end
lua_time_taken
mechanics_time_begin
mechanics_time_end
mechanics_time_taken
physics_time_begin
physics_time_end
physics_time_taken
physicsworker_time_begin
physicsworker_time_end
physicsworker_time_taken
script_time_begin
script_time_end
script_time_taken
sound_time_begin
sound_time_end
sound_time_taken
state_time_begin
state_time_end
state_time_taken
world_time_begin
world_time_end
world_time_taken
>>>

### Print a table with statistics data for some metrics

`osg_stats.py --stats physicsworker_time_taken --stats mechanics_time_taken --stats world_time_taken --stats physics_time_taken /tmp/stats`

Sample output

> | source     | key                      | number | min | max      | mean                   | median   | stdev                  | q95                  |
> |------------|--------------------------|--------|-----|----------|------------------------|----------|------------------------|----------------------|
> | /tmp/stats | physicsworker_time_taken | 56245  | 0.0 | 0.01526  | 0.0018826463330073784  | 7.7e-05  | 0.003210274653689913   | 0.009509799999999995 |
> | /tmp/stats | mechanics_time_taken     | 56245  | 0.0 | 0.015808 | 0.0030202102942483776  | 0.002177 | 0.002565895193458489   | 0.008489799999999995 |
> | /tmp/stats | world_time_taken         | 56245  | 0.0 | 0.003643 | 5.230777846919726e-05  | 5.1e-05  | 1.7322475294417906e-05 | 6.6e-05              |
> | /tmp/stats | physics_time_taken       | 56245  | 0.0 | 0.004407 | 0.00019985403146946396 | 0.000129 | 0.0002106166003676915  | 0.000697             |


### Plot a timeserie of aforementioned metrics, ignoring first 1000 frames

`osg_stats.py --begin_frame 1000 --end_frame 1200 --timeseries physicsworker_time_taken --timeseries mechanics_time_taken --timeseries world_time_taken --timeseries physics_time_taken /tmp/stats`

### Plot time spent in physics and mechanics depending on number of actors in the scene

`osg_stats.py --plot 'Physics Actors' physicsworker_time_taken mean --plot 'Physics Actors' mechanics_time_taken mean --plot 'Physics Actors' physics_time_taken mean /tmp/stats`

### Plot timeserie from 2 traces

`osg_stats.py --timeseries 'Frame Duration' /tmp/shadowson /tmp/shadowsoff`
