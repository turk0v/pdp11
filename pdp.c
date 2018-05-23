#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

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
char **global_argv;

#define sp reg[6]
#define pc reg[7]
//x3
#define ostat 0177564
#define odata 0177566

//какие параметры имеет команда 
#define NO_PARAM 0
#define HAS_SS 1
#define HAS_DD (1 << 1)
#define HAS_XX (1 << 2)
#define HAS_R  (1 << 3)
#define HAS_NN (1 << 4)
#define HAS_R6 (1 << 5)

int nn, rr, xx, z, n;

int R4, R6,rd;
short int N,Z,C,b;


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
void do_br();
void do_beq();
void do_tstb();
void do_bpl();
void do_jmp();
void do_rts();
void do_jsr();
void mem_dump(adr start, word n);
void load_file(char * s);
void test_mem();
void print_reg();
void run();
struct SSDD get_mode(word w);
void NZVC(word x);



struct Command {
	word opcode;
	word mask;
	const char * name;
	void (*do_func)();
	byte param;
			
}	cmdlist[] = {
	{0010000, 0170000, 	"mov",		do_mov, 	HAS_SS | HAS_DD },
	{0060000, 0170000, 	"add",		do_add , 	HAS_SS | HAS_DD },	
	{0000000, 0177777, 	"halt",		do_halt, 	NO_PARAM},
	{0077000, 0177000, "sob",     do_sob,     HAS_NN|HAS_R},
	{0005000, 0177700, 	"clr",		do_clr, 	HAS_DD},
	{0110000, 0170000, 	"movb",		do_movb, 	HAS_SS | HAS_DD},
	{0000400, 0177400, 	"br",		do_br, 		HAS_XX},
	{0001400, 0177400, 	"beq",		do_beq,		HAS_XX},
	{0105700, 0177700,  "tstb",     do_tstb,    HAS_DD},
	{0100000, 0177400,  "bpl",  	do_bpl,     HAS_XX},
	{0000100, 0177700,  "jmp",		do_jmp, 	HAS_DD},
	{0000200, 0177770,  "rts",      do_rts,     HAS_R6},
	{0004000, 0177000, 	"jsr",      do_jsr,    	HAS_R | HAS_DD},
	{0000000, 0170000, 	"unknown", 	do_unknown ,NO_PARAM}	
};




struct SSDD {
	word val;
	adr a;
} ss, dd;


//запись/чтение из памяти 
void b_write(adr a, byte val) {
 //   if (a == odata)
 //       printf(" %c", val);
    if (a < 8)
        reg[a] = ((val>>7) ? (val | 0xFF00) : val);
    else
        mem[a] = val;
}


byte b_read(adr a) {
	return mem[a];
}

