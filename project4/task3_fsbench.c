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
#define FILESIZE	10 * 1024 *1024  // 10MB

char *dirname;
int req_size;
ssize_t act_size;

/* Print usage including arguments */
void usage(char *prog)
{
	fprintf(stderr, "Usage: %s <working directory> <request size>\n", prog);
	exit(1);
}


/* return current time using a usec unit */
long gettimeusec(){
	struct timeval time;
	long usec; 
	gettimeofday(&time, NULL);
	usec = (time.tv_sec * 1000000 + time.tv_usec);
	return usec;
}


/* Create #NUMFILES of files for I/O benchmarking */
void file_create(){
	int i = 0;
	int fd = 0;
	char filename[128];				//variable for file name


	/* Create phase */
	for (i = 0; i < NUMFILES; i++) {
		snprintf(filename, 128, "%s/file-%d", dirname,  i);				// create file path and file name
		fd = open(filename, O_WRONLY | O_CREAT | O_EXCL , S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);	// file open 
		if (fd == -1) {
			perror("open");
			exit(1);
		}
		close(fd);
	}
	printf ("File Created .. \n");
	return;
}


/* delete files */
void file_delete(){
	int i,  ret;

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

/* sequential write test */
void file_write_sequential(){
	int i, j, fd, ret;
	char filename[128];
	ssize_t act_size;
	

	char *buf  = (char *) (memalign((size_t)req_size,(size_t) req_size));

	//buf = (char *)malloc(req_size);
	if (!buf) {
		fprintf(stderr, "ERROR: Failed to allocate buffer");
		exit(1);
	}

	
	printf ("File Opened Sequential Write .. \n");
	for (i = 0; i < NUMFILES; i++) {
		snprintf(filename, 128, "%s/file-%d", dirname,  i);
		fd = open(filename, O_WRONLY |  O_EXCL, S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		// O_DIRECT flag is used to bypass buffer cache				

		if (fd == -1) {
			perror("open");
			exit(1);
		}
		

		// Total written size is FILESIZE
		for( j = 0; j < FILESIZE; j += req_size){
			act_size = write(fd, buf, req_size);
			if (act_size == -1) {
				perror("write");
				exit(1);
			}
		}

		int a = close(fd);
	}
	return;
}


/* sequential read test */
void file_read_sequential(){
	int i, j, fd, ret;
	char filename[128];
	ssize_t act_size;
	char *buf  = (char *) (memalign((size_t)req_size,(size_t) req_size));

	
	printf ("File Opened Sequential Read .. \n");
	for (i = 0; i < NUMFILES; i++) {
		snprintf(filename, 128, "%s/file-%d", dirname, i);
		fd = open(filename, O_RDONLY);
		//fd = open(filename, O_RDONLY);

		if (fd == -1) {
			perror("open");
			exit(1);
		}
		
		
	
		// Total read size is FILESIZE
		for ( j = 0; j < FILESIZE; j += req_size ){
			act_size = read(fd, buf, req_size);
			if (act_size == -1) {
				perror("read");
				exit(1);
			}
		}

		close(fd);
	}
	return;
}


/* random write test using a unit of req_size */
void file_write_random(){
	int i, j, fd, ret;
	char filename[128];
	ssize_t act_size;
	int rand_num;
	
	// Temporary buffer area. Memalign is used due to aligned I/O for O_DIRECT ed operation. 
	char *buf  = (char *) (memalign((size_t)req_size,(size_t) req_size));//(char *) (calloc(1,(size_t)(req_size + 1)));		
	

	if (!buf) {
		fprintf(stderr, "ERROR: Failed to allocate buffer");
		exit(1);
	}

	printf ("File Opened Random Write .. \n");
	
	for (i = 0; i < NUMFILES; i++) {
		snprintf(filename, 128, "%s/file-%d", dirname,  i);
		fd = open(filename, O_WRONLY | O_EXCL , S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH); //O_WRONLY | O_EXCL 
		// O_DIRECT flag is used to bypass buffer cache	
		
		if (fd == -1) {
			perror("open");
			exit(1);
		}

		for( j = 0; j < FILESIZE; j += req_size){
			// FILESIZE is devided to the unit of req_size. 
			// random block number 0 to (FILESIZE/req_size -1)
			rand_num = rand() % ( (int) (FILESIZE/req_size) );	
			// Seeking should be the unit of Byte. So (block number * req_size) is used.
			lseek( fd, req_size * rand_num, SEEK_SET);
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

/* random read test using a unit of req_size */
void file_read_random(){
	int i, j, fd, ret;
	char filename[128];
	ssize_t act_size;
	int rand_num;

	// Temporary buffer area. Memalign is used due to aligned I/O for O_DIRECT ed operation. 
	char *buf  = (char *) (memalign((size_t)req_size,(size_t) req_size));

	
	printf ("File Opened Random Read .. \n");
	for (i = 0; i < NUMFILES; i++) {
		snprintf(filename, 128, "%s/file-%d", dirname, i);
		fd = open(filename, O_RDONLY);  
		// O_DIRECT flag is used to bypass buffer cache	

		if (fd == -1) {
			perror("open");
			exit(1);
		}
		
		
		for ( j = 0; j < FILESIZE; j += req_size ){
			// FILESIZE is devided to the unit of req_size. 
			// random block number 0 to (FILESIZE/req_size -1)
			rand_num = rand() % ( (int) (FILESIZE/req_size) );
			// Seeking should be the unit of Byte. So (block number * req_size) is used.
			lseek( fd, req_size * rand_num, SEEK_SET);
			act_size = read(fd, buf, req_size);
			if (act_size == -1) {
				perror("read");
				exit(1);
			}
		}

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
	req_size = atoi( argv[2] );

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

	end_time_random=gettimeusec();


	printf("==============  File System Benchmark Execution Result (Time usec)  ==============\n");
	printf("File Create\t : \t%10ld\n",write_time_sequential-creat_time_sequential);
	printf("Sequential Write : \t%10ld\n",read_time_sequential-write_time_sequential);
	printf("Sequential Read\t : \t%10ld\n",write_time_random-read_time_sequential);
	printf("Random Write\t : \t%10ld\n",read_time_random - write_time_random);
	printf("Random Read\t : \t%10ld\n",delete_time_random-read_time_random);
	printf("File Delete\t : \t%10ld\n",end_time_random-delete_time_random);
	printf("Total\t\t : \t%10ld\n",end_time_random - creat_time_sequential);

	printf("==================================================================================\n");


}
