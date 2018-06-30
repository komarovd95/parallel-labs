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
    compiler_data = sorted([(float(sample[-2]) / 1000.0, float(sample[-1]), [float(s) / 1000.0 for s in sample[:-2]]) for sample in samples], key=lambda k: k[0])[1:-1]
    compiler_name = file_name_match.group(2)
    compilers_data[compiler_name] = compiler_data
    expiremental_data[n] = (x, compilers_data)

expiremental_data = sorted(expiremental_data.items())

sched_types = [
    ('guided', 1)
]

compilers = ['omp', 'ocl']

# table
table_data = {compiler: [] for compiler in compilers}
for (n, (_, compilers_data)) in expiremental_data:
    for compiler in compilers:
        compiler_data = compilers_data[compiler]
        table_data[compiler].append(compiler_data[0][1])

table_file = open(join(dir_path, "table-lab{}.csv".format(lab_number)), 'w')
for compiler in compilers:
    table_file.write(",".join([compiler] + [str(d) for d in table_data[compiler]]) + "\n")
table_file.close()

# confidence intervals
ts = {17: 2.11, 7: 2.365}
table_file = open(join(dir_path, "table-lab{}-conf-intervals.csv".format(lab_number)), 'w')
for compiler in compilers:
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
    for compiler in compilers:
        samples = compilers_data[compiler]
        box_plot.add(compiler, [sample[0] for sample in samples])
    box_plot.render_to_png("./build/boxplot-{}.png".format(w_size))

for (n, (_, compilers_data)) in expiremental_data:
    draw_bp(n, compilers_data)

# fullplot
full_plot = pygal.Line(y_title="Time, ms")
full_plot.title = "Full time, ms"
full_plot.x_labels = [str(n) for (n, _) in expiremental_data]
full_plot_data = {compiler: [] for compiler in compilers}
for (n, (_, compilers_data)) in expiremental_data:
    for compiler in compilers:
        samples = compilers_data[compiler]
        m = min([sample[0] for sample in samples])
        full_plot_data[compiler].append(m)

for compiler in compilers:
    full_plot.add(compiler, full_plot_data[compiler])
full_plot.render_to_png("./build/fullplot.png")

# bar plot
def draw_brp(step, step_n):
    bar_plot = pygal.Bar(y_title="Time, ms")
    bar_plot.title = "Step: {}".format(step)
    bar_plot.x_labels = [str(n) for (n, _) in expiremental_data]
    profiles = ['omp', 'ocl-wtime', 'ocl-profiling']
    plot_data = {profile: [] for profile in profiles}
    for (_, (_, compilers_data)) in expiremental_data:
        for compiler in compilers:
            sample = min(compilers_data[compiler], key=lambda k: k[0])[2]
            if len(sample) == 3:
                plot_data[profiles[0]].append(sample[step_n])
            else:
                plot_data[profiles[1]].append(sample[step_n * 2])
                plot_data[profiles[2]].append(sample[step_n * 2 + 1])

    for profile in profiles:
        bar_plot.add(profile, plot_data[profile])
    bar_plot.render_to_png("./build/barplot-{}.png".format(step_n))

draw_brp('Map M1', 0)
draw_brp('Map M2', 1)
draw_brp('Merge', 2)

# overhead
steps = ['Map M1', 'Map M2', 'Merge']
over_plot = pygal.Line()
over_plot.title = "OpenCL Overhead"
over_plot.x_labels = [str(n) for (n, _) in expiremental_data]
over_plot_data = {step: [] for step in steps}
for (_, (_, compilers_data)) in expiremental_data:
    sample = min(compilers_data[compilers[1]], key=lambda k: k[0])[2]
    for i in range(0, 3):
        over_plot_data[steps[i]].append(sample[i * 2] / sample[i * 2 + 1])
for step in steps:
    over_plot.add(step, over_plot_data[step])
over_plot.render_to_png("./build/overplot.png")
