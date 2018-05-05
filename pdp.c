#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>

typedef unsigned char byte;
typedef int word;
typedef word adr;

byte mem[56*1024];
word reg[8];

#define LO(x) (x & 0xFF);
#define HI(x) (((x>>8) & 0xFF));

#define RELEASE 0
#define DEBUG 1
#define FULL_DEBUG 2

int debug_level = DEBUG;
#define sp reg[6]
#define pc reg[7]


#define NO_PARAM	0
#define HAS_SS		1
#define HAS_DD		2
#define HAS_NN		4
#define HAS_XX		8


byte b_read  (adr a);
void b_write (adr a, byte val);
word w_read  (adr a);
void w_write (adr a, word val);
void mem_dump(adr start, word n);
void load_file();
void test_mem();
void do_mov();
void do_add();
void do_halt();
void do_unknown();
void print_reg();
void trace(int dbg_lvl, char * format, ...);



struct Command {
	word opcode;
	word mask;
	const char * name;
	void (*do_func)();
			
}	command[] = {
	{0010000, 0170000, "mov",		do_mov},
	{0060000, 0170000, "add",		do_add},	
	{0000000, 0177777, "halt",		do_halt},
	{0000000, 0170000, "unknown", 	do_unknown}	
};



//запись/чтение из памяти 
void b_write (adr a, byte val)
{
    mem[a] = val;
}


byte b_read  (adr a)
{
    return mem[a];
}

word w_read  (adr a)
{
    word res;
    assert (a % 2 == 0);
    res = (word)(mem[a]) | (word)(mem[a+1] << 8);
    return res;        
}
void w_write (adr a, word val)
{
    assert (a % 2 == 0);
    mem[a] = LO(val);
    mem[a+1] = HI(val);
}




//печать регистра
void print_reg() 
{
	int i;
	for (i = 0; i < 8; ++i) {
		printf("r%d : %.6o\n", i, reg[i]);
	}
}





void do_mov()
{

}
void do_add()
{
	exit(0);
}
void do_halt()
{
	print_reg();
	printf("finished\n");
	exit(0);
}
void do_unknown()
{
	exit(0);
}




//память-функции

void mem_dump(adr start, word n)
{
    for (int i = 0; i < n; i = i +2)
        printf("%.6o : %.6o \n", start + i, w_read(start + i)&0xFFFF);
}

void load_file(char * s) {
	FILE *f_in = NULL;
	f_in = fopen(s,"r");
	if (f_in == NULL)
	{
		perror(s);
		exit(1);
	}
	int ad , i , n , *a;
	while (fscanf(f_in,"%x", &ad)> 0)
	{
		fscanf(f_in, "%x" , &n);
		a = malloc(n * sizeof(int));
		for(i = 0 ; i < n ;i++ )
		{
			fscanf(f_in, "%x", &a[i]);
			b_write(ad + i, a[i]);
		}
	}
	free(a);
	fclose(f_in);
}



void trace(int dbg_lvl, char * format, ...) {
	if (dbg_lvl != debug_level)
		return;
	va_list ap;
	va_start (ap, format);
	vprintf(format, ap);
	va_end(ap);	
}





void test_mem() 
{
	byte b0, b1;
	word w;
	w = 0x0b0a;
	w_write(2,w);
	b0 = b_read(2);
	b1 = b_read(3);
	printf ("%04x = %02hhx%02hhx\n", w , b1 , b0);
}

int main()
{
	load_file();
	mem_dump(0x200, 0xc);
	return 0;
}