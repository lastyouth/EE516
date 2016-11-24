/*
  Big Brother File System
  Copyright (C) 2012 Joseph J. Pfeiffer, Jr., Ph.D. <pfeiffer@cs.nmsu.edu>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.

  This code is derived from function prototypes found /usr/include/fuse/fuse.h
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  His code is licensed under the LGPLv2.
  A copy of that code is included in the file fuse.h
  
  The point of this FUSE filesystem is to provide an introduction to
  FUSE.  It was my first FUSE filesystem as I got to know the
  software; hopefully, the comments in this code will help people who
  follow later to get a gentler introduction.

  This might be called a no-op filesystem:  it doesn't impose
  filesystem semantics on top of any other existing structure.  It
  simply reports the requests that come in, and passes them to an
  underlying filesystem.  The information is saved in a logfile named
  bbfs.log, in the directory from which you run bbfs.

  gcc -Wall `pkg-config fuse --cflags --libs` -o bbfs bbfs.c
*/

#include "params.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#include "log.h"
#include "list.h"

// sbh added
int encryptEnabled = 0; // which is determined by ee516.conf file
unsigned int forAddition;
unsigned int forShift;
int cachePolicy; // 0: No cache, 1:random eviction, 2:LRU eviction

char configFilePath[BUFSIZ];

// file buffer

#define MAX_CHUNK_SIZE 	4096	// each buffer node has 4KB chunk size.
#define MAX_LIST_SIZE 	1280    // 4096 * 1280 = 5 Mbytes
// cache access would have 4 different results
#define PERFECT_HIT		0x101
#define PARTIAL_HIT		0x102
#define PARTIAL_MISS	0x103
#define PERFECT_MISS	0x104

// represent filebuffer node
struct filebuffer
{
	struct list_head listptr; // used for linkedlist
	int from; // start offset
	int to; // end offset
	int dirty; // dirty flag
	int fd; // related file pointer
	char buf[MAX_CHUNK_SIZE]; // chunk
};

// return value from bufferHitCheck function which designates the cache access result
struct hitinfo
{
	int type; // access result type : one of predefined value(i.e. PERFECT_HIT, PARTIAL_HIT, PARTIAL_MISS, PERFECT_MISS)
	int valid[2]; // valid array which indicates validation of firstpart or secondpart.
	struct filebuffer *firstpart; // cached data
	struct filebuffer *secondpart; // cached data
};

static int bufferCount = 0; // filebuffer size

static struct list_head headptr; // filebuffer head pointer

struct hitinfo bufferHitCheck(int offset, int reqSize, int fd)
{
	/*
	 * this function searches the filebuffer list and 
	 * determines cache hit or miss of current read or write request*/
	struct filebuffer *pos; // each filebuffer
	int currentfrom,currentto; // each filebuffer's offset
	int targetfrom, targetto; // offset of request
	struct hitinfo ret; // return value
	
	ret.type = PERFECT_MISS;
	ret.valid[0] = ret.valid[1] = 0;
	
	
	
	targetfrom = offset;
	targetto = offset+reqSize;
	log_msg("bufferHitCheck start : from %d to %d\n",targetfrom,targetto);
	
	
	list_for_each_entry(pos,&headptr,listptr)
	{
		currentfrom = pos->from;
		currentto = pos->to;
		
		if(pos->fd == fd && currentfrom == targetfrom)
		{
			// perfect hit
			// it is possible because there is an assumption that each request size is 4Kb
			ret.type = PERFECT_HIT;
			
			ret.valid[0] = 1;
			ret.firstpart = pos;
			
			break;
		}
		if(pos->fd == fd && currentfrom < targetfrom && targetfrom < currentto)
		{
			// partial hit candidate
			
			ret.valid[0] = 1;
			ret.firstpart = pos;
		}
		if(pos->fd == fd && currentfrom < targetto && targetto < currentto)
		{

			ret.valid[1] = 1;
			ret.secondpart = pos;
		}
	}
	if(ret.type == PERFECT_HIT)
	{
		list_move(&(ret.firstpart->listptr),&headptr); // for LRU, move target filebuffer to front
		goto search_done;
	}
	if(ret.valid[0] == 1 && ret.valid[1] == 1)
	{
		ret.type = PARTIAL_HIT; // this is that write or read request is spread between two filebuffers.
		list_move(&(ret.secondpart->listptr),&headptr); // for LRU
		list_move(&(ret.firstpart->listptr),&headptr);
		goto search_done;
	}
	if(ret.valid[0] == 1 || ret.valid[1] == 1)
	{
		// this is routine for PARTIAL_MISS
		// which means that incomplete data of request is existed in filebuffer
		if(ret.valid[0] == 1)
		{
			list_move(&(ret.firstpart->listptr),&headptr); // for LRU
		}
		else
		{
			list_move(&(ret.secondpart->listptr),&headptr); // for LRU
		}
		ret.type = PARTIAL_MISS;
		goto search_done;
	}
search_done:
	return ret;
}

void removeAllBufferCache()
{
	/*
	 * this function is used for removing all filebuffer list
	 * only for debugging*/
	while(!list_empty(&headptr))
	{
		struct filebuffer *pos;
		
		pos = list_first_entry(&headptr,struct filebuffer,listptr);
		
		list_del(&(pos->listptr));
		free(pos);
	}
	bufferCount = 0;
	INIT_LIST_HEAD(&headptr);
}

