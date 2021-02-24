#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#define MAX_WORD_SIZE 500
#define MAX_INSTRUCTIONS 1000
#define MAX_INSTRUCTION_LENGTH 500
#define MAX_LABELS 100
#define MAX_MEMORY_SIZE 2000
#define MAX_REG_LENGTH 15

typedef struct {
	int address;
	int contents;
}memory_slot;

typedef struct {
	char name[MAX_WORD_SIZE];
	int instruction;
}label;

typedef struct {
	char rs[MAX_REG_LENGTH];
	char rd[MAX_REG_LENGTH];
	char rt[MAX_REG_LENGTH];
	char write_reg[MAX_REG_LENGTH];
	int immediate;
	char label[MAX_WORD_SIZE];
	int shift_amount;
	char op[10];
	int full;
	int instruction;
	int alu_out;
	int lw_data;
	int alu_in1;
	int alu_in2;
	int type; //1=r-type/shift, 2=i-type, 3=lw
}stage;

//Monitors
static int stall=0, type, write_data_reg,hazard_monitor, forward_monitor,forward1,forward2, sw_data, read_data1, alu_out, read_data2, address_monitor, memory_out, write_data_memory, immediate_monitor, alu_in1, alu_in2;
static char pc_monitor[MAX_WORD_SIZE], write_reg_monitor[MAX_REG_LENGTH], write_reg_monitor_wb[MAX_REG_LENGTH], branch_pc[MAX_WORD_SIZE], read_reg1[MAX_REG_LENGTH], read_reg2[MAX_REG_LENGTH], write_reg[MAX_REG_LENGTH];
//Registers
static int zero, pc, r0, at, v0, v1, a0, a1, a2, a3, t0, t1, t2, t3, t4, t5, t6, t7, s0, s1, s2, s3, s4, s5, s6, s7, t8, t9, k0, k1, gp, sp, fp, ra;
//Variables
static int i, next_instruction_is_branch=0, next_branch_instruction, k,shift_amount, instruction_counter = -1, label_counter = -1, current_instruction = 0, end = 0, cycle1, cycle2, branch_instruction, immediate, memory_slots_used = -1, current_cycle;
static char ch, word[MAX_WORD_SIZE], instructions[MAX_INSTRUCTIONS][MAX_INSTRUCTION_LENGTH], opcode[10], rs[MAX_REG_LENGTH], rt[MAX_REG_LENGTH], rd[MAX_REG_LENGTH], branch_label[MAX_WORD_SIZE];
static label labels[MAX_LABELS];
static memory_slot memory[MAX_MEMORY_SIZE];
FILE* output, *filepointer;
time_t start, endt;
stage i_f, id, ex, mem, wb;

int add(int a, int b) {
	return (a + b);
}

int addi(int a, int immediate) {
	return (a + immediate);
}

int addiu(int a, int immediate) {
	return (a + abs(immediate));
}

int addu(int a, int b) {
	return (a + abs(b));
}

int and (int a, int b) {
	return (a & b);
}

int andi (int a, int immediate) {
	return (a & immediate);
}

bool beq(int a, int b) {
	if (a == b)
		return true;
	else
		return false;
}

bool bne(int a, int b) {
	if (a != b)
		return true;
	else
		return false;
}

long lw(int address, int offset) {
	address = address + offset;
	int o;
	for (o = 0; o <= memory_slots_used; o++) {
		if (memory[o].address == address) {
			return memory[o].contents;
		}
	}
}

int nor(int a, int b) {
	return (~(a | b));
}

int or(int a, int b) {
	return (a | b);
}

int ori (int a, int immediate) {
	return (a | immediate);
}

int slt(int a, int b) {
	if (a < b)
		return 1;
	else
		return 0;
}

int slti(int a, int immediate) {
	if (a < immediate)
		return 1;
	else
		return 0;
}

int sltiu(int a, int immediate) {
	if (a < abs(immediate))
		return 1;
	else
		return 0;
}

int sltu(int a, int b) {
	if (a < abs(b))
		return 1;
	else
		return 0;
}

int sll(int a, int amount) {
	return (a << amount);
}

int srl(int a, int amount) {
	return (a >> amount);
}

void sw(int value, int address, int offset) {
	address = address + offset;
	int p, found=0;
	for (p = 0; p <= memory_slots_used; p++) {
		if (memory[p].address == address) {
			found = 1;
			memory[p].contents = value;
			break;
		}
	}
	if (found == 0) {
		memory_slots_used++;
		memory[memory_slots_used].address = address;
		memory[memory_slots_used].contents = value;
	}
}

int sub(int a, int b) {
	return (a - b);
}

int subu(int a, int b) {
	return (a - abs(b));
}

void swap(int a, int b) { //xrisimopiite sto sortmemory
	memory_slot temp;
	temp.address = memory[a].address;
	temp.contents = memory[a].contents;
	memory[a].address = memory[b].address;
	memory[a].contents = memory[b].contents;
	memory[b].address = temp.address;
	memory[b].contents = temp.contents;
}

void sortmemory() {
	bool swapped=true;
	int u;
	while (swapped == true) {
		swapped = false;
		for (u = 0; u < memory_slots_used-1; u++) {
			if (memory[u].address > memory[u + 1].address) {
				swap(u, u + 1);
				swapped = true;
			}
		}
	}
}

int register_read(char reg[MAX_REG_LENGTH]) {
	if (strcmp(reg, "$zero") == 0) return zero;
	else if (strcmp(reg, "$pc") == 0) return pc;
	else if (strcmp(reg, "$r0") == 0) return r0;
	else if (strcmp(reg, "$at") == 0) return at;
	else if (strcmp(reg, "$v0") == 0) return v0;
	else if (strcmp(reg, "$v1") == 0) return v1;
	else if (strcmp(reg, "$a0") == 0) return a0;
	else if (strcmp(reg, "$a1") == 0) return a1;
	else if (strcmp(reg, "$a2") == 0) return a2;
	else if (strcmp(reg, "$a3") == 0) return a3;
	else if (strcmp(reg, "$t0") == 0) return t0;
	else if (strcmp(reg, "$t1") == 0) return t1;
	else if (strcmp(reg, "$t2") == 0) return t2;
	else if (strcmp(reg, "$t3") == 0) return t3;
	else if (strcmp(reg, "$t4") == 0) return t4;
	else if (strcmp(reg, "$t5") == 0) return t5;
	else if (strcmp(reg, "$t6") == 0) return t6;
	else if (strcmp(reg, "$t7") == 0) return t7;
	else if (strcmp(reg, "$s0") == 0) return s0;
	else if (strcmp(reg, "$s1") == 0) return s1;
	else if (strcmp(reg, "$s2") == 0) return s2;
	else if (strcmp(reg, "$s3") == 0) return s3;
	else if (strcmp(reg, "$s4") == 0) return s4;
	else if (strcmp(reg, "$s5") == 0) return s5;
	else if (strcmp(reg, "$s6") == 0) return s6;
	else if (strcmp(reg, "$s7") == 0) return s7;
	else if (strcmp(reg, "$t8") == 0) return t8;
	else if (strcmp(reg, "$t9") == 0) return t9;
	else if (strcmp(reg, "$k0") == 0) return k0;
	else if (strcmp(reg, "$k1") == 0) return k1;
	else if (strcmp(reg, "$gp") == 0) return gp;
	else if (strcmp(reg, "$sp") == 0) return sp;
	else if (strcmp(reg, "$fp") == 0) return fp;
	else if (strcmp(reg, "$ra") == 0) return ra;
}

