
#define _GNU_SOURCE
#include <dlfcn.h>


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// libc



#if 0

// malloc , ptr.mem
//calloc

void *_malloc(size_t size, const char *filename, int line) {

	void *ptr = malloc(size); 
	
	char buff[128] = {0};
	sprintf(buff, "./mem/%p.mem", ptr);
	
	FILE *fp = fopen(buff, "w");
	fprintf(fp, "[+]addr: %p, filename: %s, line: %d\n", ptr, filename, line);

	fflush(fp);
	fclose(fp);
	
	
	return ptr;
}

void _free(void *ptr, const char *filename, int line) {

	char buff[128] = {0};
	sprintf(buff, "./mem/%p.mem", ptr);

	if (unlink(buff) < 0) { // filename no exist;
		printf("double free: %p\n", ptr);
		return ;
	}
	
	return free(ptr);
}


#define malloc(size)  _malloc(size, __FILE__, __LINE__)
#define free(ptr)	_free(ptr, __FILE__, __LINE__)



#elif 0

// addr2line -f -e ./memleak -a 0x400b38

typedef void *(*malloc_t)(size_t size);
malloc_t malloc_f = NULL;

typedef void (*free_t)(void *ptr);
free_t free_f = NULL;


int enable_malloc_hook = 1;
int enable_free_hook = 1;


void *malloc(size_t size) {

	void *ptr = NULL;
	if (enable_malloc_hook) {
		enable_malloc_hook = 0;
	
		ptr = malloc_f(size);

	// main --> f1() --> f2() --> f3() { __builtin_return_address(0)  }

		void *caller = __builtin_return_address(0);

		char filename[128] = {0};
		sprintf(filename, "./mem/%p.mem", ptr);

		FILE *fp = fopen(filename, "w");
		fprintf(fp, "[+] caller: %p, addr: %p, size: %ld\n", caller, ptr, size);

		fflush(fp);
		
		enable_malloc_hook = 1;
	} else {
		ptr = malloc_f(size);
	}
	
	return ptr;
}

void free(void *ptr) {

	if (enable_free_hook) {
		enable_free_hook = 0;
        
        char buff[128] = {0};
		sprintf(buff, "./mem/%p.mem", ptr);

		if (unlink(buff) < 0) { // filename no exist;
			printf("double free: %p\n", ptr);
			return ;
		}

		free_f(ptr);

		enable_free_hook = 1;
	} else {

		free_f(ptr);
		
	}

}


void init_hook(void) {

	if (!malloc_f) {
		malloc_f = dlsym(RTLD_NEXT, "malloc");
	}
	if (!free_f) {
		free_f = dlsym(RTLD_NEXT, "free");
	}
}

#else


// malloc(); --> __libc_malloc();
extern void *__libc_malloc(size_t size);
extern void __libc_free(void *ptr);


int enable_malloc_hook = 1;
int enable_free_hook = 1;


void *malloc(size_t size) {

	void *ptr = NULL;
	if (enable_malloc_hook) {
		enable_malloc_hook = 0;
	
		ptr = __libc_malloc(size);

	// main --> f1() --> f2() --> f3() { __builtin_return_address(0)  }

		void *caller = __builtin_return_address(0);

		char filename[128] = {0};
		sprintf(filename, "./mem/%p.mem", ptr);

		FILE *fp = fopen(filename, "w");
		fprintf(fp, "[+] caller: %p, addr: %p, size: %ld\n", caller, ptr, size);

		fflush(fp);
		
		enable_malloc_hook = 1;
	} else {
		ptr = __libc_malloc(size);
	}
	
	return ptr;
}

void free(void *ptr) {

	if (enable_free_hook) {
		enable_free_hook = 0;
        
        char buff[128] = {0};
		sprintf(buff, "./mem/%p.mem", ptr);

		if (unlink(buff) < 0) { // filename no exist;
			printf("double free: %p\n", ptr);
			return ;
		}

		__libc_free(ptr);

		enable_free_hook = 1;
	} else {

		__libc_free(ptr);
		
	}

}




#endif
// 
//

#if 1

int main() {

	//init_hook();

	void *p1 = malloc(5);  //_malloc
	void *p2 = malloc(10);
	void *p3 = malloc(15);

	free(p1);
	free(p3);

}

#endif