void releaseFileBuffer(struct fuse_file_info *fi)
{
	/*
	 * if opened file is start to close, 
	 * release the file buffer list*/
	while(!list_empty(&headptr))
	{
		struct filebuffer *pos;
		
		// for each filebuffer node
		pos = list_first_entry(&headptr,struct filebuffer,listptr);
		
		if(pos->dirty == 1)
		{
			// should be written if dirty.
			pwrite(fi->fh,pos->buf,MAX_CHUNK_SIZE,pos->from);
		}
		
		// remove and free target node
		list_del(&(pos->listptr));
		free(pos);
	}
	bufferCount = 0;
	INIT_LIST_HEAD(&headptr);
	/*struct filebuffer *pos;
	
	list_for_each_entry(pos,&headptr,listptr)
	{
		if(pos->dirty == 1)
		{
			// should be written
			pwrite(fi->fh,pos->buf,MAX_CHUNK_SIZE,pos->from);
		}
	}*/
}

void printFileBuffer()
{
	// only used for debugging
	struct filebuffer *pos;
	int i = 0;
	
	list_for_each_entry(pos,&headptr,listptr)
	{
		log_msg("printFileBuffer : %dth buffer : from %d to %d\n",i++,pos->from,pos->to);
	}
}

void addFileBuffer(char *buf, int from, int to, int fd)
{
	struct filebuffer temp;
	int targetEviction;
	/*
	 * if PARTIAL_MISS or PERFECT_MISS is occured, target request data should be added to filebuffer*/
	if(bufferCount >= MAX_LIST_SIZE)
	{
		// evict something
		loadPolicyFromFile();
		
		if(cachePolicy == 1)
		{
			struct filebuffer *pos;
			// Random eviction
			srand((unsigned int)time(0));
			targetEviction = 1+rand()%bufferCount;
			
			list_for_each_entry(pos,&headptr,listptr)
			{
				targetEviction--;
				if(targetEviction == 0)
				{
					//delete target filebuffer node.
					// if dirty flag is set, write it to disk
					list_del(&(pos->listptr));
					if(pos->dirty == 1)
					{
						pwrite(pos->fd,pos->buf,MAX_CHUNK_SIZE,pos->from);
					}
					free(pos);
					break;
				}
			}
		}
		else
		{
			// LRU
			// remove last entry, it would be operated as LRU, because filebuffer node is moved to front when referred.
			struct filebuffer *pos;
			pos = list_last_entry(&headptr,struct filebuffer, listptr);
			list_del(&(pos->listptr));
			if(pos->dirty == 1)
			{
				pwrite(pos->fd,pos->buf,MAX_CHUNK_SIZE,pos->from);
			}
			free(pos);
		}
		bufferCount--;
	}
	
	// just insert
	struct filebuffer *buffer = (struct filebuffer*)calloc(1,sizeof(struct filebuffer));
	
	// initialize
	buffer->dirty = 0;
	buffer->from = from;
	buffer->to = to;
	buffer->fd = fd;
	
	// data copy
	memcpy(buffer->buf,buf,MAX_CHUNK_SIZE);
	
	// add
	list_add(&(buffer->listptr),&headptr);
	bufferCount++;
	//printFileBuffer();
}

//sbh
void loadPolicyFromFile()
{
	FILE* fp; // file pointer for reading policy file.
    int firstValue = -1, secondValue = -1, thirdValue = -1; // value from ee516.conf
	
	// sbh : initialize the policy for encryption and cache policy.
    
    fp = fopen(configFilePath,"r");
    
    if(fp != NULL)
    {
		fscanf(fp,"%d%d%d",&firstValue,&secondValue,&thirdValue);
		
		if(firstValue >= 0  && secondValue >=0 && thirdValue >=0)
		{
			// it would be proper value.
			
			if(firstValue == 0 && secondValue == 0)
			{
				encryptEnabled = 0;
			}
			else
			{
				// encryptEnabled
				encryptEnabled = 1;
				forAddition = firstValue;
				forShift = secondValue;
			}
			
			// for cache policy
			
			if(thirdValue > 2 )
			{
				// No cache
				cachePolicy = 0;
			}
			else
			{
				cachePolicy = thirdValue;
			}
		}
		
		fclose(fp);
	}
	log_msg("Load from ee516.conf : Addition : %d, Shift : %d, CachePolicy : %d\n",forAddition,forShift,cachePolicy);
}
unsigned char applyBitshift(unsigned char target, int direction, int shift, int size)
{
	int modres = shift % size;
	unsigned char ret = 0;
	
	//log_msg("applyBitshift, shift : %d , modres : %d, size : %d\n",shift,modres,size);
	if(direction > 0)
	{
		// right rotate shift, ignoring sign bit
		ret = (target >> modres) | ( target << (sizeof(target)*CHAR_BIT - modres));
	}
	else
	{
		// left rotate shift, ignoring sign bit
		ret = (target << modres) | ( target >> (sizeof(target)*CHAR_BIT - modres));
	}
	return ret;
}

// Report errors to logfile and give -errno to caller
static int bb_error(char *str)
{
    int ret = -errno;
    
    log_msg("    ERROR %s: %s\n", str, strerror(errno));
    
    return ret;
}

// Check whether the given user is permitted to perform the given operation on the given 