void register_write(char reg[MAX_REG_LENGTH], int value) {
	if (strcmp(reg, "$zero") == 0) zero = value;
	else if (strcmp(reg, "$pc") == 0) pc = value;
	else if (strcmp(reg, "$r0") == 0) r0 = value;
	else if (strcmp(reg, "$at") == 0) at = value;
	else if (strcmp(reg, "$v0") == 0) v0 = value;
	else if (strcmp(reg, "$v1") == 0) v1 = value;
	else if (strcmp(reg, "$a0") == 0) a0 = value;
	else if (strcmp(reg, "$a1") == 0) a1 = value;
	else if (strcmp(reg, "$a2") == 0) a2 = value;
	else if (strcmp(reg, "$a3") == 0) a3 = value;
	else if (strcmp(reg, "$t0") == 0) t0 = value;
	else if (strcmp(reg, "$t1") == 0) t1 = value;
	else if (strcmp(reg, "$t2") == 0) t2 = value;
	else if (strcmp(reg, "$t3") == 0) t3 = value;
	else if (strcmp(reg, "$t4") == 0) t4 = value;
	else if (strcmp(reg, "$t5") == 0) t5 = value;
	else if (strcmp(reg, "$t6") == 0) t6 = value;
	else if (strcmp(reg, "$t7") == 0) t7 = value;
	else if (strcmp(reg, "$s0") == 0) s0 = value;
	else if (strcmp(reg, "$s1") == 0) s1 = value;
	else if (strcmp(reg, "$s2") == 0) s2 = value;
	else if (strcmp(reg, "$s3") == 0) s3 = value;
	else if (strcmp(reg, "$s4") == 0) s4 = value;
	else if (strcmp(reg, "$s5") == 0) s5 = value;
	else if (strcmp(reg, "$s6") == 0) s6 = value;
	else if (strcmp(reg, "$s7") == 0) s7 = value;
	else if (strcmp(reg, "$t8") == 0) t8 = value;
	else if (strcmp(reg, "$t9") == 0) t9 = value;
	else if (strcmp(reg, "$k0") == 0) k0 = value;
	else if (strcmp(reg, "$k1") == 0) k1 = value;
	else if (strcmp(reg, "$gp") == 0) gp = value;
	else if (strcmp(reg, "$sp") == 0) sp = value;
	else if (strcmp(reg, "$fp") == 0) fp = value;
	else if (strcmp(reg, "$ra") == 0) ra = value;
}

int hexToDec(char hexVal[]){
	int len = strlen(hexVal);
	// Initializing base value to 1, i.e 16^0 
	int base = 1;
	int dec_val = 0;
	// Extracting characters as digits from last character 
	for (int i = len - 1; i >= 0; i--){
		// if character lies in '0'-'9', converting  
		// it to integral 0-9 by subtracting 48 from 
		// ASCII value. 
		if (hexVal[i] >= '0' && hexVal[i] <= '9'){
			dec_val += (hexVal[i] - 48) * base;
			// incrementing base by power 
			base = base * 16;
		}
		// if character lies in 'A'-'F' , converting  
		// it to integral 10 - 15 by subtracting 55  
		// from ASCII value 
		else if (hexVal[i] >= 'A' && hexVal[i] <= 'F'){
			dec_val += (hexVal[i] - 55) * base;
			// incrementing base by power 
			base = base * 16;
		}
		else if (hexVal[i] >= 'a' && hexVal[i] <= 'f') {
			dec_val += (hexVal[i] - 87) * base;
			// incrementing base by power 
			base = base * 16;
		}
	}
	return dec_val;
}

int label_decode(char labell[MAX_WORD_SIZE]) {
	for (int k = 0; k < MAX_LABELS; k++) {
		if (strcmp(labels[k].name, labell) == 0) {
			return labels[k].instruction;
			break;
		}
	}
	printf("\nERROR: Wrong Label (%s)\n",labell);
}

void print_registers_hex() {
	printf("\nREGISTERS: (Hex)\n\n");
	printf("\nPC= %X\tr0= %X\tat= %X\tv0= %X\tv1= %X\ta0= %X\t", pc, r0, at, v0, v1, a0);
	printf("\na1= %X\ta2= %X\ta3= %X\tt0= %X\tt1= %X\tt2= %X\t", a1, a2, a3, t0, t1, t2);
	printf("\nt3= %X\tt4= %X\tt5= %X\tt6= %X\tt7= %X\ts0= %X\t", t3, t4, t5, t6, t7, s0);
	printf("\ns1= %X\ts2= %X\ts3= %X\ts4= %X\ts5= %X\ts6= %X\t", s1, s2, s3, s4, s5, s6);
	printf("\ns7= %X\tt8= %X\tt9= %X\tk0= %X\tk1= %X\tgp= %X\t", s7, t8, t9, k0, k1, gp);
	printf("\nsp= %X\tfp= %X\tra= %X", sp, fp, ra);
}

void print_registers_dec() {
	printf("\nREGISTERS: (Dec)\n\n");
	printf("\nPC= %d\tr0= %d\tat= %d\tv0= %d\tv1= %d\ta0= %d\t", pc, r0, at, v0, v1, a0);
	printf("\na1= %d\ta2= %d\ta3= %d\tt0= %d\tt1= %d\tt2= %d\t", a1, a2, a3, t0, t1, t2);
	printf("\nt3= %d\tt4= %d\tt5= %d\tt6= %d\tt7= %d\ts0= %d\t", t3, t4, t5, t6, t7, s0);
	printf("\ns1= %d\ts2= %d\ts3= %d\ts4= %d\ts5= %d\ts6= %d\t", s1, s2, s3, s4, s5, s6);
	printf("\ns7= %d\tt8= %d\tt9= %d\tk0= %d\tk1= %d\tgp= %d\t", s7, t8, t9, k0, k1, gp);
	printf("\nsp= %d\tfp= %d\tra= %d", sp, fp, ra);
}

void print_memory() {
	printf("\t\tMEMORY\n\n");
	for (int w = 0; w <= memory_slots_used; w++) {
		printf("Address: %X\t\tContents (Decimal): %d\tContents (Hex): %X\n", memory[w].address, memory[w].contents, memory[w].contents);
	}
}

void export_cycle_info(int cycle) {
	sortmemory();
	fprintf(output, "-----Cycle %d-----\nRegisters:\n", cycle);
	fprintf(output, "%s\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x", pc_monitor, r0, at, v0, v1, a0, a1, a2, a3, t0, t1, t2, t3, t4, t5, t6, t7, s0, s1, s2, s3, s4, s5, s6, s7, t8, t9, k0, k1, gp, sp, fp, ra);
	fprintf(output, "\n\nMonitors:\n");
	fprintf(output, "%s\t%x\t", pc_monitor,pc);
	if (i_f.full == 1)
		fprintf(output, "%s\t", instructions[i_f.instruction]);
	else
		fprintf(output, "-\t");
	fprintf(output, "%s\t%s\t%s\t%s\t", branch_pc, read_reg1, read_reg2, write_reg);
	if (write_data_reg==INT_MAX)
		fprintf(output, "-\t");
	else
		fprintf(output, "%x\t", write_data_reg);
	if (read_data1 == INT_MAX)
		fprintf(output, "-\t");
	else
		fprintf(output, "%x\t", read_data1);
	if (read_data2 == INT_MAX)
		fprintf(output, "-\t");
	else
		fprintf(output, "%x\t", read_data2);
	if (immediate_monitor == INT_MAX)
		fprintf(output, "-\t");
	else
		fprintf(output, "%x\t", immediate_monitor);
	fprintf(output, "%s\t%s\t%s\t%s\t", read_reg1, read_reg2, read_reg2, id.rd);
	if (alu_in1 == INT_MAX)
		fprintf(output, "-\t");
	else
		fprintf(output, "%x\t", alu_in1);
	if (alu_in2 == INT_MAX)
		fprintf(output, "-\t");
	else
		fprintf(output, "%x\t", alu_in2);
	if (alu_out == INT_MAX)
		fprintf(output, "-\t");
	else
		fprintf(output, "%x\t", alu_out);
	if (sw_data == INT_MAX)
		fprintf(output, "-\t");
	else
		fprintf(output, "%x\t", sw_data);
	if (address_monitor == INT_MAX)
		fprintf(output, "-\t");
	else
		fprintf(output, "%x\t", address_monitor);
	if (write_data_memory == INT_MAX)
		fprintf(output, "-\t");
	else
		fprintf(output, "%x\t", write_data_memory);
	if (memory_out == INT_MAX)
		fprintf(output, "-\t");
	else
		fprintf(output, "%x\t", memory_out);
	if (mem.alu_out == INT_MAX)
		fprintf(output, "-\t");
	else
		fprintf(output, "%x\t", mem.alu_out);
	fprintf(output, "%s\t%s\t", write_reg_monitor, write_reg);
	if (write_data_reg == INT_MAX)
		fprintf(output, "-\t");
	else
		fprintf(output, "%x\t", write_data_reg);
	fprintf(output, "%d\t%d", hazard_monitor,forward_monitor);
	fprintf(output, "\n\nMemory State:\n");
	for (int r = 0; r <= memory_slots_used; r++) {
		if (r != memory_slots_used) //gia na min tiponete extra tab sto telos tin grammis
			fprintf(output, "%x\t", memory[r].contents);
		else
			fprintf(output, "%x", memory[r].contents);
	}
	fprintf(output, "\n\nPipeline Stages:\n");
	if (i_f.full == 0)
		fprintf(output, "-\t");
	else
		fprintf(output, "%s\t", instructions[i_f.instruction]);
	if (id.full == 0)
		fprintf(output, "-\t");
	else
		fprintf(output, "%s\t", instructions[id.instruction]);
	if (ex.full == 0)
		fprintf(output, "-\t");
	else
		fprintf(output, "%s\t", instructions[ex.instruction]);
	if (mem.full == 0)
		fprintf(output, "-\t");
	else
		fprintf(output, "%s\t", instructions[mem.instruction]);
	if (wb.full == 0)
		fprintf(output, "-");
	else
		fprintf(output, "%s", instructions[wb.instruction]);
	fprintf(output, "\n\n");
}

