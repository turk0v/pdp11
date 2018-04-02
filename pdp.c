#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

typedef unsigned char byte;
typedef int word;
typedef int adr;

byte mem[56*1024];

#define LO(x) (x & 0xFF);
#define HI(x) (((x>>8) & 0xFF));

byte b_read  (adr a);
void b_write (adr a, byte val);
word w_read  (adr a);
void w_write (adr a, word val);
void mem_dump(adr start, word n);
void load_file(char* filename );
void test();
// void do_mov();
// void do_add();
// void do_halt();
// void do_unknown();

// struct Command 
// {
// 	word opcode;
// 	word mask;
// 	const char * name;
// 	void (*do_func)();
// } command [] = {
// 	{0010000,0170000, "mov", do_mov},
// 	{0060000,0170000, "add", do_add},
// 	{0000000,0177777, "halt", do_halt},
// 	{0000000,0170000, "unknown", do_unknown}
// };

byte b_read  (adr a)
{
    return mem[a];
}

void b_write (adr a, byte val)
{
    mem[a] = val;
}

// void do_mov()
// {
// 	exit(0);
// }
// void do_add()
// {
// 	exit(0);
// }
// void do_halt()
// {
// 	exit(0);
// }
// void do_unknown()
// {
// 	exit(0);
// }

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
void mem_dump(adr start, word n)
{
    for (int i = 0; i < n; i = i +2)
        printf("%.6o : %.6o \n", start + i, w_read(start + i)&0xFFFF);
}

void load_file(char* filename ) {
    unsigned int n, a, i, x;
    FILE *file;
    file = fopen(filename,"r");
    if (file == NULL) {
        perror(filename);
        fclose (file);
        exit(0);
    }
    while (1) {
        if(fscanf(file,"%x", &a) != 1)
            break;
        fscanf(file, "%x", &n);
        for (i = 0; i < n; i++) {
            fscanf(file, "%x", &x);
            b_write((adr)(a+i), x);
        }
    }
    fclose (file);
}

void test() 
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
	byte a = 0xa1;
	byte b = 0xb2;
	b_write(2,a);
	b_write(3,b);
	printf("%x\n", mem[2]);
	printf("%x\n", mem[3]);
	return 0;
}