//  All the paths I see are relative to the root of the mounted
//  filesystem.  In order to get to the underlying filesystem, I need to
//  have the mountpoint.  I'll save it away early on in main(), and then
//  whenever I need a path for something I'll call this to construct
//  it.
static void bb_fullpath(char fpath[PATH_MAX], const char *path)
{
    strcpy(fpath, BB_DATA->rootdir);
    strncat(fpath, path, PATH_MAX); // ridiculously long paths will
				    // break here

    log_msg("    bb_fullpath:  rootdir = \"%s\", path = \"%s\", fpath = \"%s\"\n",
	    BB_DATA->rootdir, path, fpath);
}

///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come indirectly from /usr/include/fuse.h
//
/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int bb_getattr(const char *path, struct stat *statbuf)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_getattr(path=\"%s\", statbuf=0x%08x)\n",
	  path, statbuf);
    bb_fullpath(fpath, path);
    
    retstat = lstat(fpath, statbuf);
    if (retstat != 0)
	retstat = bb_error("bb_getattr lstat");
    
    log_stat(statbuf);
    
    return retstat;
}

/** Read the target of a symbolic link
 *
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.  If the linkname is too long to fit in the
 * buffer, it should be truncated.  The return value should be 0
 * for success.
 */
// Note the system readlink() will truncate and lose the terminating
// null.  So, the size passed to to the system readlink() must be one
// less than the size passed to bb_readlink()
// bb_readlink() code by Bernardo F Costa (thanks!)
int bb_readlink(const char *path, char *link, size_t size)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("bb_readlink(path=\"%s\", link=\"%s\", size=%d)\n",
	  path, link, size);
    bb_fullpath(fpath, path);
    
    retstat = readlink(fpath, link, size - 1);
    if (retstat < 0)
	retstat = bb_error("bb_readlink readlink");
    else  {
	link[retstat] = '\0';
	retstat = 0;
    }
    
    return retstat;
}

/** Create a file node
 *
 * There is no create() operation, mknod() will be called for
 * creation of all non-directory, non-symlink nodes.
 */
// shouldn't that comment be "if" there is no.... ?
int bb_mknod(const char *path, mode_t mode, dev_t dev)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_mknod(path=\"%s\", mode=0%3o, dev=%lld)\n",
	  path, mode, dev);
    bb_fullpath(fpath, path);
    
    // On Linux this could just be 'mknod(path, mode, rdev)' but this
    //  is more portable
    if (S_ISREG(mode)) {
        retstat = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
	if (retstat < 0)
	    retstat = bb_error("bb_mknod open");
        else {
            retstat = close(retstat);
	    if (retstat < 0)
		retstat = bb_error("bb_mknod close");
	}
    } else
	if (S_ISFIFO(mode)) {
	    retstat = mkfifo(fpath, mode);
	    if (retstat < 0)
		retstat = bb_error("bb_mknod mkfifo");
	} else {
	    retstat = mknod(fpath, mode, dev);
	    if (retstat < 0)
		retstat = bb_error("bb_mknod mknod");
	}
    
    return retstat;
}

/** Create a directory */
int bb_mkdir(const char *path, mode_t mode)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_mkdir(path=\"%s\", mode=0%3o)\n",
	    path, mode);
    bb_fullpath(fpath, path);
    
    retstat = mkdir(fpath, mode);
    if (retstat < 0)
	retstat = bb_error("bb_mkdir mkdir");
    
    return retstat;
}

/** Remove a file */
int bb_unlink(const char *path)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    //removeAllBufferCache();
    
    log_msg("bb_unlink(path=\"%s\")\n",
	    path);
    bb_fullpath(fpath, path);
    
    retstat = unlink(fpath);
    if (retstat < 0)
	retstat = bb_error("bb_unlink unlink");
    
    return retstat;
}

/** Remove a directory */
int bb_rmdir(const char *path)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("bb_rmdir(path=\"%s\")\n",
	    path);
    bb_fullpath(fpath, path);
    
    retstat = rmdir(fpath);
    if (retstat < 0)
	retstat = bb_error("bb_rmdir rmdir");
    
    return retstat;
}

/** Create a symbolic link */
// The parameters here are a little bit confusing, but do correspond
// to the symlink() system call.  The 'path' is where the link points,
// while the 'link' is the link itself.  So we need to leave the path
// unaltered, but insert the link into the mounted directory.
int bb_symlink(const char *path, const char *link)
{
    int retstat = 0;
    char flink[PATH_MAX];
    
    log_msg("\nbb_symlink(path=\"%s\", link=\"%s\")\n",
	    path, link);
    bb_fullpath(flink, link);
    
    retstat = symlink(path, flink);
    if (retstat < 0)
	retstat = bb_error("bb_symlink symlink");
    
    return retstat;
}

/** Rename a file */
// both path and newpath are fs-relative
int bb_rename(const char *path, const char *newpath)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    char fnewpath[PATH_MAX];
    
    log_msg("\nbb_rename(fpath=\"%s\", newpath=\"%s\")\n",
	    path, newpath);
    bb_fullpath(fpath, path);
    bb_fullpath(fnewpath, newpath);
    
    retstat = rename(fpath, fnewpath);
    if (retstat < 0)
	retstat = bb_error("bb_rename rename");
    
    return retstat;
}

/** Create a hard link to a file */
int bb_link(const char *path, const char *newpath)
{
    int retstat = 0;
    char fpath[PATH_MAX], fnewpath[PATH_MAX];
    
    log_msg("\nbb_link(path=\"%s\", newpath=\"%s\")\n",
	    path, newpath);
    bb_fullpath(fpath, path);
    bb_fullpath(fnewpath, newpath);
    
    retstat = link(fpath, fnewpath);
    if (retstat < 0)
	retstat = bb_error("bb_link link");
    
    return retstat;
}