void export_final_info() {
	sortmemory();
	fprintf(output, "-----Final State-----\nRegisters:\n");
	fprintf(output, "%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x", pc, r0, at, v0, v1, a0, a1, a2, a3, t0, t1, t2, t3, t4, t5, t6, t7, s0, s1, s2, s3, s4, s5, s6, s7, t8, t9, k0, k1, gp, sp, fp, ra);
	fprintf(output, "\n\nMemory State:\n");
	for (int r = 0; r <= memory_slots_used; r++) {
		if (r != memory_slots_used) //gia na min tiponete extra tab sto telos tin grammis
			fprintf(output, "%x\t", memory[r].contents);
		else
			fprintf(output, "%x", memory[r].contents);
	}
	fprintf(output, "\n\nTotal Cycles:\n%d",current_cycle);
	fprintf(output, "\n\nTotal Execution Time:\n%.0f", (float)((float)(endt - start) / CLOCKS_PER_SEC)*1000000000);
	fclose(output);
}

void parser() {
	while ((ch = getc(filepointer)) != EOF) {
		switch (ch) {
		case '.':
			fscanf(filepointer, "%s", word);
			if (strcmp(word, "data") == 0) { //memory begins here
				break;
			}
			if (strcmp(word, "text") == 0) { //code begins here
				fscanf(filepointer, "%s", word);
				while (word != EOF) {
					switch (word[0]) {
					case '\n':
						fscanf(filepointer, "%s", word);
						break;
					case '#': //ignore comment line
						while ((ch = getc(filepointer)) != '\n') {

						}
						fscanf(filepointer, "%s", word);
						break;
					default: //this line contains "main:", a comment or a label.
						if (word[0] == '#') { //ignore comment line

							break;
						}
						if (strchr(word, ':') != NULL) { //if this is true, it's a label
							label_counter++;
							labels[label_counter].instruction = instruction_counter + 1;
							i = 0;
							while (word[i] != ':') { //copy the label name
								labels[label_counter].name[i] = word[i];
								i++;
							}
							labels[label_counter].name[i] = '\0';
							printf("\n\nLabel %d is %s and points to instruction %d.\n", label_counter, labels[label_counter].name, labels[label_counter].instruction);
							fscanf(filepointer, "%s", word);
							break;
						}
						//else, it's an instruction
						i = 0;
						instruction_counter++;
						while (word[i] != '\0') { //copy the first word of the instruction
							instructions[instruction_counter][i] = word[i];
							i++;
						}
						while ((ch = getc(filepointer)) != '\n') {
							if ((ch == '#') || (ch == '\t')) { //if we find a comment, ignore it
								while (((ch = getc(filepointer)) != '\n') && (ch != EOF)) {
								}
								break;
							}
							if (ch == EOF) { //if we find the end of file
								break;
							}
							instructions[instruction_counter][i] = ch;
							i++;
						}
						instructions[instruction_counter][i] = '\0';
						printf("\nInstruction %d is: %s", instruction_counter, instructions[instruction_counter]);
						fscanf(filepointer, "%s", word);
						break;
					}
					if (ch == EOF) { //if we find the end of file
						break;
					}
				}
				if (ch == EOF) { //if we find the end of file
					break;
				}
			}
			break;
		case '#': //ignore comment line
			while ((ch = getc(filepointer)) != '\n') {

			}
			break;
		case ('\t' || ' '):
			ch = getc(filepointer);
			break;
		}
		if (ch == EOF) { //if we find the end of file
			break;
		}
	}
	fclose(filepointer);
}

void decode_r_type() {
	while (instructions[current_instruction][i] != '$') { //mexri na ftasoume ston rd
		i++;
	}
	k = 0;
	while ((instructions[current_instruction][i] != ' ') && (instructions[current_instruction][i] != ',')) { //diavazume ton rd
		rd[k] = instructions[current_instruction][i];
		i++;
		k++;
	}
	rd[k] = '\0';
	while (instructions[current_instruction][i] != '$') { //mexri na ftasoume ston rs
		i++;
	}
	k = 0;
	while ((instructions[current_instruction][i] != ' ') && (instructions[current_instruction][i] != ',')) { //diavazume ton rs
		rs[k] = instructions[current_instruction][i];
		i++;
		k++;
	}
	rs[k] = '\0';
	while (instructions[current_instruction][i] != '$') { //mexri na ftasoume ston rt
		i++;
	}
	k = 0;

	while ((instructions[current_instruction][i] != '\0') && (instructions[current_instruction][i] != ' ') && (instructions[current_instruction][i] != '\n') && (instructions[current_instruction][i] != '\t')) { //diavazume ton rt
		rt[k] = instructions[current_instruction][i];
		i++;
		k++;
	}
	rt[k] = '\0';
}

void decode_i_type() {
	while (instructions[current_instruction][i] != '$') { //mexri na ftasoume ston rt
		i++;
	}
	k = 0;
	while ((instructions[current_instruction][i] != ' ') && (instructions[current_instruction][i] != ',')) { //diavazume ton rt
		rt[k] = instructions[current_instruction][i];
		i++;
		k++;
	}
	rt[k] = '\0';
	while (instructions[current_instruction][i] != '$') { //mexri na ftasoume ston rs
		i++;
	}
	k = 0;
	while ((instructions[current_instruction][i] != ' ') && (instructions[current_instruction][i] != ',')) { //diavazume ton rs
		rs[k] = instructions[current_instruction][i];
		i++;
		k++;
	}
	rs[k] = '\0';
	while ((instructions[current_instruction][i] == ' ') || (instructions[current_instruction][i] == ',')) { //mexri na ftasoume sto immediate
		i++;
	}
	k = 0;
	while ((instructions[current_instruction][i] != ' ') && (instructions[current_instruction][i] != '\n') && (instructions[current_instruction][i] != '\t') && (instructions[current_instruction][i] != '\0')) { //diavazume to immediate
		rd[k] = instructions[current_instruction][i];
		i++;
		k++;
	}
	rd[k] = '\0';
	if ((rd[0] == '0') && (rd[1] == 'x'))
		immediate = hexToDec(rd);
	else
		immediate = atoi(rd);
}

void decode_branch() {
	while (instructions[current_instruction][i] != '$') { //mexri na ftasoume ston rs
		i++;
	}
	k = 0;
	while ((instructions[current_instruction][i] != ' ') && (instructions[current_instruction][i] != ',')) { //diavazume ton rs
		rs[k] = instructions[current_instruction][i];
		i++;
		k++;
	}
	rs[k] = '\0';
	while (instructions[current_instruction][i] != '$') { //mexri na ftasoume ston rt
		i++;
	}
	k = 0;
	while ((instructions[current_instruction][i] != ' ') && (instructions[current_instruction][i] != ',')) { //diavazume ton rt
		rt[k] = instructions[current_instruction][i];
		i++;
		k++;
	}
	rt[k] = '\0';
	while ((instructions[current_instruction][i] == ' ') || (instructions[current_instruction][i] == ',')) { //mexri na ftasoume sto label
		i++;
	}
	k = 0;
	while ((instructions[current_instruction][i] != '\0') && (instructions[current_instruction][i] != ' ') && (instructions[current_instruction][i] != '\n') && (instructions[current_instruction][i] != '\t') && (instructions[current_instruction][i] != '\0')) { //diavazume to label
		word[k] = instructions[current_instruction][i];
		i++;
		k++;
	}
	word[k] = '\0';
}

void decode_memory() {
	while (instructions[current_instruction][i] != '$') { //mexri na ftasoume ston rt
		i++;
	}
	k = 0;
	while ((instructions[current_instruction][i] != ' ') && (instructions[current_instruction][i] != ',')) { //diavazume ton rt
		rt[k] = instructions[current_instruction][i];
		i++;
		k++;
	}
	rt[k] = '\0';
	while ((instructions[current_instruction][i] == ' ') || (instructions[current_instruction][i] == ',')) { //mexri na ftasoume sto offset
		i++;
	}
	k = 0;
	while (instructions[current_instruction][i] != '(') { //diavazume to offset
		rs[k] = instructions[current_instruction][i];
		i++;
		k++;
	}
	rs[k] = '\0';
	if ((rs[0] == '0') && (rs[1] == 'x'))
		immediate = hexToDec(rs); //offset
	else
		immediate = atoi(rs); //offset
	i++; //gia na ftasoume sto base
	k = 0;
	while (instructions[current_instruction][i] != ')') { //diavazume to base
		rs[k] = instructions[current_instruction][i];
		i++;
		k++;
	}
	rs[k] = '\0';
}

