#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pwd.h>
#include <dirent.h>

void lseek_example()
{
	int fd;
	char buf[256]={0,};
	
	printf("-----------------------lseek_example-----------------------\n");
	// 1. make txt file
	system("touch lseek_example.txt");
	// 2. write alphabet
	system("echo abcdefghijklmnopqrstuvwxyz > lseek_example.txt");
	// 3. open
	fd = open("lseek_example.txt",O_RDWR);
	
	if(fd != -1)
	{
		// 4. initial read and print
		read(fd,buf,256);
		
		printf("inital read : %s\n",buf);
		
		// 5. move file pointer, 10bytes forwarded from start point
		lseek(fd, 10, SEEK_SET);
		
		// 6. read and print
		memset(buf,0,256);
		read(fd,buf,256);
		
		printf("after lseek : %s\n",buf);
		close(fd);
	}
	// 5. cleanup
	system("rm lseek_example.txt");
	printf("-----------------------lseek_example finished-----------------------\n");
}

void unlink_example()
{
	printf("-----------------------unlink_example-----------------------\n");
	// 1. make file
	system("touch unlink_example");
	// 2. write something
	system("echo abcdefghijklmnopqrstuvwxyz > unlink_example");
	
	printf("file unlink_example has been created!\n");
	// 3. show
	system("ls -l");
	
	// 4. perform unlink
	unlink("unlink_example");
	
	// 5. show
	printf("unlink has performed\n");
	system("ls -l");
	
	printf("-----------------------unlink_example finished-----------------------\n");
}

void link_example()
{
	printf("-----------------------link_example-----------------------\n");
	
	// 1. make file
	system("touch link_example");
	// 2. write something
	system("echo abcdefghijklmnopqrstuvwxyz > link_example");
	
	printf("file link_example has been created!\n");
	
	// 3. perform link
	link("link_example","hard_link_of_link_example");
	printf("link has performed between link_example and hard_link_of_link_example\n");
	// 4. show with inode number
	system("ls -i -l");
	
	// 5. clean up
	
	system("rm link_example hard_link_of_link_example");
	
	printf("-----------------------link_example finished-----------------------\n");
}

void symlink_example()
{
	printf("-----------------------symlink_example-----------------------\n");
	// 1. make file
	system("touch symlink_example");
	// 2. write something
	system("echo abcdefghijklmnopqrstuvwxyz > symlink_example");
	
	printf("file symlink_example has been created!\n");
	
	// 3. perform symlink
	symlink("symlink_example","sym_link_of_symlink_example");
	printf("symlink has performed between symlink_example and sym_link_of_symlink_example\n");
	// 4. show
	system("ls -l");
	
	// 5. clean up
	
	system("rm symlink_example sym_link_of_symlink_example");
	printf("-----------------------symlink_example finished-----------------------\n");
}

void readlink_example()
{
	char buf[256]={0,};
	printf("-----------------------readlink_example-----------------------\n");
	// 1. making symlink
	system("touch readlink_example");
	system("echo abcdefghijklmnopqrstuvwxyz > readlink_example");
	
	symlink("readlink_example","symlink_of_readlink_example");
	
	system("ls -l");
	
	// 2. perform readlink
	
	readlink("symlink_of_readlink_example",buf,256);
	
	// 3. show
	
	printf("\n\nreadlink result of symlink_of_readlink_example : %s\n",buf);
	
	// 4. cleanup
	
	system("rm symlink_of_readlink_example readlink_example");

	printf("-----------------------readlink_example finished-----------------------\n");
}

void fchmod_example()
{
	int fd;
	printf("-----------------------fchmod_example-----------------------\n");
	// 1. create sample file
	system("touch fchmod_example");
	
	printf("file fchmod_example has created\n");
	// 2. show permissions
	system("ls -l");
	
	
	// 3. perform fchmod
	
	fd = open("fchmod_example",O_RDWR);
	
	if(fd != -1)
	{
		// set 0007
		fchmod(fd,S_IROTH | S_IWOTH | S_IXOTH);
		
		close(fd);
	}
	printf("fchmod has been performed : 0007\n");
	// 4. show changed permissions
	system("ls -l");
	
	// 5. clean up
	
	system("rm fchmod_example");
	
	
	
	
	printf("-----------------------fchmod_example finished-----------------------\n");
}