/** Change the permission bits of a file */
int bb_chmod(const char *path, mode_t mode)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_chmod(fpath=\"%s\", mode=0%03o)\n",
	    path, mode);
    bb_fullpath(fpath, path);
    
    retstat = chmod(fpath, mode);
    if (retstat < 0)
	retstat = bb_error("bb_chmod chmod");
    
    return retstat;
}

/** Change the owner and group of a file */
int bb_chown(const char *path, uid_t uid, gid_t gid)
  
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_chown(path=\"%s\", uid=%d, gid=%d)\n",
	    path, uid, gid);
    bb_fullpath(fpath, path);
    
    retstat = chown(fpath, uid, gid);
    if (retstat < 0)
	retstat = bb_error("bb_chown chown");
    
    return retstat;
}

/** Change the size of a file */
int bb_truncate(const char *path, off_t newsize)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_truncate(path=\"%s\", newsize=%lld)\n",
	    path, newsize);
    bb_fullpath(fpath, path);
    
    retstat = truncate(fpath, newsize);
    if (retstat < 0)
	bb_error("bb_truncate truncate");
    
    return retstat;
}

/** Change the access and/or modification times of a file */
/* note -- I'll want to change this as soon as 2.6 is in debian testing */
int bb_utime(const char *path, struct utimbuf *ubuf)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_utime(path=\"%s\", ubuf=0x%08x)\n",
	    path, ubuf);
    bb_fullpath(fpath, path);
    
    retstat = utime(fpath, ubuf);
    if (retstat < 0)
	retstat = bb_error("bb_utime utime");
    
    return retstat;
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int bb_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    int fd;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_open(path\"%s\", fi=0x%08x)\n",
	    path, fi);
    bb_fullpath(fpath, path);
    
    fd = open(fpath, fi->flags);
    if (fd < 0)
	retstat = bb_error("bb_open open");
    
    fi->fh = fd;
    log_fi(fi);
    
    return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