void decode_shift() {
	while (instructions[current_instruction][i] != '$') { //mexri na ftasoume ston rd
		i++;
	}
	k = 0;
	while ((instructions[current_instruction][i] != ' ') && (instructions[current_instruction][i] != ',')) { //diavazume ton rd
		rd[k] = instructions[current_instruction][i];
		i++;
		k++;
	}
	rd[k] = '\0';
	while (instructions[current_instruction][i] != '$') { //mexri na ftasoume ston rt
		i++;
	}
	k = 0;
	while ((instructions[current_instruction][i] != ' ') && (instructions[current_instruction][i] != ',')) { //diavazume ton rt
		rt[k] = instructions[current_instruction][i];
		i++;
		k++;
	}
	rt[k] = '\0';
	while ((instructions[current_instruction][i] == ' ') || (instructions[current_instruction][i] == ',')) { //mexri na ftasoume sto shift amount
		i++;
	}
	k = 0;
	while ((instructions[current_instruction][i] != '\0') && (instructions[current_instruction][i] != ' ') && (instructions[current_instruction][i] != '\n') && (instructions[current_instruction][i] != '\t') && (instructions[current_instruction][i] != '\0')) { //diavazume to shift amount
		rs[k] = instructions[current_instruction][i];
		i++;
		k++;
	}
	rs[k] = '\0';
	if ((rs[0] == '0') && (rs[1] == 'x'))
		immediate = hexToDec(rs);
	else
		immediate = atoi(rs);
}

void print_pipeline_info() {
	printf("\nPIPELINE STATE AT CYCLE %d:", current_cycle);
	if (i_f.full == 1)
		printf("\nIF: %s |rs:%s rt:%s rd:%s imm(dec):%d label:%s shift amount:%d alu_out:%d lw_data(dec):%d", instructions[i_f.instruction], i_f.rs, i_f.rt, i_f.rd, i_f.immediate, i_f.label, i_f.shift_amount, i_f.alu_out, i_f.lw_data);
	else
		printf("\nIF is empty!");
	if (id.full == 1)
		printf("\nID: %s |rs:%s rt:%s rd:%s imm(dec):%d label:%s shift amount:%d alu_out:%d lw_data(dec):%d", instructions[id.instruction], id.rs, id.rt, id.rd, id.immediate, id.label, id.shift_amount, id.alu_out, id.lw_data);
	else
		printf("\nID is empty!");
	if (ex.full == 1)
		printf("\nEX: %s |rs:%s rt:%s rd:%s imm(dec):%d label:%s shift amount:%d alu_out:%d lw_data(dec):%d", instructions[ex.instruction], ex.rs, ex.rt, ex.rd, ex.immediate, ex.label, ex.shift_amount, ex.alu_out, ex.lw_data);
	else
		printf("\nEX is empty!");
	if (mem.full == 1)
		printf("\nMEM: %s |rs:%s rt:%s rd:%s imm(dec):%d label:%s shift amount:%d alu_out:%d lw_data(dec):%d", instructions[mem.instruction], mem.rs, mem.rt, mem.rd, mem.immediate, mem.label, mem.shift_amount, mem.alu_out, mem.lw_data);
	else
		printf("\nMEM is empty!");
	if (wb.full == 1)
		printf("\nWB: %s |rs:%s rt:%s rd:%s imm(dec):%d label:%s shift amount:%d alu_out:%d lw_data(dec):%d\n", instructions[wb.instruction], wb.rs, wb.rt, wb.rd, wb.immediate, wb.label, wb.shift_amount, wb.alu_out, wb.lw_data);
	else
		printf("\nWB is empty!\n");
}

void check_cycle_print() {
	if ((current_cycle == cycle1) || (current_cycle == cycle2)) {
		export_cycle_info(current_cycle);
	}
	//print_pipeline_info();
}

void advance_mem_wb() {
	if (((mem.full == 1) && (stall != 2)) || ((stall == 2) && (wb.full == 1))) { //Proxora i entoli pu einai sto mem stage sto wb stage
		if (stall!=2)
			wb = mem;
		wb.full = 1;
		if ((strcmp(wb.op, "add") == 0) || (strcmp(wb.op, "addu") == 0) || (strcmp(wb.op, "and") == 0) || (strcmp(wb.op, "nor") == 0) || (strcmp(wb.op, "or") == 0) || (strcmp(wb.op, "slt") == 0) || (strcmp(wb.op, "sltu") == 0) || (strcmp(wb.op, "sub") == 0) || (strcmp(wb.op, "subu") == 0)) {
			//an einai r-type
			register_write(wb.rd, wb.alu_out);
			write_data_reg = register_read(wb.rd);
			strcpy(write_reg, wb.rd);
			strcpy(write_reg_monitor_wb, write_reg_monitor); //monitor 25
		}
		else if ((strcmp(wb.op, "addi") == 0) || (strcmp(wb.op, "addiu") == 0) || (strcmp(wb.op, "andi") == 0) || (strcmp(wb.op, "ori") == 0) || (strcmp(wb.op, "slti") == 0) || (strcmp(wb.op, "sltiu") == 0)) {
			//an einai i-type
			register_write(wb.rt, wb.alu_out);
			write_data_reg = register_read(wb.rt);
			strcpy(write_reg, wb.rt);
			strcpy(write_reg_monitor_wb, write_reg_monitor); //monitor 25
		}
		else if ((strcmp(wb.op, "beq") == 0) || (strcmp(wb.op, "bne") == 0)) {
			//an einai branch
			write_data_reg = INT_MAX;
			strcpy(write_reg, "-");
			strcpy(write_reg_monitor_wb, "-"); //monitor 25
		}
		else if ((strcmp(wb.op, "sll") == 0) || (strcmp(wb.op, "srl") == 0)) {
			//an einai shift
			register_write(wb.rd, wb.alu_out);
			write_data_reg = register_read(wb.rd);
			strcpy(write_reg, wb.rd);
			strcpy(write_reg_monitor_wb, write_reg_monitor); //monitor 25
		}
		else if (strcmp(wb.op, "lw") == 0) {
			//an einai lw
			register_write(wb.rt, wb.lw_data);
			write_data_reg = register_read(wb.rt);
			strcpy(write_reg, wb.rt);
			strcpy(write_reg_monitor_wb, write_reg_monitor); //monitor 25
		}
		else if (strcmp(wb.op, "sw") == 0) {
			//an einai sw
			write_data_reg = INT_MAX;
			strcpy(write_reg, "-");
			strcpy(write_reg_monitor_wb, "-"); //monitor 25
		}
	}
	else {
		wb.full = 0;
		wb.instruction = INT_MAX;
		write_data_reg = INT_MAX;
		strcpy(write_reg, "-");
		strcpy(write_reg_monitor_wb, "-");
	}
}

