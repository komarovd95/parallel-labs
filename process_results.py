from os import listdir
from os.path import basename, isfile, join
import re
import math
import matplotlib.pyplot as plt
import statistics
import pygal

regexp = re.compile('lab(\d+)-(\w+|\w+-\w+-\w+-\w+-\w+)-(\d+)\.txt')
dir_path = './build'
source_files = [f for f in listdir(dir_path) if isfile(join(dir_path, f)) and regexp.match(f)]

lab_number = None
expiremental_data = {}
for file_name in source_files:
    file_name_match = regexp.match(file_name)
    lab_number = lab_number or file_name_match.group(1)
    n = int(file_name_match.group(3))
    if n not in expiremental_data:
        expiremental_data[n] = (None, {})
    (x, compilers_data) = expiremental_data[n]
    f = open(join(dir_path, file_name), 'r')
    samples = [sample.split(";") for sample in f]
    compiler_data = sorted([(int(sample[0]) // 1000, float(sample[1])) for sample in samples], key=lambda k: k[0])[1:-1]
    compiler_name = file_name_match.group(2)
    compilers_data[compiler_name] = compiler_data
    expiremental_data[n] = (x, compilers_data)

expiremental_data = sorted(expiremental_data.items())

compilers = []
for schedule_type in ["static", "dynamic", "guided"]:
    for chunk_size in [1, 2, 8, 64, 1024]:
        for n_threads in [2, 4, 10, 100]:
            compilers.append(("gcc-omp-{}-{}-{}".format(n_threads, schedule_type, chunk_size), n_threads))

# table
table_data = {compiler[0]: [] for compiler in compilers}
for (n, (_, compilers_data)) in expiremental_data:
    for (compiler, _) in compilers:
        compiler_data = compilers_data[compiler]
        table_data[compiler].append(compiler_data[0][1])

table_file = open(join(dir_path, "table-lab{}.csv".format(lab_number)), 'w')
for (compiler, _) in compilers:
    table_file.write(",".join([compiler] + [str(d) for d in table_data[compiler]]) + "\n")
table_file.close()

# boxplots
for (n, (_, compilers_data)) in expiremental_data:
    box_plot = pygal.Box(y_title="Время выполнения, мс")
    box_plot.title = "Размер задачи {}".format(n)
    for (compiler, _) in compilers:
        samples = compilers_data[compiler]
        box_plot.add(compiler, [sample[0] for sample in samples])
    box_plot.render_to_png("./build/boxplot-{}.png".format(n))

# full plot
full_plot_data = {compiler: [] for (compiler, _) in compilers}

full_plot = pygal.Line(x_title="Размер задачи", y_title="Время выполнения, с")
full_plot.title = "Лучшие результаты (среднее время, s)"
full_plot.x_labels = [str(n) for (n, _) in expiremental_data]
for (n, (_, compilers_data)) in expiremental_data:
    for (compiler, _) in compilers:
        samples = compilers_data[compiler]
        data = full_plot_data[compiler]
        data.append(min([sample[0] for sample in samples]) / 1000.0)

for (compiler, _) in compilers:
    full_plot.add(compiler, full_plot_data[compiler])
full_plot.render_to_png("./build/full-plot.png")

exit()

# speed up plots
def speed_up_plots(name, seq_compiler, compilers_info):
    data = {compiler: (threads, []) for (compiler, threads) in compilers_info}
    plot = pygal.Line(x_title="Размер задачи", y_title="Ускорение")
    plot.title = "Параллельное ускорение ({})".format(name)
    plot.x_labels = [str(n) for (n, _) in expiremental_data]

    for (n, (_, compilers_data)) in expiremental_data:
        seq_min = compilers_data[seq_compiler][0][0]
        for (compiler, _) in compilers_info:
            comp_min = compilers_data[compiler][0][0]
            data[compiler][1].append(seq_min / comp_min)

    for (compiler, n_threads) in compilers_info:
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
    [
        ("cc2", 2),
        ("cc4", 4),
        ("cc10", 10),
        ("cc100", 100)
    ]
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
def parallel_eff_plot(name, speed_up, compilers_info):
    plot = pygal.Line(x_title="Размер задачи", y_title="Эффективность")
    plot.title = "Параллельная эффективность ({})".format(name)
    plot.x_labels = [str(n) for (n, _) in expiremental_data]
    for (compiler, _) in compilers_info:
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
    [
        ("cc2", 2),
        ("cc4", 4),
        ("cc10", 10),
        ("cc100", 100)
    ]
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