// I don't fully understand the documentation above -- it doesn't
// match the documentation for the read() system call which says it
// can return with anything up to the amount of data requested. nor
// with the fusexmp code which returns the amount of data also
// returned by read.
int bb_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0, i;
    char *intermediateBuf;
    int pos,len;
    
    log_msg("\nbb_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi);
    // no need to get fpath on this one, since I work from fi->fh not the path
    log_fi(fi);
    
    loadPolicyFromFile(); // read policy from ee516.conf
    if(cachePolicy != 0 && size == MAX_CHUNK_SIZE)
    {
		// can use cache
		struct hitinfo info;
		
		info = bufferHitCheck((int)offset,size,fi->fh); // determine cache access reseult
		
		if(info.type == PERFECT_HIT)
		{
			// perfect hit
			log_msg("bb_read_cache : PERFECT_HIT -> from %d to %d\n",info.firstpart->from,info.firstpart->to);
			memcpy(buf,info.firstpart->buf,size);
			retstat = size;
			goto cache_read_process_done;
		}
		else if(info.type == PARTIAL_HIT)
		{
			char* totalbuf = (char*)calloc(2*size,sizeof(char));
			int refinedOffset = offset % MAX_CHUNK_SIZE;
			
			log_msg("bb_read_cache : PARTIAL_HIT -> from %d to %d\n",refinedOffset,refinedOffset+size);
			
			// concatenate two part
			sprintf(totalbuf,"%s%s",info.firstpart->buf,info.secondpart->buf);
			
			// then copy proper offset to buf
			memcpy(buf,totalbuf+refinedOffset,size);
			
			free(totalbuf);
			
			retstat = size;
			goto cache_read_process_done;			
		}else if(info.type == PARTIAL_MISS)
		{
			int start;
			char *tempbuf,*totalbuf;
			int refinedOffset = offset % MAX_CHUNK_SIZE;
			
			// partial miss, we should read proper data for caching
			
			if(info.valid[0] == 1)
			{
				log_msg("bb_read_cache : PARTIAL_MISS(Firstpart is OK) -> from %d to %d\n",info.firstpart->from,info.firstpart->to);
				// we need second one
				start = info.firstpart->to;
				tempbuf = (char*)calloc(size,sizeof(char));
				
				// read first part from disk
				pread(fi->fh,tempbuf,size,start);
				
				// add second part to Buffer
				addFileBuffer(tempbuf,start,start+size,fi->fh);
				
				totalbuf = (char*)calloc(2*size,sizeof(char));
				
				sprintf(totalbuf,"%s%s",info.firstpart->buf,tempbuf);
				
				// copy proper data to buf
				memcpy(buf,totalbuf+refinedOffset,size);
				
				free(tempbuf);
				free(totalbuf);
				
				retstat = size;
				
				goto cache_read_process_done;
			}
			
			if(info.valid[1] == 1)
			{
				log_msg("bb_read_cache : PARTIAL_MISS(Secondpart is OK) -> from %d to %d\n",info.secondpart->from,info.secondpart->to);
				// we need first one
				start = info.secondpart->to - MAX_CHUNK_SIZE;
				
				tempbuf = (char*)calloc(size,sizeof(char));
				
				// read second part from disk
				pread(fi->fh,tempbuf,size,start);
				
				// add first part to Buffer
				addFileBuffer(tempbuf,start,start+size,fi->fh);
				
				totalbuf = (char*)calloc(2*size,sizeof(char));
				
				sprintf(totalbuf,"%s%s",tempbuf,info.secondpart->buf);
				
				// copy proper data to buf
				memcpy(buf,totalbuf+refinedOffset,size);
				
				free(tempbuf);
				free(totalbuf);
				
				retstat = size;
				
				goto cache_read_process_done;
			}
			
			
		}else if(info.type == PERFECT_MISS)
		{
			// absolutely no data in filebuffer list
			int p,start;
			char *tempbuf,*totalbuf;
			int refinedOffset = offset % MAX_CHUNK_SIZE;
			
			p = offset % MAX_CHUNK_SIZE;
			
			if(p == 0)
			{
				// perfect align!
				start = offset;
				log_msg("bb_read_cache : PERFECT_MISS(perfect align) -> from %d to %d\n",start,start+size);
				tempbuf = (char*)calloc(size,sizeof(char));
				pread(fi->fh,tempbuf,size,start);
				
				// add to buffer
				addFileBuffer(tempbuf,start,start+size,fi->fh);
				
				memcpy(buf,tempbuf,size);
				
				free(tempbuf);
				
				retstat = size;
				
				goto cache_read_process_done;
			}
			else
			{
				// we need two chunks, because it is not aligned
				start = offset - p;
				refinedOffset = offset % MAX_CHUNK_SIZE;
				
				log_msg("bb_read_cache : PERFECT_MISS(partial align) -> from %d to %d\n",refinedOffset,refinedOffset+size);
				
				tempbuf = (char*)calloc(2*size,sizeof(char));
				
				pread(fi->fh,tempbuf,size*2,start);
				
				// add two parts to filebuffer
				addFileBuffer(tempbuf,start,start+size,fi->fh);
				addFileBuffer(tempbuf+size,start+size,start+size+size,fi->fh);								
				
				memcpy(buf,tempbuf+refinedOffset,size);
				
				free(tempbuf);
				
				retstat = size;
				
				goto cache_read_process_done;
			}
		}
	}
    
    retstat = pread(fi->fh, buf, size, offset);
    
    // sbh decryption    
cache_read_process_done:
    len = retstat;
    
    if(encryptEnabled == 1)
    {
		for(i=0;i<len;i++)
		{
			// bitshift
			buf[i] = applyBitshift(buf[i],-1,forShift,CHAR_BIT);
		}
		for(i=0;i<len;i++)
		{
			// addition
			buf[i] = buf[i] - forAddition;
		}
	}
	
    if (retstat < 0)
	retstat = bb_error("bb_read read");
    
    return retstat;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
// As  with read(), the documentation above is inconsistent with the
// documentation for the write() system call.
int bb_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    int retstat = 0,i;
    char *intermediateBuf;
    int pos,len;
    
    log_msg("\nbb_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi
	    );
    // no need to get fpath on this one, since I work from fi->fh not the path
    log_fi(fi);
    
    // sbh encryption    
    len = size;
    loadPolicyFromFile();
    // intermediateBuf is needed, because we can't modify the const pointer
    intermediateBuf = (char*)calloc(size+1,sizeof(char));
    
    strncpy(intermediateBuf,buf,size);
    
    if(encryptEnabled == 1)
    {    
		for(i=0;i<len;i++)
		{
			// addition
			intermediateBuf[i] = intermediateBuf[i] + forAddition;
		}
		for(i=0;i<len;i++)
		{
			// bitshift
			intermediateBuf[i] = applyBitshift(intermediateBuf[i],1,forShift,CHAR_BIT);
		}
	}
	
	if(cachePolicy != 0 && size == MAX_CHUNK_SIZE)
	{
		// cache activation
		struct hitinfo info;
		
		info = bufferHitCheck((int)offset,size,fi->fh);
		
		if(info.type == PERFECT_HIT)
		{
			log_msg("bb_write_cache : PERFECT_HIT -> from %d to %d\n",info.firstpart->from,info.firstpart->to);
			// write it to file buffer
			memcpy(info.firstpart->buf,intermediateBuf,size);
			info.firstpart->dirty = 1; // set dirty flag
			
			retstat = size;
			goto cache_write_process_done;
		}
		else if(info.type == PARTIAL_HIT)
		{
			char* totalbuf = (char*)calloc(2*size,sizeof(char));
			int refinedOffset = offset % MAX_CHUNK_SIZE;
			
			log_msg("bb_write_cache : PARTIAL_HIT -> from %d to %d\n",refinedOffset,refinedOffset+size);
			
			// concatenation.
			sprintf(totalbuf,"%s%s",info.firstpart->buf,info.secondpart->buf);
			
			// write it to intermediate buffer
			memcpy(totalbuf+refinedOffset,intermediateBuf,size);
			
			// then write it to file buffer
			memcpy(info.firstpart->buf,totalbuf,size);
			memcpy(info.secondpart->buf,totalbuf+MAX_CHUNK_SIZE,size);
			
			info.firstpart->dirty = 1;
			info.secondpart->dirty = 1;
			
			free(totalbuf);
			
			retstat = size;
			goto cache_write_process_done;
						
		}else if(info.type == PARTIAL_MISS)
		{
			int start;
			char *tempbuf,*totalbuf;
			int refinedOffset = offset % MAX_CHUNK_SIZE;
			
			if(info.valid[0] == 1)
			{
				// we need second one
				start = info.firstpart->to;
				tempbuf = (char*)calloc(size,sizeof(char));
				
				log_msg("bb_write_cache : PARTIAL_MISS(Firstpart is OK) -> from %d to %d\n",info.firstpart->from,info.firstpart->to);
				
				pread(fi->fh,tempbuf,size,start);
				
				totalbuf = (char*)calloc(2*size,sizeof(char));
				
				sprintf(totalbuf,"%s%s",info.firstpart->buf,tempbuf);
				
				// write data to intermediate buffer
				memcpy(totalbuf+refinedOffset,intermediateBuf,size);
				
				// firstpart write
				memcpy(info.firstpart->buf,totalbuf,size);
				info.firstpart->dirty = 1;
				
				// secondpart insert
				addFileBuffer(totalbuf+size,start+size,start+size+size,fi->fh);
				
				free(tempbuf);
				free(totalbuf);
				
				retstat = size;
				goto cache_write_process_done;
			}
			
			if(info.valid[1] == 1)
			{
				// we need first one
				start = info.secondpart->from - MAX_CHUNK_SIZE;
				
				tempbuf = (char*)calloc(size,sizeof(char));
				
				log_msg("bb_write_cache : PARTIAL_MISS(Second is OK) -> from %d to %d\n",info.secondpart->from,info.secondpart->to);
				
				pread(fi->fh,tempbuf,size,start);
				
				
				totalbuf = (char*)calloc(2*size,sizeof(char));
				
				sprintf(totalbuf,"%s%s",tempbuf,info.secondpart->buf);
				
				memcpy(totalbuf+refinedOffset,intermediateBuf,size);
				
				// secondpart write
				
				memcpy(info.secondpart->buf,totalbuf+size,size);
				
				// firstpart insert
				addFileBuffer(totalbuf,start,start+size,fi->fh);
				
				free(tempbuf);
				free(totalbuf);
				
				retstat = size;
				goto cache_write_process_done;
			}
			
			
		}else if(info.type == PERFECT_MISS)
		{
			int p,start;
			char *tempbuf,*totalbuf;
			int refinedOffset = offset % MAX_CHUNK_SIZE;
			
			p = offset % MAX_CHUNK_SIZE;
			
			if(p == 0)
			{
				// perfect align!
				start = offset;
				log_msg("bb_write_cache : PERFECT_MISS(perfect align) -> from %d to %d\n",start,start+size);
				tempbuf = (char*)calloc(size,sizeof(char));
				
				// read proper part to buffer for writing
				pread(fi->fh,tempbuf,size,start);
				
				// copy it to intermediate buffer
				memcpy(tempbuf,intermediateBuf,size);
				
				// add it to buffer
				addFileBuffer(tempbuf,start,start+size,fi->fh);
				
				// insert it
				
				free(tempbuf);
				
				retstat = size;
				goto cache_write_process_done;
			}
			else
			{
				// we need two chunks
				start = offset - p;
				refinedOffset = offset % MAX_CHUNK_SIZE;
				
				log_msg("bb_write_cache : PERFECT_MISS(partial align) -> from %d to %d\n",refinedOffset,refinedOffset+size);
				tempbuf = (char*)calloc(2*size,sizeof(char));
				
				// read proper part to buffer for writing
				pread(fi->fh,tempbuf,size*2,start);
				
				memcpy(tempbuf+refinedOffset,intermediateBuf,size);
				
				// insert two parts to file buffer
				addFileBuffer(tempbuf,start,start+size,fi->fh);
				addFileBuffer(tempbuf+size,start+size,start+size+size,fi->fh);
				
				free(tempbuf);
				
				retstat = size;
				goto cache_write_process_done;
			}
		}
		
	}
	
    retstat = pwrite(fi->fh, intermediateBuf, size, offset);
    
    free(intermediateBuf);

cache_write_process_done:
    
    if (retstat < 0)
	retstat = bb_error("bb_write pwrite");
    
    return retstat;
}

