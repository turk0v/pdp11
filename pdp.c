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
//x3
#define ostat 0177564
#define odata 0177566

//какие параметры имеет команда 
// #define NO_PARAM 0
// #define HAS_XX 1		
// #define HAS_SS (1<<1) 	
// #define HAS_DD (1<<2) 	
// #define HAS_NN (1<<3) 	
// #define HAS_R4 (1<<4) 
// #define HAS_R6 (1<<5)
#define NO_PARAM 0
#define HAS_SS 1
#define HAS_DD (1<<1)
#define HAS_XX (1<<5)
#define HAS_R  (1<<3)
#define HAS_NN (1<<4)

int nn, rr, xx, z, b, n;

int R4, R6;


byte b_read  (adr a);
void b_write (adr a, byte val);
word w_read  (adr a);
void w_write (adr a, word val);
void do_add();
void do_mov();
void do_halt();
void do_unknown();
void do_sob();
void do_clr();
void do_movb();
void mem_dump(adr start, word n);
void load_file(char * s);
void test_mem();
void print_reg();
void run();
struct SSDD get_mode(word w);



struct Command {
	word opcode;
	word mask;
	const char * name;
	void (*do_func)();
	byte param;
			
}	cmdlist[] = {
	{0010000, 0170000, "mov",		do_mov, HAS_SS | HAS_DD },
	{0060000, 0170000, "add",		do_add , HAS_SS | HAS_DD },	
	{0000000, 0177777, "halt",		do_halt, NO_PARAM},
	{0077000,  0177000,  "sob",     do_sob,     HAS_NN|HAS_R},//here
	{0005000, 0177700, 	"clr",		do_clr, 	HAS_DD},
	{0110000, 0170000, "movb",		do_movb, 	HAS_SS | HAS_DD},
	{0000000, 0170000, "unknown", 	do_unknown , NO_PARAM}	
};




struct SSDD {
	word val;
	adr a;
} ss, dd;


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
void w_write(adr a, word val) 
{
    if (a < 8){
        reg[a] = val & 0xFF;
    }
    else{
        mem[a] = val & 0xFF;
        mem[a + 1] = (val >> 8) & 0xFF;
    }
    //printf("%d",val);
}




//печать регистра
void print_reg() {
	int i;
	printf("\nRigisters are: \n");
	for (i = 0; i < 4; ++i) {
		printf("R%d : %.7o ", i, reg[i]);
	}
	printf("\n");
	for (i = 4; i < 8; ++i) {
		printf("R%d : %.7o ", i, reg[i]);
	}
	printf("\n");
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
	printf("Doing nothing");
}

// x3
void do_mov() {
    // if (dd.a == odata) {
    //     printf("-----%c--- \n", ss.val);
    // }
    w_write(dd.a, ss.val);
    //z = (ss.val == 0);
}
void do_add() {
    // if (dd.a == odata) {
    //     printf("-----%c--- \n", ss.val+dd.val);
    // }
    w_write(dd.a, (ss.val + dd.val));
    //z = ((ss.val + dd.val) == 0);
}

void do_sob()//here
{
    reg[rr]--;
    if (reg[rr] != 0)
        pc = pc - 2*(nn);
    printf("R%d",rr);
}

void do_clr()
{
    w_write(dd.a, 0);
}

void do_movb()//не работает
{
    b_write(dd.a, ss.val);
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



void trace(int dbg_lvl, char * format, ...) {//1/2 x3
	if (dbg_lvl != debug_level)
		return;
	va_list argptr;
	va_start (argptr, format);
	vprintf(format, argptr);
	va_end(argptr);	
}



void mem_dump(adr start, int n) 
{
	printf("\nMemory dumping \n");
	int i;
	for (i = start; i < start + n; i = i + 2) 
	{
		printf("%07o : %07o\n", i, w_read(i));
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




struct SSDD get_mode(word w)
{
	struct SSDD res;
	word nn = w & 7;//достаем первые три бита
	word mode = (w>>3) & 7;//достаем вторые три бита 
	switch(mode)//сравниваем моду с разными числами
	{
		case 0:
			res.a = nn;
			res.val = reg[nn];//регистр содержит искомое значение
			printf("R%d ", nn);
			break;
		case 1:
			res.a = reg[nn]; //регистр содержит адрес ячейки памяти, где лежит значение
			if(b)
				res.val = b_read(res.a);
			else
				res.val = w_read(res.a);
			printf("(R%d) ", nn);
			break;
		case 2:
			res.a = reg[nn]; //регистр содержит адрес ячейки памяти, где лежит значение, значение регистра увелич.
			if ((b) && (reg[nn] < 6 ))
			{
				res.val = b_read(res.a); //байтовая +1
				reg[nn]++;
			}
			else
			{
				res.val = w_read(res.a);//слово +2
				reg[nn]+=2;
			}
            if (nn != 7) 
            {
               	printf("(R%d)+ ", nn);
            } 
            else 
            { 
                printf("#%o ", res.val);
            }
            break;
        case 3:
            res.a = w_read(reg[nn]);//регистр содержит адрес ячейки памяти, где лежит значение, значение регистра увелич.
            if (b) 
            {
                res.val = b_read(res.a);
            }
            else 
            {
                res.val = w_read(res.a);
            }
            if (nn != 7) 
            {
                printf(" @(R%d)+ ", nn);
            }
            else 
            {
                printf(" #%o ", res.val);
            }
            reg[nn]+=2;
            break;
        case 4:
            if (b)
            { //уменьшаем значение регистра, интерпретируем его как адрес и находим значение
                reg[nn]--;
            	res.a = reg[nn];
            	res.val = b_read(res.a);
            }
            else
            {
                reg[nn]-= 2;
            	res.a = reg[nn];
            	res.val = w_read(res.a);
            }
			printf("-(R%d) ", nn);
            break;
        case 5:
        	reg[nn]-=2;
        	res.a = w_read(reg[nn]);
            if (b)
                res.val = b_read(res.a);
            else
                res.val = w_read(res.a);
            printf("@-(R%d) ", nn);
        case 6:




        default:
            printf("Not implemented");

	}
	return res;
}

void run()
{
	printf("\nRunning \n");
    pc = 01000;

    while(1) {
        word w = w_read(pc) & 0xffff;
        b = w >> 15;//этим определяем байт или не байт
        printf("%06o : %06o ", pc, w);
        int i;
        pc += 2;
        for (i = 0; ; i++)
        {
            struct Command cmd = cmdlist[i];
            if((w & cmd.mask) == cmd.opcode) // команда / не команда
            {
                printf("%s ", cmd.name);
                if(cmd.param & HAS_SS)
					ss = get_mode(w>>6);
				if(cmd.param & HAS_DD)
					dd = get_mode(w);
                if (cmd.param & HAS_NN)
                    nn = w & 0x3F;
                if (cmd.param & HAS_R)
                    rr = (w >> 6) & 7;
                //printf("\n");
                cmd.do_func();

                //print_reg();
                break;
            }
        }
        
        printf("\n");
    }
}







int main(int argc, char * argv[])
{
	load_file(argv[argc - 1]);
	mem_dump(0x200, 0xc);
	load_file(argv[argc - 1]);
	print_reg();
	// print_reg();
	mem_dump(0x200, 18);
	b_write(ostat, 0xFF);
	run();
	print_reg();
	//printf(" number is %d\n", sizeof(cmdlist)/sizeof(cmdlist[0]) );

	return 0;
}