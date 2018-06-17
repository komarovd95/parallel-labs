#define EULER 2.718281828

__kernel void map_m1(__global double *m1) {
	int i;
	i = get_global_id(0);
	m1[i] = sqrt(m1[i] / EULER);
}

__kernel void map_m2_copy(__global double *m2, __global double *tmp) {
	int i;
	i = get_global_id(0);
	tmp[i] = m2[i];
}

__kernel void map_m2_sum(__global double *m2, __global double *tmp) {
	int i;
	i = get_global_id(0);
	if (i) {
		m2[i] += tmp[i - 1];
	}
}

__kernel void map_m2(__global double *m2) {
	int i;
	i = get_global_id(0);
	m2[i] = fabs(tan(m2[i]));
}

__kernel void merge(__global double *m1, __global double *m2) {
	int i;
	i = get_global_id(0);
	m2[i] *= m1[i];
}
