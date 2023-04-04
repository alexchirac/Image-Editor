/* Copyright CHIRAC ALEXANDRU-STEFAN 313CAb 2022 - 2023 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

typedef struct image {
	int type;
	int max1;
	int n, m, i0, j0, i1, j1;
	int **v;
} IMAGE;

typedef struct command {
	char param1[50], param2[50];
	int type, p[4];
	int ok, ok1;
} COMMAND;

int clamp(int x)
{
	if (x > 255)
		return 255;
	if (x < 0)
		return 0;
	return x;
}

void jump(FILE *in)
{
	char c;
	while (fscanf(in, "%c", &c))
		if (c == '\n')
			break;
}

int check(int x1, int y1, int x2, int y2, IMAGE img)
{
	if (x1 < 0 || y1 < 0 || x2 < 0 || y2 < 0)
		return 0;
	if (img.type == 2 || img.type == 5) {
		if (x1 > img.m || x2 > img.m)
			return 0;
	} else {
		if (x1 * 3 > img.m || x2 * 3 > img.m)
			return 0;
	}
	if (y1 > img.n || y2 > img.n)
		return 0;
	if (x1 == x2 || y1 == y2)
		return 0;
	return 1;
}

void arrange(int *x1, int *x2)
{
	int aux;
	if (*x1 > *x2) {
		aux = *x1;
		*x1 = *x2;
		*x2 = aux;
	}
}

void unload(IMAGE *img)
{
	int i;
	for (i = 0; i < img->n; i++)
		free(img->v[i]);
	free(img->v);
}

void read_matrix_ascii(FILE *in, int n, int m, int **v)
{
	int i, j;
	char s[50];
	for (i = 0; i < n; i++)
		for (j = 0; j < m; j++) {
			fscanf(in, "%s", s);
			v[i][j] = atoi(s);
		}
}

void read_matrix_binary(FILE *in, int n, int m, int **v, int type)
{
	int i, j;
	if (type == 6) {
		m = m / 3;
		unsigned char *bytes = calloc(n * m * 3, sizeof(unsigned char));
		fseek(in, 1, SEEK_CUR);
		fread(bytes, 1, n * m * 3, in);
		for (i = 0; i < n; i++)
			for (j = 0; j < m; j++) {
				v[i][j * 3] = bytes[i * m * 3 + j * 3];
				v[i][j * 3 + 1] = bytes[i * m * 3 + j * 3 + 1];
				v[i][j * 3 + 2] = bytes[i * m * 3 + j * 3 + 2];
			}
		free(bytes);
		return;
	}
	unsigned char *bytes = calloc(n * m, sizeof(unsigned char));
	fseek(in, 1, SEEK_CUR);
	fread(bytes, 1, n * m, in);
	for (i = 0; i < n; i++)
		for (j = 0; j < m; j++)
			v[i][j] = bytes[i * m + j];
	free(bytes);
}

void load_file(IMAGE *img, int *ok, COMMAND com)
{
	char c, s[50];
	int nr = 0, i;
	FILE *in = fopen(com.param1, "r");
	if (!in) {
		printf("Failed to load %s\n", com.param1);
		if (*ok == 1)
			unload(img);
		*ok = 0;
		return;
	}
	if (*ok == 1)
		unload(img);
	else
		*ok = 1;
	fscanf(in, "%c", &c);
	fscanf(in, "%c", &c);
	img->type = atoi(&c);
	while (fscanf(in, "%s", s)) {
		if (s[0] == '#') {
			jump(in);
		} else {
			if (nr == 1) {
				img->max1 = atoi(s);
				break;
			}
			if (nr == 0) {
				nr++;
				img->m = atoi(s);
				fscanf(in, "%s", s);
				img->n = atoi(s);
			}
		}
	}
	if (img->type == 3 || img->type == 6)
		img->m = img->m * 3;
	img->v = malloc(img->n * sizeof(int *));
	for (i = 0; i < img->n; i++)
		img->v[i] = malloc(img->m * sizeof(int));
	if (img->type == 2 || img->type == 3)
		read_matrix_ascii(in, img->n, img->m, img->v);
	else
		read_matrix_binary(in, img->n, img->m, img->v, img->type);
	printf("Loaded %s\n", com.param1);
	img->i0 = 0;
	img->j0 = 0;
	img->i1 = img->n;
	img->j1 = img->m;
	fclose(in);
}

void imgselect(IMAGE *img, COMMAND com)
{
	int x1, x2, y1, y2, ok;
	if (strcmp(com.param1, "ALL") == 0) {
		img->i0 = 0;
		img->j0 = 0;
		img->i1 = img->n;
		img->j1 = img->m;
		printf("Selected ALL\n");
	} else {
		x1 = com.p[0];
		y1 = com.p[1];
		x2 = com.p[2];
		y2 = com.p[3];
		ok = check(x1, y1, x2, y2, *img);
		if (ok == 1 && com.ok) {
			arrange(&x1, &x2);
			arrange(&y1, &y2);
			printf("Selected %d %d %d %d\n", x1, y1, x2, y2);
			img->j0 = x1;
			img->j1 = x2;
			img->i0 = y1;
			img->i1 = y2;
			if (img->type == 3 || img->type == 6) {
				img->j0 = img->j0 * 3;
				img->j1 = img->j1 * 3;
			}
		} else {
			printf("Invalid set of coordinates\n");
		}
	}
}

void histogram(IMAGE img, COMMAND com)
{
	int x, y, *freq, i, j, val, max1 = -1, nr;
	x = com.p[0];
	y = com.p[1];
	if (img.type == 3 || img.type == 6) {
		printf("Black and white image needed\n");
	} else {
		freq = malloc(y * sizeof(int));
		for (i = 0; i < y; i++)
			freq[i] = 0;
		for (i = 0; i < img.n; i++)
			for (j = 0; j < img.m; j++) {
				val = img.v[i][j] / (img.max1 / y + 1);
				freq[val]++;
			}
		for (i = 0; i < y; i++)
			if (freq[i] > max1)
				max1 = freq[i];
		for (i = 0; i < y; i++) {
			nr = freq[i] * x / max1;
			printf("%d\t|\t", nr);
			for (j = 0; j < nr; j++)
				printf("*");
			printf("\n");
		}
		free(freq);
	}
}

int func(int sum, int area)
{
	double val;
	val = 255.0 * (double)sum / (double)area;
	return clamp(round(val));
}

void equalize(IMAGE *img)
{
	int i, j, *freq, area, *sum;
	if (img->type == 3 || img->type == 6) {
		printf("Black and white image needed\n");
	} else {
		freq = malloc(256 * sizeof(int));
		sum = malloc(256 * sizeof(int));
		for (i = 0; i < 256; i++)
			freq[i] = 0;
		for (i = 0; i < img->n; i++)
			for (j = 0; j < img->m; j++)
				freq[img->v[i][j]]++;
		area = img->n * img->m;
		sum[0] = freq[0];
		for (i = 1; i < 256; i++)
			sum[i] = sum[i - 1] + freq[i];
		for (i = 0; i < img->n; i++)
			for (j = 0; j < img->m; j++)
				img->v[i][j] = func(sum[img->v[i][j]], area);
		free(freq);
		free(sum);
		printf("Equalize done\n");
	}
}

void crop(IMAGE *img)
{
	int i, j, **aux;
	int nrl = img->i1 - img->i0;
	int nrc = img->j1 - img->j0;
	aux = malloc((nrl) * sizeof(int *));
	for (i = 0; i < nrl; i++)
		aux[i] = malloc((nrc) * sizeof(int));
	for (i = 0; i < nrl; i++)
		for (j = 0; j < nrc; j++)
			aux[i][j] = img->v[i + img->i0][j + img->j0];
	for (i = 0; i < img->n; i++)
		free(img->v[i]);
	free(img->v);
	img->v = aux;
	img->i0 = 0;
	img->j0 = 0;
	img->i1 = nrl;
	img->j1 = nrc;
	img->n = nrl;
	img->m = nrc;
	printf("Image cropped\n");
}

void free_matrix(int **aux, int n)
{
	for (int i = 0; i < n; i++)
		free(aux[i]);
	free(aux);
}

int **make_aux(IMAGE img, int v[][3])
{
	int **aux, i, j, k, l, s;
	int nrl = img.i1 - img.i0;
	int nrc = img.j1 - img.j0;
	aux = malloc(nrl * sizeof(int *));
	for (i = 0; i < nrl; i++)
		aux[i] = malloc(nrc * sizeof(int));
	for (i = img.i0; i < img.i1; i++)
		for (j = img.j0; j < img.j1; j++) {
			if (i * (j / 3) != 0 && i != img.n - 1 && j / 3 != img.m / 3 - 1) {
				s = 0;
				for (k = 0; k < 3; k++)
					for (l = 0; l < 3; l++)
						s = s + v[k][l] * img.v[i - 1 + k][j - 3 + l * 3];
				aux[i - img.i0][j - img.j0] = s;
			} else {
				aux[i - img.i0][j - img.j0] = img.v[i][j];
			}
		}
	return aux;
}

void apply(IMAGE *img, COMMAND com)
{
	char s[50];
	int **aux, i, j;
	if (img->type == 2 || img->type == 5) {
		printf("Easy, Charlie Chaplin\n");
		return;
	}
	strcpy(s, com.param1);
	if (strcmp(s, "EDGE") == 0) {
		int v[3][3] = { {-1, -1, -1}, {-1, 8, -1}, {-1, -1, -1} };
		aux = make_aux(*img, v);
		for (i = img->i0; i < img->i1; i++)
			for (j = img->j0; j < img->j1; j++)
				img->v[i][j] = clamp(aux[i - img->i0][j - img->j0]);
		free_matrix(aux, img->i1 - img->i0);
		printf("APPLY EDGE done\n");
		return;
	}
	if (strcmp(s, "SHARPEN") == 0) {
		int v[3][3] = { {0, -1, 0}, {-1, 5, -1}, {0, -1, 0} };
		aux = make_aux(*img, v);
		for (i = img->i0; i < img->i1; i++)
			for (j = img->j0; j < img->j1; j++)
				img->v[i][j] = clamp(aux[i - img->i0][j - img->j0]);
		free_matrix(aux, img->i1 - img->i0);
		printf("APPLY SHARPEN done\n");
		return;
	}
	if (strcmp(s, "BLUR") == 0) {
		int v[3][3] = { {1, 1, 1}, {1, 1, 1}, {1, 1, 1} };
		aux = make_aux(*img, v);
		int nr1 = img->n - 1;
		int nr2 = img->m / 3 - 1;
		for (i = img->i0; i < img->i1; i++)
			for (j = img->j0; j < img->j1; j++)
				if (i * (j / 3) != 0 && i != nr1 && j / 3 != nr2)
					img->v[i][j] = aux[i - img->i0][j - img->j0] / 9;
				else
					img->v[i][j] = aux[i - img->i0][j - img->j0];
		free_matrix(aux, img->i1 - img->i0);
		printf("APPLY BLUR done\n");
		return;
	}
	if (strcmp(s, "GAUSSIAN_BLUR") == 0) {
		int v[3][3] = { {1, 2, 1}, {2, 4, 2}, {1, 2, 1} };
		aux = make_aux(*img, v);
		int nr1 = img->n - 1;
		int nr2 = img->m / 3 - 1;
		for (i = img->i0; i < img->i1; i++)
			for (j = img->j0; j < img->j1; j++)
				if (i * (j / 3) != 0 && i != nr1 && j / 3 != nr2)
					img->v[i][j] = aux[i - img->i0][j - img->j0] / 16;
				else
					img->v[i][j] = aux[i - img->i0][j - img->j0];
		free_matrix(aux, img->i1 - img->i0);
		printf("APPLY GAUSSIAN_BLUR done\n");
		return;
	}
	printf("APPLY parameter invalid\n");
}

int valid(int x)
{
	if (x % 90 != 0)
		return 0;
	if (x < -360 || x > 360)
		return 0;
	return 1;
}

void switch_val(int *x, int *y)
{
	int aux;
	aux = *x;
	*x = *y;
	*y = aux;
}

void rotate_selection_90(IMAGE *img)
{
	int **aux, i, j, sti, stj;
	int nrl = img->i1 - img->i0;
	int nrc = img->j1 - img->j0;
	if (img->type == 2 || img->type == 5) {
		aux = malloc(nrl * sizeof(int *));
		for (i = 0; i < nrl; i++)
			aux[i] = malloc(nrc * sizeof(int));
		for (i = 0; i < nrl; i++)
			for (j = 0; j < nrc; j++)
				aux[i][j] = img->v[img->i1 - 1 - j][img->j0 + i];
		for (i = 0; i < nrl; i++)
			for (j = 0; j < nrc; j++)
				img->v[img->i0 + i][img->j0 + j] = aux[i][j];
		for (i = 0; i < nrl; i++)
			free(aux[i]);
		free(aux);
	} else {
		aux = malloc(nrl * sizeof(int *));
		for (i = 0; i < nrl; i++)
			aux[i] = malloc(nrc * sizeof(int));
		for (i = 0; i < nrl; i++)
			for (j = 0; j < nrc / 3; j++) {
				sti = img->i1 - 1 - j;
				stj = img->j0 + i * 3;
				aux[i][j * 3] = img->v[sti][stj];
				aux[i][j * 3 + 1] = img->v[sti][stj + 1];
				aux[i][j * 3 + 2] = img->v[sti][stj + 2];
			}
		for (i = 0; i < nrl; i++)
			for (j = 0; j < nrc; j++)
				img->v[img->i0 + i][img->j0 + j] = aux[i][j];
		for (i = 0; i < nrl; i++)
			free(aux[i]);
		free(aux);
	}
}

void rotate_image_90(IMAGE *img)
{
	int **aux, i, j;
	if (img->type == 2 || img->type == 5) {
		switch_val(&img->m, &img->n);
		switch_val(&img->i0, &img->j0);
		switch_val(&img->i1, &img->j1);
		aux = malloc(img->n * sizeof(int *));
		for (i = 0; i < img->n; i++)
			aux[i] = malloc(img->m * sizeof(int));
		for (i = 0; i < img->n; i++)
			for (j = 0; j < img->m; j++)
				aux[i][j] = img->v[img->m - 1 - j][i];
		for (i = 0; i < img->m; i++)
			free(img->v[i]);
		free(img->v);
		img->v = aux;
	} else {
		switch_val(&img->m, &img->n);
		switch_val(&img->i0, &img->j0);
		switch_val(&img->i1, &img->j1);
		img->m = img->m * 3;
		img->n = img->n / 3;
		img->i0 = img->i0 / 3;
		img->i1 = img->i1 / 3;
		img->j0 = img->j0 * 3;
		img->j1 = img->j1 * 3;
		int m1 = img->m / 3;
		aux = malloc(img->n * sizeof(int *));
		for (i = 0; i < img->n; i++)
			aux[i] = malloc(img->m * sizeof(int));
		for (i = 0; i < img->n; i++)
			for (j = 0; j < m1; j++) {
				aux[i][j * 3] = img->v[m1 - 1 - j][i * 3];
				aux[i][j * 3 + 1] = img->v[m1 - 1 - j][i * 3 + 1];
				aux[i][j * 3 + 2] = img->v[m1 - 1 - j][i * 3 + 2];
			}
		for (i = 0; i < img->m / 3; i++)
			free(img->v[i]);
		free(img->v);
		img->v = aux;
	}
}

void rotate(IMAGE *img, COMMAND com)
{
	int angle, true_angle, nrrot;
	angle = com.p[0];
	int nrl = img->i1 - img->i0;
	int nrc = img->j1 - img->j0;
	if (valid(angle) == 0) {
		printf("Unsupported rotation angle\n");
		return;
	}
	if (img->type == 2 || img->type == 5) {
		if (nrl != nrc && (nrl != img->n || nrc != img->m)) {
			printf("The selection must be square\n");
			return;
		}
	} else {
		if (nrl != nrc / 3 && (nrl != img->n || nrc != img->m)) {
			printf("The selection must be square\n");
			return;
		}
	}

	if (angle < 0)
		true_angle = 360 + angle;
	else
		true_angle = angle;
	nrrot = true_angle / 90;
	if (nrl != img->n || nrc != img->m)
		for (int i = 0; i < nrrot; i++)
			rotate_selection_90(img);
	else
		for (int i = 0; i < nrrot; i++)
			rotate_image_90(img);

	printf("Rotated %d\n", angle);
}

void save(IMAGE img, COMMAND com)
{
	char name[50];
	int i, j, m1, type;
	if (img.type == 3 || img.type == 6)
		m1 = img.m / 3;
	else
		m1 = img.m;
	strcpy(name, com.param1);
	if (com.ok == 1) {
		FILE *out = fopen(name, "w");
		if (img.type == 5 || img.type == 6)
			type = img.type - 3;
		else
			type = img.type;
		fprintf(out, "P%d\n", type);
		fprintf(out, "%d %d\n", m1, img.n);
		fprintf(out, "%d\n", img.max1);
		for (i = 0; i < img.n; i++) {
			for (j = 0; j < img.m; j++)
				fprintf(out, "%d ", img.v[i][j]);
			fprintf(out, "\n");
		}
		fclose(out);
		printf("Saved %s\n", name);
		return;
	}
	FILE *out = fopen(name, "w");
	if (img.type == 2 || img.type == 3)
		type = img.type + 3;
	else
		type = img.type;
	fprintf(out, "P%d\n", type);
	fprintf(out, "%d %d\n", m1, img.n);
	fprintf(out, "%d\n", img.max1);
	for (i = 0; i < img.n; i++) {
		for (j = 0; j < img.m; j++)
			fputc(img.v[i][j], out);
	}
	fclose(out);
	printf("Saved %s\n", name);
}

void restart_com(COMMAND *com)
{
	com->ok = 0;
	com->ok1 = 0;
	strcpy(com->param1, "");
	strcpy(com->param1, "");
	com->type = 0;
}

int are_nr(char s1[], char s2[], char s3[], char s4[])
{
	int i, l;
	if (s1[0] != '-' && !isdigit(s1[0]))
		return 0;
	l = strlen(s1);
	for (i = 1; i < l; i++)
		if (!isdigit(s1[i]))
			return 0;
	if (s2[0] != '-' && !isdigit(s2[0]))
		return 0;
	l = strlen(s2);
	for (i = 1; i < l; i++)
		if (!isdigit(s2[i]))
			return 0;
	if (s3[0] != '-' && !isdigit(s3[0]))
		return 0;
	l = strlen(s3);
	for (i = 1; i < l; i++)
		if (!isdigit(s3[i]))
			return 0;
	if (s4[0] != '-' && !isdigit(s4[0]))
		return 0;
	l = strlen(s4);
	for (i = 1; i < l; i++)
		if (!isdigit(s4[i]))
			return 0;
	return 1;
}

COMMAND make_com(char line[])
{
	char op[50], s[50], *line1;
	int aux;
	char s1[50], s2[50], s3[50], s4[50];
	COMMAND com;
	restart_com(&com);
	if (sscanf(line, "%s", op) == 0)
		return(com);
	line1 = strtok(line, " "); line1 = strtok(NULL, "");
	if (strcmp(op, "LOAD") == 0) {
		com.type = 1;
		if (sscanf(line1, "%s %s", com.param1, s) == 1)
			return com;
	}
	if (strcmp(op, "SELECT") == 0) {
		if (!line1) {
			com.type = 0; return com;
		}
		com.type = 2;
		if (sscanf(line1, "%s %s", com.param1, s) == 1)
			if (strcmp(com.param1, "ALL") == 0)
				return com;
		if (sscanf(line1, "%s %s %s %s %s", s1, s2, s3, s4, s) == 4) {
			com.ok = are_nr(s1, s2, s3, s4);
			if (com.ok) {
				com.p[0] = atoi(s1); com.p[1] = atoi(s2);
				com.p[2] = atoi(s3); com.p[3] = atoi(s4);
			} else {
				com.type = 0;
			}
			return com;
		}
		com.type = 0; return com;
	}
	if (strcmp(op, "HISTOGRAM") == 0) {
		com.type = 3;
		if (!line1) {
			com.ok1 = 1;
			return com;
		}
		if (sscanf(line1, "%s %s %s", s1, s2, s) == 2) {
			com.p[0] = atoi(s1); com.p[1] = atoi(s2); return com;
		}
		com.type = 0;
		return com;
	}
	if (strcmp(op, "EQUALIZE") == 0) {
		com.type = 4;
		return com;
	}
	if (strcmp(op, "CROP") == 0) {
		com.type = 5; return com;
	}
	if (strcmp(op, "ROTATE") == 0) {
		com.type = 6;
		if (sscanf(line1, "%d %d", &com.p[0], &aux) == 1)
			return com;
	}
	if (strcmp(op, "APPLY") == 0) {
		com.type = 7;
		if (!line1) {
			com.ok1 = 1; return com;
		}
		if (sscanf(line1, "%s %s", com.param1, s) == 1)
			return com;
	}
	if (strcmp(op, "SAVE") == 0) {
		com.type = 8;
		int nr = sscanf(line1, "%s %s %s", com.param1, com.param2, s);
		if (nr == 2)
			if (strncmp(com.param2, "ascii", 5) == 0)
				com.ok = 1;
		if (nr == 1 || nr == 2)
			return com;
	}
	if (strcmp(op, "EXIT") == 0) {
		com.type = 9; return com;
	}
	return com;
}

int main(void)
{
	char line[50];
	IMAGE img;
	int ok = 0;
	COMMAND com;
	while (fgets(line, 50, stdin)) {
		com = make_com(line);
		if (com.type != 1 && ok == 0)
			printf("No image loaded\n");
		if (com.type == 1) {
			load_file(&img, &ok, com);
			continue;
		}
		if (com.type == 2) {
			if (ok == 1)
				imgselect(&img, com);
			continue;
		}
		if (com.type == 3) {
			if (ok == 1) {
				if (com.ok1 == 1) {
					printf("Invalid command\n");
					continue;
				}
				histogram(img, com);
			}
			continue;
		}
		if (com.type == 4) {
			if (ok == 1)
				equalize(&img);
			continue;
		}
		if (com.type == 5) {
			if (ok == 1)
				crop(&img);
			continue;
		}
		if (com.type == 6) {
			if (ok == 1)
				rotate(&img, com);
			continue;
		}
		if (com.type == 7) {
			if (ok == 1) {
				if (com.ok1 == 1) {
					printf("Invalid command\n");
					continue;
				}
				apply(&img, com);
			}
			continue;
		}
		if (com.type == 8) {
			if (ok == 1)
				save(img, com);
			continue;
		}
		if (com.type == 9) {
			if (ok == 1)
				unload(&img);
			break;
		}
		printf("Invalid command\n");
	}

	return 0;
}
