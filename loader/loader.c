/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <signal.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "exec_parser.h"

static so_exec_t *exec;
static int filero;	 // for the file in read-only mode
static int total_pages = 0;	 // the maximum number of pages

#define PAGE_SIZE 4096

static void segv_handler(int signum, siginfo_t *info, void *context)
{
	unsigned int iterator = 0;
	uintptr_t fault_address;	// Address at which fault occurred.
	so_seg_t *fault_seg = NULL;	// The segment with problems.
	int page_number = 0;
	void *page_map;
	int mflags = 0;			// flags for mapping
	int mapout = 0;			// mapped bytes outside of file size

	/*
	 * If the signal received is not SIGSEGV or
	 * the signal information is not given quit
	 */
	if (signum != SIGSEGV || info == NULL)
		return;

	fault_address = (uintptr_t) info->si_addr;

	/* Search for the segment with problems. */
	for (iterator = 0; iterator < exec->segments_no; iterator++) {
		if ((&exec->segments[iterator])->vaddr <= fault_address &&
		fault_address <= (&exec->segments[iterator])->vaddr +
		(&exec->segments[iterator])->mem_size)
			fault_seg = &exec->segments[iterator];
	}


	/* Check if the fault address can be found in the segments */
	if (fault_seg == NULL) {
		SIG_DFL(SIGSEGV);
		return; // exit if not found
	}

	page_number = (fault_address - fault_seg->vaddr) / PAGE_SIZE;

	/*
	 * Allocate a vector in data with the size of total_pages.
	 * Each page represents if the page has been mapped;
	 */
	if (fault_seg->data == NULL) {
		fault_seg->data = (char *) calloc(page_number, sizeof(char));
		total_pages = page_number;
		if (fault_seg->data == NULL) {
			SIG_DFL(SIGSEGV);
			return; // exit if mapped
		}
	} else if (total_pages < page_number) {
		total_pages = page_number;
		fault_seg->data = (char *) realloc(fault_seg->data, total_pages * sizeof(char));
	}

	if (((char *)(fault_seg->data))[page_number] == 1) {
		SIG_DFL(SIGSEGV);
		return; // exit if mapped
	}

	/*
	 * if file size == map size => fixed | shared
	 * Check if there is an undefined behaviour or
	 * exceeds file boundaries.
	 */
	mflags = MAP_PRIVATE | MAP_FIXED;
	if (fault_seg->file_size < fault_seg->mem_size) {
		if (fault_seg->file_size < page_number * PAGE_SIZE)
			mflags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED;
		else if (fault_seg->file_size < (page_number + 1) * PAGE_SIZE)
			mapout = (page_number + 1) * PAGE_SIZE -
				fault_seg->file_size;
	}

	page_map = mmap((void *) fault_seg->vaddr + page_number * PAGE_SIZE,
			PAGE_SIZE, PROT_READ | PROT_WRITE,
			mflags, filero,
			page_number * PAGE_SIZE + fault_seg->offset);

	if (page_map == MAP_FAILED) {
		SIG_DFL(SIGSEGV);
		return; // exit if failed
	}

	/* Set 0 the bytes that are outside of file size. */
	if (mapout != 0) {
		uintptr_t ptr = fault_seg->vaddr +
			PAGE_SIZE * (page_number + 1) - mapout;
		if (memset((char *) ptr, 0, mapout) == NULL) {
			SIG_DFL(SIGSEGV);
			return; // exit if failed
		}
	}

	/* Set permisions. */
	if (mprotect(page_map, PAGE_SIZE, fault_seg->perm) == -1) {
		SIG_DFL(SIGSEGV);
		return; // exit if failed
	}

	/* Set the page as mapped. */
	((char *)(fault_seg->data))[page_number] = 1;
}

int so_init_loader(void)
{
	int rc;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = segv_handler;
	sa.sa_flags = SA_SIGINFO;
	rc = sigaction(SIGSEGV, &sa, NULL);
	if (rc < 0) {
		perror("sigaction");
		return -1;
	}
	return 0;
}

int so_execute(char *path, char *argv[])
{
	filero = open(path, O_RDONLY);
	if (filero == -1)
		return -1;

	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	return -1;
}