/** Get file system statistics
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 *
 * Replaced 'struct statfs' parameter with 'struct statvfs' in
 * version 2.5
 */
int bb_statfs(const char *path, struct statvfs *statv)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_statfs(path=\"%s\", statv=0x%08x)\n",
	    path, statv);
    bb_fullpath(fpath, path);
    
    // get stats for underlying filesystem
    retstat = statvfs(fpath, statv);
    if (retstat < 0)
	retstat = bb_error("bb_statfs statvfs");
    
    log_statvfs(statv);
    
    return retstat;
}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
int bb_flush(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_flush(path=\"%s\", fi=0x%08x)\n", path, fi);
    // no need to get fpath on this one, since I work from fi->fh not the path
    log_fi(fi);
	
    return retstat;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int bb_release(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    //sbh : if file is closing, release file buffer list
    releaseFileBuffer(fi);
    
    log_msg("\nbb_release(path=\"%s\", fi=0x%08x)\n",
	  path, fi);
    log_fi(fi);

    // We need to close the file.  Had we allocated any resources
    // (buffers etc) we'd need to free them here as well.
    retstat = close(fi->fh);
    
    return retstat;
}

/** Synchronize file contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 *
 * Changed in version 2.2
 */
int bb_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_fsync(path=\"%s\", datasync=%d, fi=0x%08x)\n",
	    path, datasync, fi);
    log_fi(fi);
    
    // some unix-like systems (notably freebsd) don't have a datasync call
#ifdef HAVE_FDATASYNC
    if (datasync)
	retstat = fdatasync(fi->fh);
    else
#endif	
	retstat = fsync(fi->fh);
    
    if (retstat < 0)
	bb_error("bb_fsync fsync");
    
    return retstat;
}