void fchown_example()
{
	int fd;
	char username[256]={0,};
	uid_t uid;
	gid_t gid;
	struct passwd *user_pw;
	printf("-----------------------fchown_example-----------------------\n");
	// this function should be invoked by root.
	
	// 1. make file, it should has user and group id of root.
	system("touch fchown_example");
	
	// 2. show the owner and group owner
	system("ls -l");
	
	// 3. Request user session name from user"
	printf("Enter the user session name : ");
	scanf("%s",username);
	
	user_pw = getpwnam(username);
	
	if(user_pw == NULL)
	{
		printf("No such username %s\n",username);
		goto cleanup;
	}
	
	
	// 4. retrieve uid, gid from username
	uid = user_pw->pw_uid;
	gid = user_pw->pw_gid;
	
	// 5. perform fchown
	fd = open("fchown_example",O_RDWR);
	
	if(fd != -1)
	{
		fchown(fd,uid,gid);
		close(fd);
	}
	
	// 6. show result
	printf("fchown has been performed to file fchown_example by username\n");
	
	system("ls -l");
	
	
cleanup:
	system("rm fchown_example");
	printf("-----------------------fchown_example finished-----------------------\n");
}

void fstat_example()
{
	int fd;
	struct stat buf;
	printf("-----------------------fstat_example-----------------------\n");
	// 1. make file
	system("touch fstat_example");
	system("echo abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz > fstat_example");
	
	// 2. get information by invoking fstat
	
	fd = open("fstat_example",O_RDWR);
	
	printf("information of fstat_example\n");
	
	if(fd != -1)
	{
		fstat(fd,&buf);
		
		close(fd);
		
		// 3. show file information
		printf("id_of_device : %d\ninode_number : %d\npermissions : %o\nnumber_of_hard_links : %d\nuser_id_of_owner : %d\ngroup_id_of_owner : %d\ntotalsize : %d\nblocksize : %d\n",(int)buf.st_dev,(int)buf.st_ino,(int)buf.st_mode,(int)buf.st_nlink,(int)buf.st_uid,(int)buf.st_gid,(int)buf.st_size,(int)buf.st_blksize);
	}
	
	// 4. cleanup
	
	system("rm fstat_example");
	
	printf("-----------------------fstat_example finished-----------------------\n");
}

void readdir_example()
{
	DIR *dirinfo;
	struct dirent *entry;
	printf("-----------------------readdir_example-----------------------\n");
	
	// 1. open current directory
	dirinfo = opendir(".");
	
	if(dirinfo == NULL)
	{
		goto cleanup;
	}
	
	// 2. iterate for retrieving contents
	
	while((entry=readdir(dirinfo)) != NULL)
	{
		printf("Name : %s\n",entry->d_name);
	}
	
cleanup:
	
	printf("-----------------------readdir_example finished-----------------------\n");
}

void getcwd_example()
{
	char buf[256]={0,};
	char *ret;
	printf("-----------------------getcwd_example-----------------------\n");
	// perform getcwd directly
	ret = getcwd(buf,256);
	
	// show
	printf("return from getcwd : %s\n",ret);
	printf("buf passed as parameter : %s\n",buf);
	
	printf("-----------------------getcwd_example finished-----------------------\n");
}


int main()
{
	int number;
	
	while(1)
	{
		// serve menus.
		printf("Enter the execution number of function operation example\n\n");
		printf("\t1. lseek function\n");
		printf("\t2. unlink function\n");
		printf("\t3. link function\n");
		printf("\t4. symlink function\n");
		printf("\t5. readlink function\n");
		printf("\t6. fchmod function\n");
		printf("\t7. fchown function\n");
		printf("\t8. fstat function\n");
		printf("\t9. readdir function\n");
		printf("\t10. getcwd function\n");
		printf("\t11 EXIT\n");
		
		// get menu number
		scanf("%d",&number);
		
		switch(number)
		{
			case 1:
				lseek_example();
				break;
			case 2:
				unlink_example();
				break;
			case 3:
				link_example();
				break;
			case 4:
				symlink_example();
				break;
			case 5:
				readlink_example();
				break;
			case 6:
				fchmod_example();
				break;
			case 7:
				fchown_example();
				break;
			case 8:
				fstat_example();
				break;
			case 9:
				readdir_example();
				break;
			case 10:
				getcwd_example();
				break;
			case 11:
				goto end;
				break;
			default:
			printf("Wrong number, try again\n");
		}
	}
end:
	return 0;
}
