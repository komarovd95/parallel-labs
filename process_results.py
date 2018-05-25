from os import listdir
from os.path import basename, isfile, join
import re
import math
import matplotlib.pyplot as plt
import statistics
import pygal

regexp = re.compile('lab(\d+)-(\w+)-(\d+)\.txt')
dir_path = './build'
source_files = [f for f in listdir(dir_path) if isfile(join(dir_path, f)) and regexp.match(f)]

measurement_results = {}
for source_file in source_files:
    match_result = regexp.match(source_file)
    n = int(match_result.group(3))
    if n not in measurement_results:
        measurement_results[n] = (None, {})
    (x, m) = measurement_results[n]
    f = open(join(dir_path, source_file), 'r')
    split = [line.split(";") for line in f]
    for s in split:
        result = float(s[1])
        if x:
            if not math.isclose(x, result, rel_tol=1e-6):
                print("Fail: x={}, result={}, source={}".format(x, result, source_file))
                exit(1)
        else:
            measurement_results[n] = (result, m)       

    measurements = sorted([int(s[0]) // 1000 for s in split])[1:-1]
    m[match_result.group(2)] = (statistics.mean(measurements), measurements)

measurement_results = sorted(measurement_results.items())
compilers_settings = [
    ("gcc1", 1),
    ("gcc2", 2),
    ("gcc4", 4),
    ("gcc10", 10),
    ("gcc100", 100),
    ("cc1", 1),
    ("cc4", 4),
    ("icc1", 1),
    ("icc2", 2),
    ("icc4", 4),
    ("icc10", 10),
    ("icc100", 100)
]

# boxplots
for (n, (_, measurements)) in measurement_results:
    box_plot = pygal.Box(y_title="Время выполнения, мс")
    box_plot.title = "Размер задачи {}".format(n)
    for (compiler, _) in compilers_settings:
        (_, m) = measurements[compiler]
        box_plot.add(compiler, m)
    box_plot.render_to_png("./build/boxplot-{}.png".format(n))

# full plot
full_plot_data = {compiler: [] for (compiler, _) in compilers_settings}

full_plot = pygal.Line(x_title="Размер задачи", y_title="Время выполнения, с")
full_plot.title = "Лучшие результаты (среднее время, s)"
full_plot.x_labels = [str(n) for (n, _) in measurement_results]
for (n, (_, measurements)) in measurement_results:
    for (compiler, _) in compilers_settings:
        (_, m) = measurements[compiler]
        data = full_plot_data[compiler]
        data.append(min(m) / 1000.0)

for (compiler, _) in compilers_settings:
    full_plot.add(compiler, full_plot_data[compiler])
full_plot.render_to_png("./build/full-plot.png")

# speed up plots
def speed_up_plots(name, seq_compiler, compilers):
    data = {compiler: (threads, []) for (compiler, threads) in compilers}
    plot = pygal.Line(x_title="Размер задачи", y_title="Ускорение")
    plot.title = "Параллельное ускорение ({})".format(name)
    plot.x_labels = [str(n) for (n, _) in measurement_results]

    for (n, (_, measurements)) in measurement_results:
        (sequential_mean, _) = measurements[seq_compiler]
        for (compiler, _) in compilers:
            (compiler_mean, _) = measurements[compiler]
            data[compiler][1].append(sequential_mean / compiler_mean)

    for (compiler, n_threads) in compilers:
        plot.add(str(n_threads), data[compiler][1])
    plot.render_to_png("./build/{}-speed-up-plot.png".format(name))
    return data

gcc_s = speed_up_plots(
    "GCC", 
    "gcc1", 
    [
        ("gcc2", 2),
        ("gcc4", 4),
        ("gcc10", 10),
        ("gcc100", 100)
    ]
)

cc_s = speed_up_plots(
    "CC",
    "cc1",
    [("cc4", 4)]
)

icc_s = speed_up_plots(
    "ICC",
    "icc1",
    [
        ("icc2", 2),
        ("icc4", 4),
        ("icc10", 10),
        ("icc100", 100)
    ]
)

# parallel effiency plots
def parallel_eff_plot(name, speed_up, compilers):
    plot = pygal.Line(x_title="Размер задачи", y_title="Эффективность")
    plot.title = "Параллельная эффективность ({})".format(name)
    plot.x_labels = [str(n) for (n, _) in measurement_results]
    for (compiler, _) in compilers:
        (n_threads, data) = speed_up[compiler]
        plot.add(str(n_threads), [d / n_threads for d in data])
    plot.render_to_png("./build/{}-eff-plot.png".format(name))


parallel_eff_plot(
    "GCC", 
    gcc_s, 
    [
        ("gcc2", 2),
        ("gcc4", 4),
        ("gcc10", 10),
        ("gcc100", 100)
    ]
)

parallel_eff_plot(
    "CC",
    cc_s,
    [("cc4", 4)]
)

parallel_eff_plot(
    "ICC",
    icc_s,
    [
        ("icc2", 2),
        ("icc4", 4),
        ("icc10", 10),
        ("icc100", 100)
    ]
)