void advance_ex_mem() {
	if (((ex.full == 1) && (stall != 2)) || ((stall == 2) && (mem.full == 1))) { //Proxora i entoli pu einai sto ex stage sto mem stage
		if (stall != 2) { //gia na min proxora i entoli an proigoumenos ixa stall
			mem = ex;
			mem.full = 1;
		}
		if ((strcmp(mem.op, "add") == 0) || (strcmp(mem.op, "addu") == 0) || (strcmp(mem.op, "and") == 0) || (strcmp(mem.op, "nor") == 0) || (strcmp(mem.op, "or") == 0) || (strcmp(mem.op, "slt") == 0) || (strcmp(mem.op, "sltu") == 0) || (strcmp(mem.op, "sub") == 0) || (strcmp(mem.op, "subu") == 0)) {
			//an einai r-type
			address_monitor = INT_MAX;
			write_data_memory = INT_MAX;
			memory_out = INT_MAX;
			//MONITOR 23 is mem.alu_out
			strcpy(write_reg_monitor, mem.rd);
		}
		else if ((strcmp(mem.op, "addi") == 0) || (strcmp(mem.op, "addiu") == 0) || (strcmp(mem.op, "andi") == 0) || (strcmp(mem.op, "ori") == 0) || (strcmp(mem.op, "slti") == 0) || (strcmp(mem.op, "sltiu") == 0)) {
			//an einai i-type
			address_monitor = INT_MAX;
			write_data_memory = INT_MAX;
			memory_out = INT_MAX;
			strcpy(write_reg_monitor, mem.rt);
		}
		else if ((strcmp(mem.op, "beq") == 0) || (strcmp(mem.op, "bne") == 0)) {
			//an einai branch
			address_monitor = INT_MAX;
			write_data_memory = INT_MAX;
			memory_out = INT_MAX;
			strcpy(write_reg_monitor, "-");
		}
		else if ((strcmp(mem.op, "sll") == 0) || (strcmp(mem.op, "srl") == 0)) {
			//an einai shift
			address_monitor = INT_MAX;
			write_data_memory = INT_MAX;
			memory_out = INT_MAX;
			strcpy(write_reg_monitor, mem.rd);
		}
		else if (strcmp(mem.op, "lw") == 0) {
			//an einai lw
			address_monitor = mem.alu_out;
			write_data_memory = INT_MAX;
			memory_out = lw(address_monitor, 0); //0 epidi egine idi i praksi sto proigumeni stage
			mem.lw_data = memory_out;
			strcpy(write_reg_monitor, mem.rt);
		}
		else if (strcmp(mem.op, "sw") == 0) {
			//an einai sw
			address_monitor = mem.alu_out;
			write_data_memory = sw_data;
			memory_out = INT_MAX;
			sw(sw_data, alu_out, 0); //0 epidi egine idi i praksi sto proigumeni stage
			strcpy(write_reg_monitor, "-");
		}
	}
	else {
		mem.full = 0;
		mem.lw_data = INT_MAX;
		mem.alu_out = INT_MAX;
		mem.instruction = INT_MAX;
		address_monitor = INT_MAX;
		write_data_memory = INT_MAX;
		memory_out = INT_MAX;
		strcpy(write_reg_monitor, "-");
	}
}

