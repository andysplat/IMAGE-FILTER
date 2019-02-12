#include<mpi.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	
	unsigned char r;
	unsigned char g;
	unsigned char b;

}rgb;

typedef struct {
	
	char colored;
	unsigned char ** incolor;
	rgb ** color;
	int width;
	int height;
	unsigned char maxval;

}image;

//DO NOT MODIFY THIS FILE
float K[5][3][3] = {
	{{1.f/9, 1.f/9, 1.f/9},
    {1.f/9, 1.f/9, 1.f/9},
    {1.f/9, 1.f/9, 1.f/9}},
	{{1.f/16, 1.f/8, 1.f/16},
    {1.f/8, 1.f/4, 1.f/8},
    {1.f/16, 1.f/8, 1.f/16}},
	{{0.f, -2.f/3, 0.f},
    {-2.f/3, 11.f/3, -2.f/3},
    {0.f, -2.f/3, 0.f}},
	{{-1.f, -1.f, -1.f},
    {-1.f, 9.f, -1.f},
    {-1.f, -1.f, -1.f}},
	{{0.f, 1.f, 0.f},
    {0.f, 0.f, 0.f},
    {0.f, -1.f, 0.f}}
};

int F[10] = {1, 0, 2, 4, 3, 1, 0, 2, 4, 3};

void readInput(const char * fileName, image *img) {
	int o;
	char colored[2];
	FILE * f = fopen(fileName, "r");
	fscanf(f, "%s\n", colored);
	if(strcmp(colored, "P5") == 0)
		img->colored = 0;
	if(strcmp(colored, "P6") == 0)
		img->colored = 1;
	fscanf(f, "%d %d\n", &img->width, &img->height);
	fscanf(f, "%d\n", &img->maxval);
	if(img->colored) {
		img->color = (rgb**) malloc(img->height * sizeof(rgb*));
		for(o = 0; o < img->height; o++) {
			img->color[o] = (rgb*) malloc(img->width * sizeof(rgb));
			fread(img->color[o], 3, img->width, f);
		}
	}
	else {
		img->incolor = (unsigned char**) calloc(img->height, sizeof(unsigned char*));
		for(o = 0; o < img->height; o++) {
			img->incolor[o] = (unsigned char*) calloc(img->width, sizeof(unsigned char));
			fread(img->incolor[o], 1, img->width, f);
		}
	}
	fclose(f);
}

void writeData(const char * fileName, image *img) {
	int o;
	FILE * f = fopen(fileName, "w");
	if(img->colored) {
		fprintf(f, "%s\n%d %d\n%d\n", "P6", img->width, img->height, img->maxval);
		for(o = 0; o < img->height; o++)
			fwrite(img->color[o], 3, img->width, f);
	}
        else {

		fprintf(f, "%s\n%d %d\n%d\n", "P5", img->width, img->height, img->maxval);
		for(o = 0; o < img->height; o++)
			fwrite(img->incolor[o], sizeof(unsigned char), img->width, f);

	}
	fclose(f);
}

int get_filter(char * filter) {
	if(strcmp(filter, "smooth") == 0)
		return 0;
	if(strcmp(filter, "blur") == 0)
		return 1;
	if(strcmp(filter, "sharpen") == 0)
		return 2;
	if(strcmp(filter, "mean") == 0)
		return 3;
	if(strcmp(filter, "emboss") == 0)
		return 4;
    return -1;
}

void filter(image *in, image *out, int k, int rank, int nProcesses) {

	int o, x, y, z, w;
	out->colored = in->colored;
	out->maxval = in->maxval;
	out->width = in->width;
	out->height = in->height;
	if(out->colored) 
		out->color = (rgb**) malloc(out->height * sizeof(rgb*));
	else 
		out->incolor = (unsigned char**) malloc(out->height * sizeof(unsigned char*));

	for(o = 0; o < out->height; o++)
		if(out->colored) 
			out->color[o] = (rgb*) malloc(out->width * sizeof(rgb));
		else 
			out->incolor[o] = (unsigned char*) malloc(out->width * sizeof(unsigned char));
	for(x = rank; x < out->height; x += nProcesses)
		for(y = 0; y < out->width; y++)
			if(out->colored) {
				if((x != 0) && (y != 0) && (x != out->height - 1) && (y != out->width - 1)) {	
					float r = 0.f;
					float g = 0.f;
					float b = 0.f;
					for(z = x - 1; z <= x + 1; z++)
						for(w = y - 1; w <= y + 1; w++) {
							r += in->color[z][w].r * K[k][z - x + 1][w - y + 1];
							g += in->color[z][w].g * K[k][z - x + 1][w - y + 1];
							b += in->color[z][w].b * K[k][z - x + 1][w - y + 1];
						}
					out->color[x][y].r = (unsigned char) r;
					out->color[x][y].g = (unsigned char) g;
					out->color[x][y].b = (unsigned char) b;
				}
				else {
					out->color[x][y].r = in->color[x][y].r;
					out->color[x][y].g = in->color[x][y].g;
					out->color[x][y].b = in->color[x][y].b;
				}
			}
			else {
				if((x != 0) && (y != 0) && (x != out->height - 1) && (y != out->width - 1)) {	
					float incolor = 0.f;
					for(z = x - 1; z <= x + 1; z++)
						for(w = y - 1; w <= y + 1; w++)
							incolor += in->incolor[z][w] * K[k][z - x + 1][w - y + 1];
					out->incolor[x][y] = (unsigned char) incolor;
				}
				else
					out->incolor[x][y] = in->incolor[x][y];
			}
}