word w_read(adr a) {
	return (mem[a + 1] << 8) | mem[a];
}
void w_write(adr a, word val) 
{
    if (a < 8){
        reg[a] = val;
    }
    else{
    	assert(!(a % 2));
        mem[a] = val & 0xff;
        mem[a + 1] = (val >> 8);
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





//функции
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


void do_mov() 
{
    w_write(dd.a, (ss.val & 0xFFFF));
    NZVC(ss.val & 0xFFFF);
    return;
}
void do_add() 
{

    w_write(dd.a, ((dd.val + ss.val) & 0xFFFF));
    NZVC((dd.val + ss.val) & 0xFFFF);
    return;
}

void do_sob()
{
    reg[rr]--;
    if (reg[rr] != 0)
        pc = pc - 2*(nn);
    printf("R%d",rr);
    NZVC(pc);
}

void do_clr()
{
    w_write(dd.a, 0);
    NZVC(0);
}

void do_movb()
{
    b_write(dd.a, ss.val);
    NZVC(ss.val);
}

void do_br()//ветка без условия
{
	pc = (pc + (2 * xx));
}

void do_beq() // ветка с условием
{
	printf(" %.6o", (pc + (2 * xx)) & 0xFFFF);
	if (Z == 1)
		do_br();
}

void do_tstb() {
	C = 0;
	NZVC(dd.val);
}
void do_bpl() 
{
	printf(" %.6o", (pc + (2 * xx)) & 0xFFFF);
	if (N == 0) {
		do_br();
	}
}

void do_jmp() 
{
	pc = dd.a; //переходит в указанный адрес
}
void do_rts()  //смотри 4 презентацию
{
	pc = reg[R6] & 0xFFFF;
	reg[R6] = w_read(sp) & 0xFFFF;
	sp = (sp + 2)  & 0xFFFF;
	printf(" R%d", R6);
}

void do_jsr() { //смотри 4 презентацию 

	sp = (sp - 2) & 0xFFFF; //push
	w_write(sp, reg[rr] & 0xFFFF);
	reg[rr] = pc & 0xFFFF;
	pc = dd.a & 0xFFFF;
	//printf("pc, %06o",pc);
	printf(",R%d", rr);
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
			if ((b) && (nn < 6 ))
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
                printf("#%06o ", res.val);
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
            	if (res.a == ostat) 
            	{
				printf("@#%o", ostat);
				}
				else if (res.a == odata) 
				{
				printf("@#%o", odata);
				}
				else
				{
                printf(" @#%06o ", res.val);
            	}
            }
            reg[nn]+=2;
            break;
        case 4:
            if (b)
            { //уменьшаем значение регистра, интерпретируем его как адрес и находим значение
                reg[nn]--;
            }
            else
            {
                reg[nn]-= 2;
            }
            res.a = reg[nn];
            if(b)
            {
            	res.val = b_read(res.a);
            }
            else
            {
            	res.val = w_read(res.a);
            }
			printf("-(R%d) ", nn);
			assert(res.a < 56*1024 && res.a > 7);
            break;
        case 5:
				reg[nn] -= 2;
				res.a = w_read(reg[nn]);
				if (b) {
					res.val = b_read(res.a);
				} else {
					res.val = w_read(res.a);
				}
				//~ if (n != 7) {
						printf("@-(R%d) ", nn);
				//~ }
				//~ else {
					//~ printf(" #%o ", result.val);
				//~ }
				break;
        case 6:
        		rd = w_read(pc);
				pc += 2;
				res.a = (reg[nn] + rd) & 0177777;
				if (b) {
					res.val = b_read(res.a);
				} else {
					res.val = w_read(res.a);
				}
				if (nn != 7) {
					printf("%.6o(R%o) ", rd, nn);
				}
				else {
					printf(" %.6o ", res.a);
				}
				break;
		case 7:
				rd = w_read(pc);
				pc += 2;
				res.a = w_read(reg[nn]);
				res.a = w_read((res.a + rd) & 0177777);
				if (b) {
					res.val = b_read(res.a);
				} else {
					res.val = w_read(res.a);
				}
				if (n != 7) {
					printf("@%.6o(R%o) ", rd, nn);
				}
				else {
					printf(" @%.6o ", res.a);
				}
				break;	




        default:
            printf("Not implemented");

	}
	return res;
}


void run()
{
	printf("\nRunning \n");
    pc = 01000;
    w_write (ostat, 0xFF);

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
                if (cmd.param & HAS_XX)
                    xx = (char)w ;
                if (cmd.param & HAS_R6)
                	R6 = w & 7;
                //printf("\n");
                cmd.do_func();
                if (strstr(global_argv[1], "-T" ) != 0)
                	print_reg();
                break;
            }
        }
        
        printf("\n");
    }
}

void NZVC(word x)
{
	Z = (x == 0); //флаг нуля 
	if(b)
	{
		N = (x>>7) & 1; //флаг отрицательного байта для байта
		C = (x>>8) & 1; //флаг переполнения байта для байта
	}
	else 
	{
		N = (x>>15) & 1; //флаг отрицательного байта для слова
		C = (x>>16) & 1; //флаг переполнения байта для слова
	}
}

int main(int argc, char * argv[])
{
	global_argv = argv;
	load_file(argv[argc - 1]);
	//int result;
	//result = strcmp(check,argv[1]);
	//printf("sign is %d", result);
	//print_reg();
	run();
	print_reg();

	return 0;
}