void advance_pipeline(int add_instruction) {
	current_cycle++;
	hazard_monitor = 0;
	forward_monitor = 0;
	forward1 = 0;
	forward2 = 0;
	advance_mem_wb();
	advance_ex_mem();
	if (((id.full == 1) && (stall!=2)) || ((stall == 2) && (ex.full == 1))) { //Proxora i entoli pu einai sto id stage sto ex stage
		if (stall != 2) //gia na min proxora i entoli an proigoumenos ixa stall
			ex = id;
		ex.full = 1;
		if ((strcmp(ex.op, "add") == 0) || (strcmp(ex.op, "addu") == 0) || (strcmp(ex.op, "and") == 0) || (strcmp(ex.op, "nor") == 0) || (strcmp(ex.op, "or") == 0) || (strcmp(ex.op, "slt") == 0) || (strcmp(ex.op, "sltu") == 0) || (strcmp(ex.op, "sub") == 0) || (strcmp(ex.op, "subu") == 0)) {
			//an einai r-type
			//FORWARDING UNIT
			if (strcmp(mem.write_reg,ex.rs)==0) { //an o write reg sto mem einai idios me ton rs sto ex
				if (mem.type==3) { //an einai lw (load-use hazard)
					stall = 1;
					hazard_monitor = 1;
				}
				else{ //den einai load-use, ara kanoume forward
					alu_in1 = mem.alu_out;
					forward_monitor = 1;
					forward1 = 1;
				}
			}
			else if (strcmp(wb.write_reg, ex.rs) == 0) { //an o write reg sto wb einai idios me ton rs
				if (wb.type == 3)//an einai lw
					alu_in1 = wb.lw_data;
				else
					alu_in1 = wb.alu_out;
				forward_monitor = 1;
				forward1 = 1;
			}
			if (strcmp(mem.write_reg, ex.rt) == 0) { //an o write reg sto mem einai idios me ton rt
				if (mem.type == 3) { //an einai lw (load-use hazard)
					stall = 1;
					hazard_monitor = 1;
				}
				else { //den einai load-use, ara kanoume forward
					alu_in2 = mem.alu_out;
					forward_monitor = 1;
					forward2 = 1;
				}
			}
			else if (strcmp(wb.write_reg, ex.rt) == 0) { //an o write reg sto wb einai idios me ton rt
				if (wb.type==3)//an einai lw
					alu_in2 = wb.lw_data;
				else
					alu_in2 = wb.alu_out;
				forward_monitor = 1;
				forward2 = 1;
			}
			if (forward1==0)
				alu_in1 = ex.alu_in1;
			if (forward2==0)
				alu_in2 = ex.alu_in2;
			if (strcmp(ex.op, "add") == 0)
				alu_out = add(alu_in1, alu_in2);
			else if (strcmp(ex.op, "addu") == 0)
				alu_out = addu(alu_in1, alu_in2);
			else if (strcmp(ex.op, "and") == 0)
				alu_out = and(alu_in1, alu_in2);
			else if (strcmp(ex.op, "nor") == 0)
				alu_out = nor(alu_in1, alu_in2);
			else if (strcmp(ex.op, "or") == 0)
				alu_out = or(alu_in1, alu_in2);
			else if (strcmp(ex.op, "slt") == 0)
				alu_out = slt(alu_in1, alu_in2);
			else if (strcmp(ex.op, "sltu") == 0)
				alu_out = sltu(alu_in1, alu_in2);
			else if (strcmp(ex.op, "sub") == 0)
				alu_out = sub(alu_in1, alu_in2);
			else if (strcmp(ex.op, "subu") == 0)
				alu_out = subu(alu_in1, alu_in2);
			ex.alu_out = alu_out;
			sw_data = INT_MAX;
		}
		else if ((strcmp(ex.op, "addi") == 0) || (strcmp(ex.op, "addiu") == 0) || (strcmp(ex.op, "andi") == 0) || (strcmp(ex.op, "ori") == 0) || (strcmp(ex.op, "slti") == 0) || (strcmp(ex.op, "sltiu") == 0)) {
			//an einai i-type
			//FORWARDING UNIT
			if (strcmp(mem.write_reg, ex.rs) == 0) { //an o write reg sto mem einai idios me ton rs
				if (mem.type == 3) { //an einai lw (load-use hazard)
					stall = 1;
					hazard_monitor = 1;
				}
				else { //den einai load-use, ara kanoume forward
					alu_in1 = mem.alu_out;
					forward_monitor = 1;
					forward1 = 1;
				}
			}
			else if (strcmp(wb.write_reg, ex.rs) == 0) { //an o write reg sto wb einai idios me ton rs
				alu_in1 = wb.alu_out;
				forward_monitor = 1;
				forward1 = 1;
			}
			if (forward1==0)
				alu_in1 = ex.alu_in1;
			alu_in2 = ex.immediate;
			if (strcmp(ex.op, "addi") == 0)
				alu_out = addi(alu_in1, alu_in2);
			else if (strcmp(ex.op, "addiu") == 0)
				alu_out = addiu(alu_in1, alu_in2);
			else if (strcmp(ex.op, "andi") == 0)
				alu_out = andi(alu_in1, alu_in2);
			else if (strcmp(ex.op, "ori") == 0)
				alu_out = ori(alu_in1, alu_in2);
			else if (strcmp(ex.op, "slti") == 0)
				alu_out = slti(alu_in1, alu_in2);
			else if (strcmp(ex.op, "sltiu") == 0)
				alu_out = sltiu(alu_in1, alu_in2);
			ex.alu_out = alu_out;
			sw_data = INT_MAX;
		}
		else if ((strcmp(ex.op, "beq") == 0) || (strcmp(ex.op, "bne") == 0)) {
			//an einai branch
			//den ginete tipota
			alu_in1 = INT_MAX;
			alu_in2 = INT_MAX;
			alu_out = INT_MAX;
			sw_data = INT_MAX;
		}
		else if ((strcmp(ex.op, "sll") == 0) || (strcmp(ex.op, "srl") == 0)) {
			//an einai shift
			//FORWARDING UNIT
			if (strcmp(mem.write_reg, ex.rt) == 0) { //an o write reg sto mem einai idios me ton rt
				if (mem.type == 3) { //an einai lw (load-use hazard)
					stall = 1;
					hazard_monitor = 1;
				}
				else { //den einai load-use, ara kanoume forward
					alu_in1 = mem.alu_out;
					forward_monitor = 1;
					forward1 = 1;
				}
			}
			else if (strcmp(wb.write_reg, ex.rt) == 0) { //an o write reg sto wb einai idios me ton rt
				alu_in1 = wb.alu_out;
				forward_monitor = 1;
				forward1 = 1;
			}
			if (forward1==0)
				alu_in1 = id.alu_in1;
			alu_in2 = ex.shift_amount;
			if (strcmp(ex.op, "sll") == 0)
				alu_out = sll(alu_in1, alu_in2);
			else if(strcmp(ex.op, "srl") == 0)
				alu_out = srl(alu_in1, alu_in2);
			ex.alu_out = alu_out;
			sw_data = INT_MAX;
		}
		else if (strcmp(ex.op, "lw") == 0) {
			//an einai lw
			alu_in1 = ex.alu_in1;
			alu_in2 = ex.immediate;
			alu_out = alu_in1 + alu_in2;
			ex.alu_out = alu_out;
			sw_data = INT_MAX;
		}
		else if (strcmp(ex.op, "sw") == 0) {
			//an einai sw
			//FORWARDING UNIT
			if (strcmp(mem.write_reg, ex.rs) == 0) { //an o write reg sto mem einai idios me ton rs
				if (mem.type == 3) { //an einai lw (load-use hazard)
					stall = 1;
					hazard_monitor = 1;
				}
				else { //den einai load-use, ara kanoume forward
					alu_in1 = mem.alu_out;
					forward_monitor = 1;
					forward1 = 1;
				}
			}
			else if (strcmp(wb.write_reg, ex.rs) == 0) { //an o write reg sto wb einai idios me ton rs
				alu_in1 = wb.alu_out;
				forward_monitor = 1;
				forward1 = 1;
			}
			if (strcmp(mem.write_reg, ex.rt) == 0) { //an o write reg sto mem einai idios me ton rt
				if (mem.type == 3) { //an einai lw (load-use hazard)
					stall = 1;
					hazard_monitor = 1;
				}
				else { //den einai load-use, ara kanoume forward
					sw_data = mem.alu_out;
					forward_monitor = 1;
					forward2 = 1;
				}
			}
			else if (strcmp(wb.write_reg, ex.rt) == 0) { //an o write reg sto wb einai idios me ton rt
				sw_data = wb.alu_out;
				forward_monitor = 1;
				forward2 = 1;
			}
			if (forward1==0)
				alu_in1 = ex.alu_in1;
			alu_in2 = ex.immediate;
			alu_out = alu_in1 + alu_in2;
			ex.alu_out = alu_out;
			if (forward2==0)
				sw_data = register_read(ex.rt);
		}
	}
	else {
		ex.full = 0;
		ex.instruction = INT_MAX;
		alu_in1 = INT_MAX;
		alu_in2 = INT_MAX;
		alu_out = INT_MAX;
		sw_data = INT_MAX;
	}
	if (((i_f.full == 1) && (stall != 2)) || ((stall == 2) && (id.full == 1))) { //Proxora i entoli pu einai sto if stage sto id stage
		if (stall != 2) //gia na min proxora i entoli an proigoumenos ixa stall
			id = i_f;
		id.full = 1;
		if ((strcmp(id.op, "add") == 0) || (strcmp(id.op, "addu") == 0) || (strcmp(id.op, "and") == 0) || (strcmp(id.op, "nor") == 0) || (strcmp(id.op, "or") == 0) || (strcmp(id.op, "slt") == 0) || (strcmp(id.op, "sltu") == 0) || (strcmp(id.op, "sub") == 0) || (strcmp(id.op, "subu") == 0)) {
			//an einai r-type
			strcpy(id.write_reg, id.rd);
			id.alu_in1 = register_read(id.rs); //diavazume twra tis times ton kataxoritwn
			id.alu_in2 = register_read(id.rt);
			strcpy(branch_pc, "-");
			strcpy(read_reg1, id.rs);
			strcpy(read_reg2, id.rt);
			read_data1 = register_read(id.rs);
			read_data2 = register_read(id.rt);
			immediate_monitor = id.immediate;
			//TO WRITE_REG (MONITOR 7) DINETE APO TO WB
			//MONITOR 8 = MONITOR 26 (den exei diki tu metavliti)
		}
		else if ((strcmp(id.op, "addi") == 0) || (strcmp(id.op, "addiu") == 0) || (strcmp(id.op, "andi") == 0) || (strcmp(id.op, "ori") == 0) || (strcmp(id.op, "slti") == 0) || (strcmp(id.op, "sltiu") == 0)) {
			//an einai i-type
			strcpy(id.write_reg, id.rt);
			id.alu_in1 = register_read(id.rs);
			strcpy(branch_pc, "-");
			strcpy(read_reg1, id.rs);
			strcpy(read_reg2, "-");
			read_data1 = register_read(id.rs);
			read_data2 = INT_MAX;
			immediate_monitor = id.immediate;
		}
		else if ((strcmp(id.op, "beq") == 0) || (strcmp(id.op, "bne") == 0)) {
			//an einai branch
			//FORWARDING UNIT
			if (stall != 1) { //gia na min allaksei to stall an exw idi stall apo load-use
				if (strcmp(ex.write_reg, id.rs) == 0) { //an o write reg sto ex einai idios me ton rs
					if (stall == 2) //gia na min midenizete to if an eimai se stall cycle
						stall = 5;
					else
						stall = 3;
					hazard_monitor = 1;
				}
				else if (strcmp(ex.write_reg, id.rt) == 0) { //an o write reg sto ex einai idios me ton rt
					if (stall == 2) //gia na min midenizete to if an eimai se stall cycle
						stall = 5;
					else
						stall = 3;
					hazard_monitor = 1;
				}
				else if (strcmp(mem.write_reg, id.rs) == 0) { //an o write reg sto mem einai idios me ton rs
					if (stall == 2) //gia na min midenizete to if an eimai se stall cycle
						stall = 4;
					else
						stall = 1;
					hazard_monitor = 1;
				}
				else if (strcmp(mem.write_reg, id.rt) == 0) { //an o write reg sto mem einai idios me ton rt
					if (stall == 2) //gia na min midenizete to if an eimai se stall cycle
						stall = 4;
					else
						stall = 1;
					hazard_monitor = 1;
				}
			}
			strcpy(id.write_reg, "-");
			strcpy(branch_pc, id.label);
			strcpy(read_reg1, id.rs);
			strcpy(read_reg2, id.rt);
			read_data1 = register_read(id.rs);
			read_data2 = register_read(id.rt);
			immediate_monitor = id.immediate;
			if ((stall == 0)||(stall==2)) { //gia na min ekteleite to branch an tha exw stall
				if (strcmp(id.op, "bne") == 0) {
					if (bne(register_read(id.rs), register_read(id.rt))) {
						next_instruction_is_branch = 1;
						hazard_monitor = 1;
						//add_instruction = 2; //Den tha mpei i epomeni entoli epidi isxyei to branch (Hazard)
						next_branch_instruction = label_decode(id.label);
					}
				}
				else if (strcmp(id.op, "beq") == 0) {
					if (beq(register_read(id.rs), register_read(id.rt))) {
						next_instruction_is_branch = 1;
						hazard_monitor = 1;
						//add_instruction = 2; //Den tha mpei i epomeni entoli epidi isxyei to branch (Hazard)
						next_branch_instruction = label_decode(id.label);
					}
				}
			}
		}
		else if ((strcmp(id.op, "sll") == 0) || (strcmp(id.op, "srl") == 0)) {
			//an einai shift
			strcpy(id.write_reg, id.rd);
			id.alu_in1 = register_read(id.rt);
			strcpy(branch_pc, "-");
			strcpy(read_reg1, "-");
			strcpy(read_reg2, id.rt);
			read_data1 = INT_MAX;
			read_data2 = register_read(id.rt);
			immediate_monitor = id.shift_amount;
		}
		else if (strcmp(id.op, "lw") == 0) {
			//an einai lw
			strcpy(id.write_reg, id.rt);
			id.alu_in1 = register_read(id.rs);
			strcpy(branch_pc, "-");
			strcpy(read_reg1, id.rs);
			strcpy(read_reg2, "-");
			read_data1 = register_read(id.rs);
			read_data2 = INT_MAX;
			id.lw_data = read_data2;
			immediate_monitor = id.immediate;
		}
		else if (strcmp(id.op, "sw") == 0) {
			//an einai sw
			strcpy(id.write_reg, "-");
			id.alu_in1 = register_read(id.rs);
			strcpy(branch_pc, "-");
			strcpy(read_reg1, id.rs);
			strcpy(read_reg2, id.rt);
			read_data1 = register_read(id.rs);
			read_data2 = register_read(id.rt);
			immediate_monitor = id.immediate;
		}
	}
	else {
		id.full = 0;
		strcpy(id.rt, "-");
		strcpy(id.rs, "-");
		strcpy(id.rd, "-");
		id.immediate = INT_MAX;
		id.instruction = INT_MAX;
		strcpy(branch_pc, "-");
		strcpy(read_reg1, "-");
		strcpy(read_reg2, "-");
		read_data1 = INT_MAX;
		read_data2 = INT_MAX;
		immediate_monitor = INT_MAX;
	}
	if ((add_instruction == 1) && (stall != 2) && (stall!=4) && (stall != 5)) {
		i_f.full = 1;
		i_f.instruction = current_instruction;
		i_f.immediate = immediate;
		strcpy(i_f.label,branch_label);
		strcpy(i_f.op, opcode);
		strcpy(i_f.rd, rd);
		strcpy(i_f.rs, rs);
		strcpy(i_f.rt, rt);
		i_f.shift_amount = shift_amount;
		i_f.alu_out = INT_MAX;
		i_f.lw_data = INT_MAX;
		i_f.type = type;
	}
	else if (((stall != 2) && (stall != 4) && (stall != 5)) || (add_instruction==2)) {//gia na min proxora i entoli an proigoumenos ixa stall
		i_f.full = 0;
		i_f.instruction = INT_MAX;
	}
	if (next_instruction_is_branch==1) //gia na allazei to monitor 1 otan esxyei to branch
		sprintf(pc_monitor, "%x", next_branch_instruction);
	check_cycle_print();
	if ((stall == 3) || (stall==5)) { //proxorun mono to ex kai mem (branch hazard)
		advance_mem_wb();
		advance_ex_mem();
		ex.full = 0;
		strcpy(ex.write_reg, "-");
		stall = 2;
		advance_pipeline(0);
	}
	else if ((stall == 1) || (stall == 4)) { //LOAD-USE Hazard
		advance_mem_wb(); //proxora i load apo to mem sto wb
		mem.full = 0; //mpenei bubble sto mem
		strcpy(mem.write_reg, "-");
		stall = 2; //otan to stall einai = 2 den proxoroun oi entoles sto pipeline.
		advance_pipeline(0);
	}
	else if (stall == 2) { //eixa stall proigumenos
		stall = 0;
	}
}

