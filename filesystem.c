#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#define DISK_S (4 * 1024 * 1024) // 4MB
#define MAX_FILES (64)
#define MAX_NAME (32)

//after closing the file store file data in the block
#define FTABLE_S (MAX_FILES * sizeof(fileData) + sizeof(size_t))

//only one vdisk open
static bool isOpen = false;

//static file descriptor for vdisk
static int vDisk;

//disk file path in system
static char *dPath;

//file data structure
typedef struct
{
	char name[MAX_NAME];
	off_t size;
	off_t address;
} fileData;

//file list node
typedef struct node
{
	fileData data;

	struct node *next;
} node;

//file list
static node *head;
static node *tail;

//free space in bytes
static off_t freeSpace;

//write offset from the beginning of free space
static off_t offset;

//occupied ftable slots
static size_t nFiles;

void openDisk(const char *fpath)
{
	//opens a file which will be a virtual disk
	//if the file does not exist, it is created

	if (isOpen)
	{
		fprintf("%s disk already opened\n", dPath);

		return;
	}

	int err;

	vDisk = open(fpath, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (vDisk == -1)
	{
		err = errno;

		//the file exists
		//load file data
		if (err == EEXIST)
		{
			vDisk = open(fpath, O_RDWR);

			//read number of files
			lseek(vDisk, MAX_FILES * sizeof(fileData), SEEK_SET);
			read(vDisk, &nFiles, sizeof(size_t));

			lseek(vDisk, 0, SEEK_SET);
			freeSpace = DISK_S;

			//read all nodes from F_TABLE
			int i;
			node *tmp;

			for (i = 0; i < nFiles; ++i)
			{
				tmp = (node *)malloc(sizeof(node));

				read(vDisk, &(tmp->data), sizeof(fileData));

				tmp->next = NULL;

				freeSpace -= tmp->data.size;

				if (head == NULL)
					tail = head = tmp;

				else
				{
					tail->next = tmp;
					tail = tmp;
				}
			}

			if (nFiles == 0)
				offset = 0;

			else
				offset = tail->data.address + tail->data.size;

			lseek(vDisk, offset + FTABLE_S, SEEK_SET);

			dPath = fpath;

			isOpen = true;

			return;
		}

		//other error
		else
		{
			perror("Cannot open the disk");

			return;
		}
	}

	//file does not exist and will be created

	//check if ftruncate failed
	if (ftruncate(vDisk, DISK_S + FTABLE_S) == -1)
	{
		perror("Cannot truncate the disk");

		return;
	}

	// Here file was created
	// set initial values
	//set file offset to the beginning of free space
	lseek(vDisk, FTABLE_S, SEEK_SET);
	dPath = fpath;

	offset = 0;
	nFiles = 0;
	freeSpace = DISK_S;

	head = tail = NULL;

	isOpen = true;
}

void closeDisk()
{
	//close disk and write file data in the data sector represented by F_TABLE which is on the beginning of the file used as a disk

	if (!isOpen)
	{
		return;
	}

	node *temp = head;
	node *del;

	//set file offset to the beginning
	lseek(vDisk, 0, SEEK_SET);

	//go through file list and write data to disk and free memory which is allocated for the list

	while (temp) // updating the F_TABLE using information from list
	{
		write(vDisk, &(temp->data), sizeof(fileData));

		del = temp;
		temp = temp->next;

		free(del);
	}

	tail = head = NULL;

	//write number of files stored on the disk at the end of data sector
	lseek(vDisk, MAX_FILES * sizeof(fileData), SEEK_SET);

	write(vDisk, &nFiles, sizeof(size_t));

	// close the disk
	close(vDisk);

	isOpen = false;
}

bool isNameUsed(const char *);

void copyToDisk(const char *sourcePath, const char *fileName)
{
	if (!isOpen)
	{
		return;
	}

	struct stat info; // information about file

	if (stat(sourcePath, &info) == -1)
	{
		perror("Cannot reach the file to be copied");

		return;
	}

	//check if allocation table is full
	if (nFiles == MAX_FILES)
	{
		fprintf(stderr, "File number limit reached\n");

		return;
	}

	//check if there is enough free space for the file
	if (freeSpace < info.st_size)
	{
		fprintf(stderr, "There is not enough space for this file\n");

		return;
	}

	//check if fileName length is short enough
	if (strlen(fileName) >= MAX_NAME)
	{
		fprintf(stderr, "%s -file name is too long\n", fileName);

		return;
	}

	//check if file with this name already exists
	if (!isNameUsed(fileName))
	{
		fprintf(stderr, "%s - file name is already used\n", fileName);

		return;
	}

	//create new structure with information about new file
	node *tmp = (node *)malloc(sizeof(node));

	strcpy(tmp->data.name, fileName);

	tmp->data.size = info.st_size;
	tmp->data.address = offset;
	tmp->next = NULL;

	++nFiles;

	//the first file
	if (head == NULL)
		tail = head = tmp;

	//else add new node at the end of file list
	else
	{
		tail->next = tmp;
		tail = tmp;
	}

	// try to open the file which will be copied
	int sourceFile = open(sourcePath, O_RDONLY);

	char *buffer = (char *)malloc(info.st_size);

	//copy file
	read(sourceFile, buffer, info.st_size);
	write(vDisk, buffer, info.st_size);

	free(buffer);

	//close copied file
	close(sourceFile);

	// change first free address and amount of free space
	offset += info.st_size;
	freeSpace -= info.st_size;

	printf("%s - copyToDisk done\n", fileName);
}

void copyFromDisk(const char *destinationPath, const char *fileName)
{
	if (!isOpen)
	{
		return;
	}

	if (nFiles == 0)
	{
		fprintf(stderr, "Disk is empty\n");

		return;
	}

	if (strlen(fileName) >= MAX_NAME)
	{
		fprintf(stderr, "%s - File name too long\n", fileName);

		return;
	}

	node *temp = head;

	while (temp)
	{
		//find the file on the disk
		if (strcmp(fileName, temp->data.name) == 0)
		{
			//attempt to open destination file
			int destination = open(destinationPath, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

			if (destination == -1)
			{
				perror("Cannot open the destination file");

				return;
			}

			char *buffer = (char *)malloc(temp->data.size);

			//read data from file to buffer
			pread(vDisk, buffer, temp->data.size, temp->data.address + FTABLE_S);

			//write data to destination file
			write(destination, buffer, temp->data.size);

			free(buffer);

			close(destination);

			printf("%s - copyFromDisk done\n", fileName);

			return;
		}

		temp = temp->next;
	}

	fprintf(stderr, "%s - File does not exist\n", fileName);
}

void removeFile(const char *fileName) // shift another files if needed
{
	if (!isOpen)
	{
		return;
	}

	if (nFiles == 0)
	{
		fprintf(stderr, "Disk is empty\n");

		return;
	}

	if (strlen(fileName) >= MAX_NAME)
	{
		fprintf(stderr, "%s - File name is too long\n", fileName);

		return;
	}

	node *temp = head;
	node *prev = NULL;

	while (temp)
	{

		if (strcmp(fileName, temp->data.name) == 0)
		{

			node *toShift = temp->next;
			off_t shifted = 0;
			off_t shiftbeg;

			//set shift begin position
			if (toShift)
				shiftbeg = toShift->data.address;

			while (toShift)
			{
				// update structures with information about files( in list)
				// addresses have changed
				toShift->data.address -= temp->data.size;
				shifted += toShift->data.size; // count the amount of blocks that will be shifted

				toShift = toShift->next;
			}

			//if deleted file is not on the end of used space
			if (temp->next)
			{
				// store all files that should be shifted in the buffer
				char *buffer = (char *)malloc(shifted);

				lseek(vDisk, shiftbeg + FTABLE_S, SEEK_SET);
				read(vDisk, buffer, shifted);

				//shift files (write them in the new position)
				lseek(vDisk, temp->data.address + FTABLE_S, SEEK_SET);
				write(vDisk, buffer, shifted);

				free(buffer);
			}

			//removed file is on the last position
			else
				tail = prev;

			//if tail is NULL removed file was the only file on the disk so offset is 0
			if (tail == NULL)
				offset = 0;

			else
				offset = tail->data.address + tail->data.size; // there were another files on the disk

			lseek(vDisk, offset + FTABLE_S, SEEK_SET); // set position at the first free block

			//update amount of free space
			freeSpace += temp->data.size;

			//update file list
			--nFiles;

			if (prev != NULL)
				prev->next = temp->next;

			else
				head = temp->next;

			//erase list node representing the file which is removed
			free(temp);

			printf("%s - removeFile done\n", fileName);

			return;
		}

		prev = temp;

		temp = temp->next;
	}

	fprintf(stderr, "%s - File does not exist\n", fileName);
}

void deleteList()
{
	node *temp = head;
	node *del;

	while (temp)
	{
		del = temp;
		temp = temp->next;

		free(del);
	}

	head = tail = NULL;
}

void removeDisk()
{
	//requires the disk to be open first

	if (!isOpen)
	{
		return;
	}

	close(vDisk);

	unlink(dPath);

	deleteList();

	isOpen = false;

	printf("Disk is removed\n");
}

void showDir()
{
	if (!isOpen)
	{
		return;
	}

	printf("Files: %d / %d\n", nFiles, MAX_FILES);

	if (head != NULL)
	{

		node *temp = head;
		int i = 1;
		while (temp)
		{
			printf("%d. %s \n", i, temp->data.name);
			i++;
			temp = temp->next;
		}
	}
}

void mapDisk()
{
	if (!isOpen)
	{
		return;
	}
	printf("DISK MAP\n");

	printf("Disk size: %d\n", DISK_S);
	printf("Number of files: %d\n", nFiles);
	printf("System-data sector: begin = 0, size = %d\n", FTABLE_S);
	printf("Data sector:\n");
	printf("Used: begin = %d size = %d\n", FTABLE_S, offset);
	printf("Free: begin = %d size = %d\n", FTABLE_S + offset, freeSpace);
}

bool isNameUsed(const char *fileName)
{
	//check if name exists among files on disk

	node *temp = head;

	while (temp)
	{
		if (strcmp(fileName, temp->data.name) == 0)
			return false;

		temp = temp->next;
	}

	return true;
}

int main(int argc, char **argv)
{
	openDisk(argv[1]);

	if (!strcmp(argv[2], "-sd"))
	{
		showDir();
	}

	else if (!strcmp(argv[2], "-md"))
	{
		mapDisk();
	}

	else if (!strcmp(argv[2], "-ct"))
	{
		copyToDisk(argv[3], argv[4]);
	}

	else if (!strcmp(argv[2], "-cf"))
	{
		copyFromDisk(argv[3], argv[4]);
	}

	else if (!strcmp(argv[2], "-rf"))
	{
		removeFile(argv[3]);
	}

	else if (!strcmp(argv[2], "-rd"))
	{
		removeDisk();
	}

	//close disk after program has finished
	if (isOpen)
		closeDisk();

	return 0;
}