int main(int argc, char * argv[]) {
	//agrv[1] input
	//argv[2] output
	if(argc < 4) {
		exit(-1);
	}
	int rank;
	int nProcesses;
	int o, k, f = 0, t;
//DO NOT MODIFY THIS FILE
//DO NOT MODIFY THIS FILE
	image input;
	image aux;
	image output;
//DO NOT MODIFY THIS FILE
	readInput(argv[1], &input);
//DO NOT MODIFY THIS FILE
	MPI_Init(&argc, &argv);
	MPI_Status status;
	MPI_Request request;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);
	struct timespec start, finish;
	double elapsed;
//DO NOT MODIFY THIS FILE
	clock_gettime(CLOCK_MONOTONIC, &start);
	if(argc == 4)
		if(strcmp(argv[3], "bssembssem") == 0) {
			argc = 13;
			f = 1;
		}
	for(o = 3; o < argc; o++) {
		if(f == 1)
			k = F[o - 3];
		else
			k = get_filter(argv[o]);
		if(o == 3) {
			filter(&input, &aux, k, rank, nProcesses);
				if(rank != 0) {
					for(t = rank; t < aux.height; t += nProcesses)
						if(aux.colored)
							MPI_Send(aux.color[t], 3 * aux.width, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
						else 
							MPI_Send(aux.incolor[t], aux.width, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
				}	
				else {
					for(k = 1; k < nProcesses; k++)
						if(aux.colored)
							for(t = k; t < aux.height; t += nProcesses)
								MPI_Recv(aux.color[t], 3 * aux.width, MPI_CHAR, k, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
						else
							for(t = k; t < aux.height; t += nProcesses)
								MPI_Recv(aux.incolor[t], aux.width, MPI_CHAR, k, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				}
		}
		else
			if(o % 2 == 0) {
				filter(&aux, &output, k, rank, nProcesses);
				if(rank != 0) {
					for(t = rank; t < output.height; t += nProcesses)
						if(output.colored)
							MPI_Send(output.color[t], 3 * output.width, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
						else 
						MPI_Send(output.incolor[t], output.width, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
				}	
				else {
					for(k = 1; k < nProcesses; k++)
						if(output.colored)
							for(t = k; t < output.height; t += nProcesses)
								MPI_Recv(output.color[t], 3 * output.width, MPI_CHAR, k, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
						else
							for(t = k; t < output.height; t += nProcesses)
								MPI_Recv(output.incolor[t], output.width, MPI_CHAR, k, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				}
			}
			else {
				filter(&output, &aux, k, rank, nProcesses);
				if(rank != 0) {
					for(t = rank; t < aux.height; t += nProcesses)
						if(aux.colored)
							MPI_Send(aux.color[t], 3 * aux.width, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
						else 
							MPI_Send(aux.incolor[t], aux.width, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
				}	
				else {
					for(k = 1; k < nProcesses; k++)
						if(aux.colored)
							for(t = k; t < aux.height; t += nProcesses)
								MPI_Recv(aux.color[t], 3 * aux.width, MPI_CHAR, k, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
						else
							for(t = k; t < aux.height; t += nProcesses)
								MPI_Recv(aux.incolor[t], aux.width, MPI_CHAR, k, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				}
			}
		if(rank == 0) {
			if(o % 2 == 0) {
				for(k = 1; k < nProcesses; k++)
						if(output.colored)
							for(t = 0; t < output.height; t++)
								MPI_Send(output.color[t], 3 * output.width, MPI_CHAR, k, 0, MPI_COMM_WORLD);
						else
							for(t = 0; t < output.height; t++)
								MPI_Send(output.incolor[t], output.width, MPI_CHAR, k, 0, MPI_COMM_WORLD);
			}	
			else {
			for(k = 1; k < nProcesses; k++)
						if(aux.colored)
							for(t = 0; t < aux.height; t++)
								MPI_Send(aux.color[t], 3 * aux.width, MPI_CHAR, k, 0, MPI_COMM_WORLD);
						else
							for(t = 0; t < aux.height; t++)
								MPI_Send(aux.incolor[t], aux.width, MPI_CHAR, k, 0, MPI_COMM_WORLD);
			}	
		}
		else {
			if(o % 2 == 0) {
				for(t = 0; t < output.height; t++)
					if(output.colored)
						MPI_Recv(output.color[t], 3 * output.width, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					else
						MPI_Recv(output.incolor[t], output.width, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			}	
			else {
			for(t = 0; t < aux.height; t++)
				if(aux.colored)
					MPI_Recv(aux.color[t], 3 * aux.width, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				else
					MPI_Recv(aux.incolor[t], aux.width, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			}
		}			
	}
	if(rank == 0) {
		if(o % 2 == 0)
			output = aux;
		writeData(argv[2], &output);
	}	
	clock_gettime(CLOCK_MONOTONIC, &finish);	
//DO NOT MODIFY THIS FILE
	elapsed = (finish.tv_sec - start.tv_sec);
	elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
//DO NOT MODIFY THIS FILE
//DO NOT MODIFY THIS FILE
	MPI_Finalize();
	return 0;
}
