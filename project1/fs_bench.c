/*
Simple File System Benchmark
Copyright (C) 1984-2015 Core lab. <djshin.core.kaist.ac.kr>

*/

#define _GNU_SOURCE

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <malloc.h>

#define NUMFILES	1
#define FILESIZE	10 * 1024 *1024  // 1MB

char *dirname;
int req_size;
ssize_t act_size;


void usage(char *prog)
{
	fprintf(stderr, "Usage: %s <working directory> <request size>\n", prog);
	exit(1);
}


long gettimeusec(){
	struct timeval time;
	long usec;

	gettimeofday(&time, NULL);

	usec = (time.tv_sec * 1000000 + time.tv_usec);
	return usec;
}



void file_create(){
	int i = 0;
	int fd = 0;
	char filename[128];


	/* Create phase */
	for (i = 0; i < NUMFILES; i++) {
		snprintf(filename, 128, "%s/file-%d", dirname, i);
		fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if (fd == -1) {
			perror("open");
			exit(1);
		}
		close(fd);
	}
	printf("File Created .. \n");
	return;
}


void file_delete(){
	int i, ret;

	char filename[128];

	/* Unlink phase */
	for (i = 0; i < NUMFILES; i++) {
		snprintf(filename, 128, "%s/file-%d", dirname, i);
		ret = unlink(filename);
		if (ret == -1) {
			perror("unlink");
			exit(1);
		}
	}
	return;
}


