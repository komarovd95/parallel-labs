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
    compiler_data = sorted([(int(sample[0]) / 1000.0, float(sample[1])) for sample in samples], key=lambda k: k[0])[1:-1]
    compiler_name = file_name_match.group(2)
    compilers_data[compiler_name] = compiler_data
    expiremental_data[n] = (x, compilers_data)

expiremental_data = sorted(expiremental_data.items())

compilers = []
for schedule_type in ["static", "dynamic", "guided"]:
    for chunk_size in [1, 2, 8, 64, 1024]:
        for n_threads in [2, 4, 10, 100]:
            compilers.append(("gcc-omp-{}-{}-{}".format(n_threads, schedule_type, chunk_size), n_threads))

additional_data = []
for schedule_type in ["static", "dynamic", "guided"]:
    for chunk_size in [1, 2, 8, 64, 1024]:
        for n_threads in [2, 4, 10, 100]:
            additional_data.append(("gcc-omp-{}-{}-{}".format(n_threads, schedule_type, chunk_size), n_threads, schedule_type, chunk_size))

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

# confidence intervals
ts = {17: 2.11, 7: 2.365}
table_file = open(join(dir_path, "table-lab3-conf-intervals.csv"), 'w')
for (compiler, _) in compilers:
    conf_intervals = []
    for (n, (_, compilers_data)) in expiremental_data:
        compiler_data = compilers_data[compiler]
        samples = [cd[0] for cd in compiler_data]
        mean = statistics.mean(samples)
        std = math.sqrt(sum([(x - mean) ** 2 for x in samples]) / (len(samples) -1))
        coeff = ts[len(samples) - 1]
        conf_interval = coeff * (std / math.sqrt(len(samples)))
        conf_intervals.append(conf_interval)
    table_file.write(",".join([compiler] + [str(ci) for ci in conf_intervals]) + "\n")
table_file.close()

# boxplots
def draw_bp(w_size, compilers_data, compilers_f, sched_type):
    box_plot = pygal.Box(y_title="Время выполнения, мс")
    box_plot.title = "Размер задачи {}, {}".format(w_size, sched_type)
    for (compiler, n_threads, _, chunk_size) in compilers_f:
        samples = compilers_data[compiler]
        box_plot.add("{}, {}".format(chunk_size, n_threads), [sample[0] for sample in samples])
    box_plot.render_to_png("./build/boxplot-{}-{}.png".format(sched_type, w_size))

for (n, (_, compilers_data)) in expiremental_data:
    draw_bp(n, compilers_data, additional_data[0:20], "static")
    draw_bp(n, compilers_data, additional_data[20:40], "dynamic")
    draw_bp(n, compilers_data, additional_data[40:60], "guided")

# full plot
def draw_fp(chunk_size):
    compilers_info = [ad for ad in additional_data if ad[3] == chunk_size]
    full_plot_data = {(n_threads, sched_type): [] for (_, n_threads, sched_type, _) in compilers_info}
    full_plot = pygal.Line(x_title="Размер задачи", y_title="Время выполнения, с")
    full_plot.title = "Лучшие результаты, chunk-size={} (среднее время, s)".format(chunk_size)
    full_plot.x_labels = [str(n) for (n, _) in expiremental_data]
    for (n, (_, compilers_data)) in expiremental_data:
        for (compiler, n_threads, sched_type, _) in compilers_info:
            samples = compilers_data[compiler]
            data = full_plot_data[(n_threads, sched_type)]
            data.append(min([sample[0] for sample in samples]) / 1000.0)

    for (_, n_threads, sched_type, _) in compilers_info:
        full_plot.add("{}, {}".format(sched_type, n_threads), full_plot_data[(n_threads, sched_type)])
    full_plot.render_to_png("./build/full-plot-{}.png".format(chunk_size))

for chunk_size in [1, 2, 8, 64, 1024]:
    draw_fp(chunk_size)

# speed up plots
def speed_up_plots(name, seq_compiler, compilers_info):
    data = {compiler: (threads, []) for (compiler, threads, _, _) in compilers_info}
    plot = pygal.Line(x_title="Размер задачи", y_title="Ускорение")
    plot.title = "Параллельное ускорение ({})".format(name)
    plot.x_labels = [str(n) for (n, _) in expiremental_data]

    for (n, (_, compilers_data)) in expiremental_data:
        seq_min = compilers_data[seq_compiler][0][0]
        for (compiler, _, _, _) in compilers_info:
            comp_min = compilers_data[compiler][0][0]
            data[compiler][1].append(seq_min / comp_min)

    for (compiler, n_threads, s_type, c_size) in compilers_info:
        plot.add("{}, {}".format(s_type, c_size), data[compiler][1])
    plot.render_to_png("./build/{}-speed-up-plot.png".format(name))
    return data

for nt in [2, 4, 10, 100]:
    speed_up_plots("all, threads={}".format(nt), "gcc1", [ad for ad in additional_data if ad[1] == nt])
    
dynamic_s = speed_up_plots(
    "all",
    "gcc1",
    additional_data
)
static_s = speed_up_plots(
    "static, chunk-size=8",
    "gcc1",
    [ad for ad in additional_data if ad[3] == 8 and ad[2] == "static"]
)

dynamic_s = speed_up_plots(
    "dynamic, chunk-size=64",
    "gcc1",
    [ad for ad in additional_data if ad[3] == 64 and ad[2] == "dynamic"]
)

guided_s = speed_up_plots(
    "guided, chunk-size=1", 
    "gcc1",
    [ad for ad in additional_data if ad[3] == 1 and ad[2] == "guided"]
)

# parallel effiency plots
def parallel_eff_plot(name, speed_up, compilers_info):
    plot = pygal.Line(x_title="Размер задачи", y_title="Эффективность")
    plot.title = "Параллельная эффективность ({})".format(name)
    plot.x_labels = [str(n) for (n, _) in expiremental_data]
    for (compiler, _, _, _) in compilers_info:
        (n_threads, data) = speed_up[compiler]
        plot.add(str(n_threads), [d / n_threads for d in data])
    plot.render_to_png("./build/{}-eff-plot.png".format(name))


parallel_eff_plot(
    "guided, chunk-size=1", 
    guided_s, 
    [ad for ad in additional_data if ad[3] == 1 and ad[2] == "guided"]
)

parallel_eff_plot(
    "dynamic, chunk-size=64", 
    dynamic_s, 
    [ad for ad in additional_data if ad[3] == 64 and ad[2] == "dynamic"]
)

parallel_eff_plot(
    "static, chunk-size=8",
    static_s,
    [ad for ad in additional_data if ad[3] == 8 and ad[2] == "static"]
)