int main() {
	filepointer = fopen("pipeline_test.s", "r");
	//Initializaton of values
	for (i = 0; i < MAX_MEMORY_SIZE; i++) {
		memory[i].address = 0;
	}
	gp = 0x10008000;
	sp = 0x7ffffffc;
	zero = 0;
	pc = 0;
	output = fopen("output_file.txt", "w");
	fprintf(output, "Name: Michalis Piponidis\nID: 912526\n\n");
	printf("\nInsert Cycle 1: ");
	scanf("%d", &cycle1);
	printf("\nInsert Cycle 2: ");
	scanf("%d", &cycle2);
	parser();
	instruction_counter--;
	//Execution of Instructions
	i_f.full = 0;
	id.full = 0;
	ex.full = 0;
	mem.full = 0;
	wb.full = 0;
	current_cycle = 0;
	start = clock();
	while ((end == 0) && (current_instruction<=instruction_counter+1)) { //end becomes 1 when sll $zero, $zero, 0 is found
		i = 0;
		while ((instructions[current_instruction][i] != ' ') && (instructions[current_instruction][i] != '\t')) { //isolate the opcode (instruction)
			opcode[i] = instructions[current_instruction][i];
			i++;
		}
		opcode[i] = '\0';
		if (strcmp(opcode, "add") == 0) { //add
			decode_r_type();
			//set values gia na mpoun sto stage
			pc = current_instruction * 4;
			sprintf(pc_monitor, "%x", pc+4);
			strcpy(branch_label, "-");
			shift_amount = INT_MAX;
			immediate = INT_MAX;
			type = 1;
			advance_pipeline(1);
			//register_write(rd, add(register_read(rs), register_read(rt)));
			//printf("\nInstruction %d is %s with rd= %s rs= %s rt= %s\n", current_instruction, opcode, rd, rs, rt);
		}
		else if (strcmp(opcode, "addi") == 0) { //addi
			decode_i_type();
			//set values gia na mpoun sto stage
			strcpy(rd, "-");
			pc = current_instruction * 4;
			sprintf(pc_monitor, "%x", pc + 4);
			strcpy(branch_label, "-");
			shift_amount = INT_MAX;
			type = 2;
			advance_pipeline(1);
			//register_write(rs, addi(register_read(rt), immediate));
			//printf("\nInstruction %d is %s with rs= %s rt= %s immediate= %d\n", current_instruction, opcode, rs, rt, immediate);
		}
		else if (strcmp(opcode, "addiu") == 0) { //addiu
			decode_i_type();
			//set values gia na mpoun sto stage
			strcpy(rd, "-");
			pc = current_instruction * 4;
			sprintf(pc_monitor, "%x", pc + 4);
			strcpy(branch_label, "-");
			shift_amount = INT_MAX;
			type = 2;
			advance_pipeline(1);
			//register_write(rs, addiu(register_read(rt), immediate));
			//printf("\nInstruction %d is %s with rs= %s rt= %s immediate= %d\n", current_instruction, opcode, rs, rt, immediate);
			
		}
		else if (strcmp(opcode, "addu") == 0) { //addu
			decode_r_type();
			//set values gia na mpoun sto stage
			pc = current_instruction * 4;
			sprintf(pc_monitor, "%x", pc + 4);
			strcpy(branch_label, "-");
			shift_amount = INT_MAX;
			immediate = INT_MAX;
			type = 1;
			advance_pipeline(1);
			//register_write(rd, addu(register_read(rs), register_read(rt)));
			//printf("\nInstruction %d is %s with rd= %s rs= %s rt= %s\n", current_instruction, opcode, rd, rs, rt);
		}
		else if (strcmp(opcode, "and") == 0) { //and
			decode_r_type();
			//set values gia na mpoun sto stage
			pc = current_instruction * 4;
			sprintf(pc_monitor, "%x", pc + 4);
			strcpy(branch_label, "-");
			shift_amount = INT_MAX;
			immediate = INT_MAX;
			type = 1;
			advance_pipeline(1);
			//register_write(rd, and (register_read(rs), register_read(rt)));
			//printf("\nInstruction %d is %s with rd= %s rs= %s rt= %s\n", current_instruction, opcode, rd, rs, rt);
		}
		else if (strcmp(opcode, "andi") == 0) { //andi
			decode_i_type();
			//set values gia na mpoun sto stage
			strcpy(rd, "-");
			pc = current_instruction * 4;
			sprintf(pc_monitor, "%x", pc + 4);
			strcpy(branch_label, "-");
			shift_amount = INT_MAX;
			type = 2;
			advance_pipeline(1);
			//register_write(rs, andi(register_read(rt), immediate));
			//printf("\nInstruction %d is %s with rs= %s rt= %s immediate= %d\n", current_instruction, opcode, rs, rt, immediate);
		}
		else if (strcmp(opcode, "beq") == 0) { //beq
			decode_branch();
			//set values gia na mpoun sto stage
			pc = current_instruction * 4;
			strcpy(rd, "-");
			immediate = INT_MAX;
			shift_amount = INT_MAX;
			strcpy(branch_label, word);
			branch_instruction = label_decode(word);
			sprintf(pc_monitor, "%x", pc + 4);
			type = 0;
			advance_pipeline(1);
			//printf("\nInstruction %d is %s with rs= %s rt= %s label= %s. Branch: %d\n", current_instruction, opcode, rs, rt, word, (register_read(rs) == register_read(rt)));
		}
		else if (strcmp(opcode, "bne") == 0) { //bne
			decode_branch();
			//set values gia na mpoun sto stage
			pc = current_instruction * 4;
			strcpy(rd, "-");
			immediate = INT_MAX;
			shift_amount = INT_MAX;
			strcpy(branch_label, word);
			branch_instruction = label_decode(word);
			sprintf(pc_monitor, "%x", pc + 4);
			type = 0;
			advance_pipeline(1);
			//printf("\nInstruction %d is %s with rs= %s rt= %s label= %s. Branch: %d\n", current_instruction, opcode, rs, rt, word, (register_read(rs) != register_read(rt)));
		}
		else if (strcmp(opcode, "lw") == 0) { //lw
			decode_memory();
			//set values gia na mpoun sto stage
			pc = current_instruction * 4;
			sprintf(pc_monitor, "%x", pc + 4);
			strcpy(rd, "-");
			strcpy(branch_label, "-");
			shift_amount = INT_MAX;
			type = 3;
			advance_pipeline(1);
			//register_write(rt, lw(register_read(rs), immediate));
			//printf("\nInstruction %d is %s with rt= %s base= %s offset= %d.\n", current_instruction, opcode, rt, rs, immediate);
		}
		else if (strcmp(opcode, "nor") == 0) { //nor
			decode_r_type();
			//set values gia na mpoun sto stage
			pc = current_instruction * 4;
			sprintf(pc_monitor, "%x", pc + 4);
			strcpy(branch_label, "-");
			shift_amount = INT_MAX;
			immediate = INT_MAX;
			type = 1;
			advance_pipeline(1);
			//register_write(rd, nor(register_read(rs), register_read(rt)));
			//printf("\nInstruction %d is %s with rd= %s rs= %s rt= %s\n", current_instruction, opcode, rd, rs, rt);
		}
		else if (strcmp(opcode, "or") == 0) { //or
			decode_r_type();
			//set values gia na mpoun sto stage
			pc = current_instruction * 4;
			sprintf(pc_monitor, "%x", pc + 4);
			strcpy(branch_label, "-");
			shift_amount = INT_MAX;
			immediate = INT_MAX;
			type = 1;
			advance_pipeline(1);
			//register_write(rd, or (register_read(rs), register_read(rt)));
			//printf("\nInstruction %d is %s with rd= %s rs= %s rt= %s\n", current_instruction, opcode, rd, rs, rt);
		}
		else if (strcmp(opcode, "ori") == 0) { //ori
			decode_i_type();
			//set values gia na mpoun sto stage
			strcpy(rd, "-");
			pc = current_instruction * 4;
			sprintf(pc_monitor, "%x", pc + 4);
			strcpy(branch_label, "-");
			shift_amount = INT_MAX;
			type = 2;
			advance_pipeline(1);
			//register_write(rs, ori(register_read(rt), immediate));
			//printf("\nInstruction %d is %s with rs= %s rt= %s immediate= %d\n", current_instruction, opcode, rs, rt, immediate);
		}
		else if (strcmp(opcode, "slt") == 0) { //slt
			decode_r_type();
			//set values gia na mpoun sto stage
			pc = current_instruction * 4;
			sprintf(pc_monitor, "%x", pc + 4);
			strcpy(branch_label, "-");
			shift_amount = INT_MAX;
			immediate = INT_MAX;
			type = 1;
			advance_pipeline(1);
			//register_write(rd, slt(register_read(rs), register_read(rt)));
			//printf("\nInstruction %d is %s with rd= %s rs= %s rt= %s\n", current_instruction, opcode, rd, rs, rt);
		}
		else if (strcmp(opcode, "slti") == 0) { //slti
			decode_i_type();
			//set values gia na mpoun sto stage
			strcpy(rd, "-");
			pc = current_instruction * 4;
			sprintf(pc_monitor, "%x", pc + 4);
			strcpy(branch_label, "-");
			shift_amount = INT_MAX;
			type = 2;
			advance_pipeline(1);
			//register_write(rs, slti(register_read(rt), immediate));
			//printf("\nInstruction %d is %s with rs= %s rt= %s immediate= %d\n", current_instruction, opcode, rs, rt, immediate);
		}
		else if (strcmp(opcode, "sltiu") == 0) { //sltiu
			decode_i_type();
			//set values gia na mpoun sto stage
			strcpy(rd, "-");
			pc = current_instruction * 4;
			sprintf(pc_monitor, "%x", pc + 4);
			strcpy(branch_label, "-");
			shift_amount = INT_MAX;
			type = 2;
			advance_pipeline(1);
			//register_write(rs, sltiu(register_read(rt), immediate));
			//printf("\nInstruction %d is %s with rs= %s rt= %s immediate= %d\n", current_instruction, opcode, rs, rt, immediate);
		}
		else if (strcmp(opcode, "sltu") == 0) { //sltu
			decode_r_type();
			//set values gia na mpoun sto stage
			pc = current_instruction * 4;
			sprintf(pc_monitor, "%x", pc + 4);
			strcpy(branch_label, "-");
			shift_amount = INT_MAX;
			immediate = INT_MAX;
			type = 1;
			advance_pipeline(1);
			//register_write(rd, sltu(register_read(rs), register_read(rt)));
			//printf("\nInstruction %d is %s with rd= %s rs= %s rt= %s\n", current_instruction, opcode, rd, rs, rt);
		}
		else if (strcmp(opcode, "sll") == 0) { //sll
			decode_shift();
			//set values gia na mpoun sto stage
			pc = current_instruction * 4;
			sprintf(pc_monitor, "%x", pc + 4);
			strcpy(rs, "-");
			shift_amount = immediate;
			immediate = INT_MAX;
			type = 1;
			strcpy(branch_label, "-");
			//Sinthiki teliomatos programmatos
			if ((strcmp("$zero", rd) == 0) && (strcmp("$zero", rt) == 0) && (shift_amount == 0)) {
				if ((strcmp(i_f.op,"beq")!=0) && (strcmp(i_f.op, "bne") != 0)) //gia na min telionei otan i proigumeni einai branch
					end = 1;
 				advance_pipeline(1);
				//printf("\nLast Instruction %d is %s with rd= %s rt= %s shift amount= %d\n", current_instruction, opcode, rd, rt, immediate);
				//break;
			}
			else {
				advance_pipeline(1);
			}
			//register_write(rd, sll(register_read(rt), shift_amount));
			//printf("\nInstruction %d is %s with rd= %s rt= %s shift amount= %d\n", current_instruction, opcode, rd, rt, immediate);
		}
		else if (strcmp(opcode, "srl") == 0) { //srl
			decode_shift();
			//set values gia na mpoun sto stage
			pc = current_instruction * 4;
			sprintf(pc_monitor, "%x", pc + 4);
			strcpy(rs, "-");
			shift_amount = immediate;
			immediate = INT_MAX;
			strcpy(branch_label, "-");
			type = 1;
			advance_pipeline(1);
			//register_write(rd, srl(register_read(rt), shift_amount));
			//printf("\nInstruction %d is %s with rd= %s rt= %s shift amount= %d\n", current_instruction, opcode, rd, rt, immediate);
		}
		else if (strcmp(opcode, "sw") == 0) { //sw
			decode_memory();
			//set values gia na mpoun sto stage
			pc = current_instruction * 4;
			sprintf(pc_monitor, "%x", pc + 4);
			strcpy(rd, "-");
			strcpy(branch_label, "-");
			shift_amount = INT_MAX;
			type = 0;
			advance_pipeline(1);
			//sw(register_read(rt), register_read(rs), immediate);
			//printf("\nInstruction %d is %s with rt= %s base= %s offset= %d.\n", current_instruction, opcode, rt, rd, immediate);
		}
		else if (strcmp(opcode, "sub") == 0) { //sub
			decode_r_type();
			//set values gia na mpoun sto stage
			pc = current_instruction * 4;
			sprintf(pc_monitor, "%x", pc + 4);
			strcpy(branch_label, "-");
			shift_amount = INT_MAX;
			immediate = INT_MAX;
			type = 1;
			advance_pipeline(1);
			//register_write(rd, sub(register_read(rs), register_read(rt)));
			//printf("\nInstruction %d is %s with rd= %s rs= %s rt= %s\n", current_instruction, opcode, rd, rs, rt);
		}
		else if (strcmp(opcode, "subu") == 0) { //subu
			decode_r_type();
			//set values gia na mpoun sto stage
			pc = current_instruction * 4;
			sprintf(pc_monitor, "%x", pc + 4);
			strcpy(branch_label, "-");
			shift_amount = INT_MAX;
			immediate = INT_MAX;
			type = 1;
			advance_pipeline(1);
			//register_write(rd, subu(register_read(rs), register_read(rt)));
			//printf("\nInstruction %d is %s with rd= %s rs= %s rt= %s\n", current_instruction, opcode, rd, rs, rt);
		}
		if (next_instruction_is_branch==0)
			current_instruction++;
		else if (next_instruction_is_branch == 1) { //ginetai 1 otan isxiei to branch (sto 2o stage)
			i_f.full = 0;
			next_instruction_is_branch = 0;
			current_instruction = next_branch_instruction;
		}
		//print_registers_hex();
		//printf("\n-----------------------------------------------------------------------------------\n");
		//printf("\nCURR: %d\tINS+COUNTER: %d\n", current_instruction, instruction_counter);
	}
	while ((i_f.full == 1) || (id.full == 1) || (ex.full == 1) || (mem.full == 1)) {
		advance_pipeline(0);
	}
	endt = clock();
	//print_registers_hex();
	printf("\nFINAL CYCLES %d\n\n", current_cycle);
	export_final_info();
	printf("\nThe program has ended.\n");
	printf("\nTime (seconds): %f\n", ((float)(endt - start) / CLOCKS_PER_SEC));
}