#ifdef HAVE_SYS_XATTR_H
/** Set extended attributes */
int bb_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_setxattr(path=\"%s\", name=\"%s\", value=\"%s\", size=%d, flags=0x%08x)\n",
	    path, name, value, size, flags);
    bb_fullpath(fpath, path);
    
    retstat = lsetxattr(fpath, name, value, size, flags);
    if (retstat < 0)
	retstat = bb_error("bb_setxattr lsetxattr");
    
    return retstat;
}

/** Get extended attributes */
int bb_getxattr(const char *path, const char *name, char *value, size_t size)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_getxattr(path = \"%s\", name = \"%s\", value = 0x%08x, size = %d)\n",
	    path, name, value, size);
    bb_fullpath(fpath, path);
    
    retstat = lgetxattr(fpath, name, value, size);
    if (retstat < 0)
	retstat = bb_error("bb_getxattr lgetxattr");
    else
	log_msg("    value = \"%s\"\n", value);
    
    return retstat;
}

/** List extended attributes */
int bb_listxattr(const char *path, char *list, size_t size)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    char *ptr;
    
    log_msg("bb_listxattr(path=\"%s\", list=0x%08x, size=%d)\n",
	    path, list, size
	    );
    bb_fullpath(fpath, path);
    
    retstat = llistxattr(fpath, list, size);
    if (retstat < 0)
	retstat = bb_error("bb_listxattr llistxattr");
    
    log_msg("    returned attributes (length %d):\n", retstat);
    for (ptr = list; ptr < list + retstat; ptr += strlen(ptr)+1)
	log_msg("    \"%s\"\n", ptr);
    
    return retstat;
}

/** Remove extended attributes */
int bb_removexattr(const char *path, const char *name)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_removexattr(path=\"%s\", name=\"%s\")\n",
	    path, name);
    bb_fullpath(fpath, path);
    
    retstat = lremovexattr(fpath, name);
    if (retstat < 0)
	retstat = bb_error("bb_removexattr lrmovexattr");
    
    return retstat;
}
#endif

/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int bb_opendir(const char *path, struct fuse_file_info *fi)
{
    DIR *dp;
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_opendir(path=\"%s\", fi=0x%08x)\n",
	  path, fi);
    bb_fullpath(fpath, path);
    
    dp = opendir(fpath);
    if (dp == NULL)
	retstat = bb_error("bb_opendir opendir");
    
    fi->fh = (intptr_t) dp;
    
    log_fi(fi);
    
    return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
int bb_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
	       struct fuse_file_info *fi)
{
    int retstat = 0;
    DIR *dp;
    struct dirent *de;
    
    log_msg("\nbb_readdir(path=\"%s\", buf=0x%08x, filler=0x%08x, offset=%lld, fi=0x%08x)\n",
	    path, buf, filler, offset, fi);
    // once again, no need for fullpath -- but note that I need to cast fi->fh
    dp = (DIR *) (uintptr_t) fi->fh;

    // Every directory contains at least two entries: . and ..  If my
    // first call to the system readdir() returns NULL I've got an
    // error; near as I can tell, that's the only condition under
    // which I can get an error from readdir()
    de = readdir(dp);
    if (de == 0) {
	retstat = bb_error("bb_readdir readdir");
	return retstat;
    }

    // This will copy the entire directory into the buffer.  The loop exits
    // when either the system readdir() returns NULL, or filler()
    // returns something non-zero.  The first case just means I've
    // read the whole directory; the second means the buffer is full.
    do {
	log_msg("calling filler with name %s\n", de->d_name);
	if (filler(buf, de->d_name, NULL, 0) != 0) {
	    log_msg("    ERROR bb_readdir filler:  buffer full");
	    return -ENOMEM;
	}
    } while ((de = readdir(dp)) != NULL);
    
    log_fi(fi);
    
    return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int bb_releasedir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_releasedir(path=\"%s\", fi=0x%08x)\n",
	    path, fi);
    log_fi(fi);
    
    closedir((DIR *) (uintptr_t) fi->fh);
    
    return retstat;
}

/** Synchronize directory contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data
 *
 * Introduced in version 2.3
 */
// when exactly is this called?  when a user calls fsync and it
// happens to be a directory? ???
int bb_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_fsyncdir(path=\"%s\", datasync=%d, fi=0x%08x)\n",
	    path, datasync, fi);
    log_fi(fi);
    
    return retstat;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