void file_write_sequential(){
	int i, j, fd, ret;
	char filename[128];
	ssize_t act_size;


	char *buf = (char *)(memalign((size_t)req_size, (size_t)req_size));

	//buf = (char *)malloc(req_size);
	if (!buf) {
		fprintf(stderr, "ERROR: Failed to allocate buffer");
		exit(1);
	}


	for (i = 0; i < NUMFILES; i++) {
		snprintf(filename, 128, "%s/file-%d", dirname, i);
		fd = open(filename, O_DIRECT | O_WRONLY | O_EXCL, S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		//fd = open(filename,  O_WRONLY |  O_EXCL, S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

		if (fd == -1) {
			perror("open");
			exit(1);
		}
		printf("File Opened Sequential Write .. \n");

		for (j = 0; j < FILESIZE; j += req_size){
			act_size = write(fd, buf, req_size);
			if (act_size == -1) {
				perror("write");
				exit(1);
			}
		}

		close(fd);
	}
	return;
}



void file_read_sequential(){

	/* Your Implementation!! */
	int i, j, fd, ret;
        char filename[128];
        ssize_t act_size;


        char *buf = (char *)(memalign((size_t)req_size, (size_t)req_size));

        //buf = (char *)malloc(req_size);
        if (!buf) {
                fprintf(stderr, "ERROR: Failed to allocate buffer");
                exit(1);
        }


        for (i = 0; i < NUMFILES; i++) {
                snprintf(filename, 128, "%s/file-%d", dirname, i);
                fd = open(filename, O_DIRECT | O_RDONLY | O_EXCL, S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
                //fd = open(filename,  O_WRONLY |  O_EXCL, S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

                if (fd == -1) {
                        perror("open");
                        exit(1);
                }
                printf("File Opened Sequential Read .. \n");

                for (j = 0; j < FILESIZE; j += req_size){
                        act_size = read(fd, buf, req_size);
                        if (act_size == -1) {
                                perror("Read");
                                exit(1);
                        }
                }

                close(fd);
        }

	return;
}


void file_write_random(){

	/* Your Implementation!! */
	int i, j, fd, ret;
        char filename[128];
        ssize_t act_size;


        char *buf = (char *)(memalign((size_t)req_size, (size_t)req_size));

        //buf = (char *)malloc(req_size);
        if (!buf) {
                fprintf(stderr, "ERROR: Failed to allocate buffer");
                exit(1);
        }


        for (i = 0; i < NUMFILES; i++) {
                snprintf(filename, 128, "%s/file-%d", dirname, i);
                fd = open(filename, O_DIRECT | O_WRONLY | O_EXCL, S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
                //fd = open(filename,  O_WRONLY |  O_EXCL, S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

                if (fd == -1) {
                        perror("open");
                        exit(1);
                }
                printf("File Opened Random Write .. \n");

		int count = (int)(FILESIZE / req_size);

		int modulus = count;
		int* parr = (int*)malloc(sizeof(int)*modulus);
		for(j=0;j<modulus;j++)
		{
			*(parr+j) = 0;
		}
		while(count > 0)
		{
			int seekpoint = rand()%modulus;
			
			while(*(parr+seekpoint))
			{
				seekpoint = rand()%modulus;
			}
			*(parr+seekpoint) = 1;
			lseek(fd,req_size*seekpoint,SEEK_SET);
			act_size = write(fd,buf,req_size);
			if(act_size == -1)
			{
				perror("write");
                                exit(1);
                        }
			count--;
                }
		free(parr);
                close(fd);
        }
	return;
}


void file_read_random(){


	/* Your Implementation!! */
	int i, j, fd, ret;
        char filename[128];
        ssize_t act_size;


        char *buf = (char *)(memalign((size_t)req_size, (size_t)req_size));

        //buf = (char *)malloc(req_size);
        if (!buf) {
                fprintf(stderr, "ERROR: Failed to allocate buffer");
                exit(1);
        }


        for (i = 0; i < NUMFILES; i++) {
                snprintf(filename, 128, "%s/file-%d", dirname, i);
                fd = open(filename, O_DIRECT | O_RDONLY | O_EXCL, S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
                //fd = open(filename,  O_WRONLY |  O_EXCL, S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

                if (fd == -1) {
                        perror("open");
                        exit(1);
                }
                printf("File Opened Random Read .. \n");

                int count = (int)(FILESIZE / req_size);

                int modulus = count;
		int* parr = (int*)malloc(sizeof(int)*modulus);
                for(j=0;j<modulus;j++)
                {
                        *(parr+j) = 0;
                }

                while(count > 0)
                {
                        int seekpoint = rand()%modulus;
			while(*(parr+seekpoint))
                        {
                                seekpoint = rand()%modulus;
                        }
                        *(parr+seekpoint) = 1;

                        lseek(fd,req_size*seekpoint,SEEK_SET);
                        act_size = read(fd,buf,req_size);
                        if(act_size == -1)
                        {
                                perror("read");
                                exit(1);
                        }
                        count--;
                }
		free(parr);
                close(fd);
        }
	return;
}



int main(int argc, char **argv)
{
	long creat_time_sequential, write_time_sequential, read_time_sequential, delete_time_sequential, end_time_sequential;
	long creat_time_random, write_time_random, read_time_random, delete_time_random, end_time_random;

	if (argc != 3) {
		usage(argv[0]);
	}
	dirname = argv[1];
	req_size = atoi(argv[2]);

	// Sequential Access Test
	creat_time_sequential = gettimeusec();
	file_create();


	write_time_sequential = gettimeusec();
	file_write_sequential();


	read_time_sequential = gettimeusec();
	file_read_sequential();



	// Random Access Test

	srand(30);

	write_time_random = gettimeusec();
	file_write_random();


	read_time_random = gettimeusec();
	file_read_random();


	delete_time_random = gettimeusec();
	file_delete();

	end_time_random = gettimeusec();




	printf("==============  File System Benchmark Execution Result (Time usec)  ==============\n");
	printf("File Create\t : \t%10ld\n", write_time_sequential - creat_time_sequential);
	printf("Sequential Write : \t%10ld\n", read_time_sequential - write_time_sequential);
	printf("Sequential Read\t : \t%10ld\n", write_time_random - read_time_sequential);
	printf("Random Write\t : \t%10ld\n", read_time_random - write_time_random);
	printf("Random Read\t : \t%10ld\n", delete_time_random - read_time_random);
	printf("File Delete\t : \t%10ld\n", end_time_random - delete_time_random);
	printf("Total\t\t : \t%10ld\n", end_time_random - creat_time_sequential);

	printf("==================================================================================\n");

	return 0;
}
