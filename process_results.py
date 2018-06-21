from os import listdir
from os.path import basename, isfile, join
import re
import math
import matplotlib.pyplot as plt
import statistics
import pygal

regexp = re.compile('lab(\d+)-(\w+-\w+|\w+-\w+-\w+-\w+)-(\d+)\.txt')
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
    compiler_data = sorted([(float(sample[-2]) / 1000.0, float(sample[-1]), [float(s) / 1000.0 for s in sample[0:5]]) for sample in samples], key=lambda k: k[0])[1:-1]
    compiler_name = file_name_match.group(2)
    compilers_data[compiler_name] = compiler_data
    expiremental_data[n] = (x, compilers_data)

expiremental_data = sorted(expiremental_data.items())

sched_types = [
    ('dynamic', 64),
    ('static', 8),
    ('guided', 1)
]

compilers = []
for thread_type in ['omp', 'pthread']:
    for (sched_type, chunk_size) in sched_types:
        for n_threads in [2, 4, 10, 100]:
            compilers.append(("{}-{}-{}-{}".format(thread_type, n_threads, sched_type, chunk_size), n_threads))

# table
table_data = {compiler: [] for (compiler, _) in compilers}
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
table_file = open(join(dir_path, "table-lab{}-conf-intervals.csv".format(lab_number)), 'w')
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
def draw_bp(w_size, compilers_data):
    box_plot = pygal.Box(y_title="Время выполнения, мс")
    box_plot.title = "Размер задачи {}".format(w_size)
    for (compiler, _) in compilers:
        samples = compilers_data[compiler]
        box_plot.add(compiler, [sample[0] for sample in samples])
    box_plot.render_to_png("./build/boxplot-{}.png".format(w_size))

for (n, (_, compilers_data)) in expiremental_data:
    draw_bp(n, compilers_data)

# bar plot
def draw_brp(scheduling):
    bar_plot = pygal.StackedBar(y_title="Percents")
    labels = []
    for n_threads in [2, 4, 10, 100]:
        for thread_type in ['omp', 'pthread']:
            labels.append("{}-{}".format(thread_type, n_threads))
    bar_plot.x_labels = labels
    compilers_data = []
    for n_threads in [2, 4, 10, 100]:
        for thread_type in ['omp', 'pthread']:
            compilers_data.append("{}-{}-{}-{}".format(thread_type, n_threads, scheduling[0], scheduling[1]))
    (n, (_, data)) = expiremental_data[-1]
    bar_plot.title = "Steps {}".format(n)
    plot_data = {"generate": [], "map": [], "merge": [], "sort": [], "reduce": []}
    for compiler in compilers_data:
        samples = data[compiler][0][2]
        s = sum(samples)
        plot_data["generate"].append((samples[0] / s) * 100.0)
        plot_data["map"].append((samples[1] / s) * 100.0)
        plot_data["merge"].append((samples[2] / s) * 100.0)
        plot_data["sort"].append((samples[3] / s) * 100.0)
        plot_data["reduce"].append((samples[4] / s) * 100.0)
    for step in ["generate", "map", "merge", "sort", "reduce"]:
        bar_plot.add(step, plot_data[step])
    bar_plot.render_to_png("./build/barplot-{}.png".format(scheduling[0]))

for sched_type in sched_types:
    draw_brp(sched_type)

def draw_brp2(n_threads, scheduling):
    bar_plot = pygal.Bar(y_title="Full Time")
    bar_plot.title = "Время выполнения ({}, {}), мс".format(scheduling[0], scheduling[1])
    bar_plot.x_labels = [str(n) for (n, _) in expiremental_data]
    plot_data = {'omp': [], 'pthread': []}
    compilers_data = [("{}-{}-{}-{}".format(compiler, n_threads, scheduling[0], scheduling[1]), compiler) for compiler in ['omp', 'pthread']]
    for (n, (_, data)) in expiremental_data:
        for (compiler, c_name) in compilers_data:
            samples = data[compiler]
            plot_data[c_name].append(min([sample[0] for sample in samples]))
    for thread_type in ['omp', 'pthread']:
        bar_plot.add(thread_type, plot_data[thread_type])
    bar_plot.render_to_png("./build/fulltime-{}".format(scheduling[0]))

for sched_type in sched_types:
    draw_brp2(4, sched_type)

# line plots
single = {
    100000: 8.336, 
    1890000: 189.468, 
    3680000: 380.045, 
    5470000: 582.056, 
    7260000: 783.5, 
    9050000: 1011.387, 
    10840000: 1209.798, 
    12630000: 1406.593,
    14420000: 1634.340,
    16210000: 1843.313,
    18000000: 2105.945
}

def draw_ln(scheduling):
    plot = pygal.Line()
    plot.x_labels = [str(n) for (n, _) in expiremental_data]
    plot.title = "Parallel speedup ({}, {})".format(scheduling[0], scheduling[1])
    compilers_data = []
    for thread_type in ['omp', 'pthread']:
        for n_threads in [2, 4, 10, 100]:
            compilers_data.append("{}-{}-{}-{}".format(thread_type, n_threads, scheduling[0], scheduling[1]))
    plot_data = {compiler:[] for compiler in compilers_data}
    for (n, (_, data)) in expiremental_data:
        for compiler in compilers_data:
            samples = data[compiler]
            plot_data[compiler].append(single[n] / min([sample[0] for sample in samples]))
    for compiler in compilers_data:
        plot.add(compiler, plot_data[compiler])
    plot.render_to_png("./build/speedup-{}.png".format(scheduling[0]))

for sched_type in sched_types:
    draw_ln(sched_type)