// Undocumented but extraordinarily useful fact:  the fuse_context is
// set up before this function is called, and
// fuse_get_context()->private_data returns the user_data passed to
// fuse_main().  Really seems like either it should be a third
// parameter coming in here, or else the fact should be documented
// (and this might as well return void, as it did in older versions of
// FUSE).
void *bb_init(struct fuse_conn_info *conn)
{
    log_msg("\nbb_init()\n");
    
    log_conn(conn);
    log_fuse_context(fuse_get_context());
    
    return BB_DATA;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void bb_destroy(void *userdata)
{
    log_msg("\nbb_destroy(userdata=0x%08x)\n", userdata);
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
int bb_access(const char *path, int mask)
{
    int retstat = 0;
    char fpath[PATH_MAX];
   
    log_msg("\nbb_access(path=\"%s\", mask=0%o)\n",
	    path, mask);
    bb_fullpath(fpath, path);
    
    retstat = access(fpath, mask);
    
    if (retstat < 0)
	retstat = bb_error("bb_access access");
    
    return retstat;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int bb_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    int fd;
    
    log_msg("\nbb_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n",
	    path, mode, fi);
    bb_fullpath(fpath, path);
    
    fd = creat(fpath, mode);
    if (fd < 0)
	retstat = bb_error("bb_create creat");
    
    fi->fh = fd;
    
    log_fi(fi);
    
    return retstat;
}

/**
 * Change the size of an open file
 *
 * This method is called instead of the truncate() method if the
 * truncation was invoked from an ftruncate() system call.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the truncate() method will be
 * called instead.
 *
 * Introduced in version 2.5
 */
int bb_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_ftruncate(path=\"%s\", offset=%lld, fi=0x%08x)\n",
	    path, offset, fi);
    log_fi(fi);
    
    retstat = ftruncate(fi->fh, offset);
    if (retstat < 0)
	retstat = bb_error("bb_ftruncate ftruncate");
    
    return retstat;
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
int bb_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_fgetattr(path=\"%s\", statbuf=0x%08x, fi=0x%08x)\n",
	    path, statbuf, fi);
    log_fi(fi);

    // On FreeBSD, trying to do anything with the mountpoint ends up
    // opening it, and then using the FD for an fgetattr.  So in the
    // special case of a path of "/", I need to do a getattr on the
    // underlying root directory instead of doing the fgetattr().
    if (!strcmp(path, "/"))
	return bb_getattr(path, statbuf);
    
    retstat = fstat(fi->fh, statbuf);
    if (retstat < 0)
	retstat = bb_error("bb_fgetattr fstat");
    
    log_stat(statbuf);
    
    return retstat;
}

struct fuse_operations bb_oper = {
  .getattr = bb_getattr,
  .readlink = bb_readlink,
  // no .getdir -- that's deprecated
  .getdir = NULL,
  .mknod = bb_mknod,
  .mkdir = bb_mkdir,
  .unlink = bb_unlink,
  .rmdir = bb_rmdir,
  .symlink = bb_symlink,
  .rename = bb_rename,
  .link = bb_link,
  .chmod = bb_chmod,
  .chown = bb_chown,
  .truncate = bb_truncate,
  .utime = bb_utime,
  .open = bb_open,
  .read = bb_read,
  .write = bb_write,
  /** Just a placeholder, don't set */ // huh???
  .statfs = bb_statfs,
  .flush = bb_flush,
  .release = bb_release,
  .fsync = bb_fsync,
  
#ifdef HAVE_SYS_XATTR_H
  .setxattr = bb_setxattr,
  .getxattr = bb_getxattr,
  .listxattr = bb_listxattr,
  .removexattr = bb_removexattr,
#endif
  
  .opendir = bb_opendir,
  .readdir = bb_readdir,
  .releasedir = bb_releasedir,
  .fsyncdir = bb_fsyncdir,
  .init = bb_init,
  .destroy = bb_destroy,
  .access = bb_access,
  .create = bb_create,
  .ftruncate = bb_ftruncate,
  .fgetattr = bb_fgetattr
};

void bb_usage()
{
    fprintf(stderr, "usage:  bbfs [FUSE and mount options] rootDir mountPoint\n");
    abort();
}


int main(int argc, char *argv[])
{
    int fuse_stat;
    struct bb_state *bb_data;


    // bbfs doesn't do any access checking on its own (the comment
    // blocks in fuse.h mention some of the functions that need
    // accesses checked -- but note there are other functions, like
    // chown(), that also need checking!).  Since running bbfs as root
    // will therefore open Metrodome-sized holes in the system
    // security, we'll check if root is trying to mount the filesystem
    // and refuse if it is.  The somewhat smaller hole of an ordinary
    // user doing it with the allow_other flag is still there because
    // I don't want to parse the options string.
    //if ((getuid() == 0) || (geteuid() == 0)) {
	//fprintf(stderr, "Running BBFS as root opens unnacceptable security holes\n");
	//return 1;
    //}
    
    // Perform some sanity checking on the command line:  make sure
    // there are enough arguments, and that neither of the last two
    // start with a hyphen (this will break if you actually have a
    // rootpoint or mountpoint whose name starts with a hyphen, but so
    // will a zillion other programs)
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
	bb_usage();

    bb_data = malloc(sizeof(struct bb_state));
    if (bb_data == NULL) {
	perror("main calloc");
	abort();
    }

    // Pull the rootdir out of the argument list and save it in my
    // internal data
    bb_data->rootdir = realpath(argv[argc-2], NULL);
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
    
    bb_data->logfile = log_open();
    
        
    //loadPolicyFromFile();
    // sbh: initialize the bufferlist
    INIT_LIST_HEAD(&headptr);
    // set path of config file.
    getcwd(configFilePath,BUFSIZ);
    sprintf(configFilePath,"%s/%s",configFilePath,"ee516.conf");
    printf("%s\n",configFilePath);
    // turn over control to fuse
    fprintf(stderr, "about to call fuse_main\n");
    fuse_stat = fuse_main(argc, argv, &bb_oper, bb_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    
    return fuse_stat;
}
