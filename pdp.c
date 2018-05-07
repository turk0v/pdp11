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
void load_file(char * s);
void test_mem();
void print_reg();



// struct Command {
// 	word opcode;
// 	word mask;
// 	const char * name;
// 	void (*do_func)();
			
// }	command[] = {
// 	{0010000, 0170000, "mov",		do_mov},
// 	{0060000, 0170000, "add",		do_add},	
// 	{0000000, 0177777, "halt",		do_halt},
// 	{0000000, 0170000, "unknown", 	do_unknown}	
// };



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
void print_reg() {
	printf("\n");
	int i;
	for (i = 0; i < 8; i++) {
		printf("R%d:%06o ", i, reg[i]);
	}
	printf("\n");
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
	printf("\n");
	print_reg();
	printf("THE END\n");
	exit(0);
}
void do_unknown()
{
	exit(0);
}




//память-функции


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
	va_list argptr;
	va_start (argptr, format);
	vprintf(format, argptr);
	va_end(argptr);	
}



void mem_dump(adr start, int n) {
	int i;
	for (i = start; i < start + n; i = i + 2) {
		trace(0, "%06o : %06o\n", i, w_read(i));
	}
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




int main(int argc, char * argv[])
{
	printf("3 arg is %s\n", argv[2]);
	printf("there %d argc\n", argc);
	load_file(argv[argc - 1]);
	//print_reg();
	//mem_dump(0200, 10);

	return 0;
}