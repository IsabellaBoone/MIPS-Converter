/* Include statements */ 
#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <ctype.h> 

/* Define statements */
#define MAX_READS 11 // Max number of reads 
#define MAX 50 // Max length of strings
#define SENTINEL "\n" // Sentinel for gathering input. 
#define REGS 32 // Number of registers
#define INSTRUCTIONS 31 // Number of instructions 

typedef struct {
  char *name; 
  char type; // 1 = r, 2 = i, 3 = j
  unsigned int opcode;  //int opcode;
  unsigned int func; 
}instr_t;

typedef struct {
  char name[10];
}reg_t; 

enum norm_state { WS1, INSTR, WS2, REG };   // Needed for normalize
char *normalize(const char const *string);  // Condenses input by removing spaces from input
int getInstructionIndex(char *b);           // Get the index of where the matching instruction is 
char getInstrType(char *b);                  // Get the instruction type (r, i, j)
int validRegister(char *b);                 // Test is the register is valid
int getRegisterIndex(char *b);              // Get the index of where the matching register is located in our array

// Global variable of valid instruction set and registers.
instr_t instructions[] = {
  /* 8 mem types - lbu, lhu, ll, lw, sb, sc, sh, sw,   */
    {"lbu", 'i', 0x24, 0x24}, {"lhu", 'i', 0x25, 0x25}, {"ll", 'i', 0x30, 0x30},
    {"lw", 'i', 0x23, 0x23},  { "sb", 'i', 0x28, 0x28},  {"sc", 'i', 0x38, 0x38},
    {"sh", 'i', 0x29, 0x29},  {"sw", 'i', 0x2b, 0x2b},  {"sll", 'r', 0x0, 0x00},
    // special instructions 
    {"lui", 'i', 0xf, 0xf},   {"ori", 'i', 0xd, 0xd},   
    // core instructions 
    {"add", 'r', 0x0, 0x20},  {"addi", 'i', 0x8, 0x8},  {"addiu", 'i', 0x9, 0x09}, 
    {"addu", 'r', 0x0, 0x21}, {"and", 'r', 0x0, 0x24},  {"andi", 'i', 0xc, 0xc}, 
    {"beq", 'i', 0x4, 0x4},   {"bne", 'i', 0x5, 0x5},   {"j", 'j', 0x2, 0x2}, 
    {"jal", 'j', 0x3, 0x3},   {"jr", 'r', 0x0, 0x08},   {"nor", 'r', 0x0, 0x27},  
    {"or", 'r', 0x0, 0x25},   {"slt", 'r', 0x0, 0x2a},  {"slti", 'i', 0xa, 0xa},  
    {"sltiu", 'i', 0xb, 0xb}, {"sltu", 'r', 0x0, 0x2b}, {"srl", 'r', 0x0, 0x02},  
    {"sub", 'r', 0x0, 0x22},  {"subu", 'r',0x0, 0x23},  
    // arithmetic core instruction set 
    {"div", 'r', 0x0, 0x1a},   {"divu", 'r', 0x0, 0x1b}, {"lwc1", 'i', 0x31, 0x31},
    {"ldc1", 'i', 0x35, 0x35}, {"mfhi", 'r', 0x0, 0x10}, {"mflo", 'r', 0x0, 0x12}, 
    {"mfc0", 'r', 0x10, 0x0}, {"mult", 'r', 0x0, 0x18}, {"multu", 'r', 0x0, 0x19}, 
    {"sra", 'r', 0x0, 0x3},    {"swc1", 'i', 0x39, 0x39}, {"sdc1", 'i', 0x3d, 0x3d}};
    // pseudo instruction set 

  reg_t registers[] = {
  	  {"$zero"}, {"$at"}, {"$v0"}, {"$v1"}, {"$a0"}, {"$a1"}, 
      {"$a2"}, {"$a3"}, {"$t0"}, {"$t1"}, {"$t2"}, {"$t3"}, {"$t4"}, 
      {"$t5"}, {"$t6"}, {"$t7"}, {"$s0"}, {"$s1"}, {"$s2"}, {"$s3"},
      {"$s4"}, {"$s5"}, {"$s6"}, {"$s7"}, {"$t8"}, {"$t9"}, {"$k0"}, 
      {"$k1"}, {"$gp"}, {"$sp"}, {"$fp"}, {"$ra"}}; 

int main(void) {
  int flag0 = 0, instrNum = 0; 
  /* flag0 = 0 indicates there is currently no error
     flag0 = 1 indicates there is an instruction error
     flag0 = 2 indicates there is a register error 
     flag0 = 3 indicates there is a format error 
     
     instrNum = number of instructions entered by user. */  

  char buffer[MAX_READS][MAX]; 
  /* Read in input, normalize it and store it in buffer. 
   * ex - buffer[0] = add,$t1,$t2,$t3 or addi $t1, $t2, 4 */
  while(fgets(buffer[instrNum], sizeof(buffer[instrNum]), stdin) && (instrNum < MAX_READS)) {
    // If the input only reads a newline, break, and do NOT increment instrNum 
    if(strcmp(buffer[instrNum], SENTINEL) == 0) {
      break; 
    }

    // Convert all chars in array to lowercase 
    for(int j = 0; j < MAX; j++) {
      char c = buffer[instrNum][j];
      // If we find the newline, break out 
      if(c == '\n') {
        break; 
      }
      c = tolower(c); // convert char to lowercase 
      buffer[instrNum][j] = c; // put into buffer 
    }

    char *token = strtok(buffer[instrNum], "\n"); // take everything before the newline
    strcpy(buffer[instrNum], token); // copy it to buffer[instrNum], looks like 
    strcpy(buffer[instrNum], normalize(buffer[instrNum])); // normalize it "add     $t1,     $t2,     $t3" 
    instrNum++; // increment instruction number, now looks like "add,$t1,$t2,$t3"
  }

  /* Start reading each instruction and begin converting them. */ 
  for(int i = 0; i < instrNum; i++) {  
    // Variables 
    char line[6][MAX], *token = strtok(buffer[i], ","); 
    char *name, type; 
    unsigned int opcode, func; 
    int index, j = 0; 

    while(token != NULL) {
      strcpy(line[j], token); 
      token = strtok(NULL, ","); 
      j++; 
    }

    name = line[0]; 
    type = instructions[getInstructionIndex(line[0])].type; 
    opcode = instructions[getInstructionIndex(line[0])].opcode; 
    func = instructions[getInstructionIndex(line[0])].func; 
    index = getInstructionIndex(name); 

    /* This flag will only flip is there is an error with an instruction,
       register, or the format. */  
    if(flag0 == 0) {
      // if index = -1, function could not match user input instruction to database
      if(index == -1) {
        flag0 = 1;         
      }

      /* Take anything in line[j] that starts with a '$' and check if it's a valid register.
         If it is invalid, the function will return a -1 and we can trip flag0. */ 
      for(j = 0; j < 6; j++) {
        if(line[j][0] == '$') {
          if(validRegister(line[j]) == -1) {
            flag0 = 2;   
          }
        }        
      }

      /* The remainder of the code is to find what the hex value is. */
      switch(type) {
        case('r'): {
          
          if((line[1][0] != '$') || (line[2][0] != '$') || (line[3][0] != '$')) {
            flag0 = 3; 
            break;
          }
          // r type -  needs to add opcode, rs, rt, rd, shamt, func. 
	  if(flag0 == 0) 
          printf("%08x\n", opcode << 26 | getRegisterIndex(line[2]) << 21  | 
           getRegisterIndex(line[3]) << 16  | getRegisterIndex(line[1]) << 11  | func); 
          break;
        }
     
        case 'i': {
          int two; 
          // If instruction index is below 9, it is a memtype instruction 
          // and memtypes need special handling for the third parameter
          if(index < 9) {
            // If the instruction is the lw instruction, it needs special handling 
            if(index == 3) {
 
             int count = 0;
             for(j = 0; j < 6; j++) {
              if(line[j][0] == '$') {
               if(validRegister(line[j])) {
               count++;
                }
               }        
              }
	     if(count >2)
	       flag0 = 3;

              // Get the parenthesis off of the string 
              sscanf(line[2], " %d( %s )", &two, line[2]);
              char* temp = strtok(line[2], ")");
	      
              if(flag0 == 0)
              printf("%08x\n", opcode << 26 | getRegisterIndex(line[2]) << 21 |
                getRegisterIndex(line[1]) << 16 | abs(two));
              break; 
            }
            // Get the parenthesis off of the string
            sscanf(line[2], " %d( %s )", &two, line[2]);
            char* temp = strtok(line[2], ")");

            // Print that instruction 
            printf("%08x\n", opcode << 26 | getRegisterIndex(line[2]) << 21 | 
              getRegisterIndex(line[1]) << 16 | two);
            break;
          } else if(index == 9) {
            // Else, if we have a lui instruction, it requires special handling 
            printf("%08x\n", opcode << 26 | 0 << 21 | getRegisterIndex(line[1]) << 16 | 
            abs(atoi(line[2]))); 
            break;
          } else if(index == 10) {
            // Else, if we we have an ori instruction, it requires special handling 
            // Get the parenthesis off the string 
            sscanf(line[2], " %d( %s )", &two, line[2]);
            char* temp = strtok(line[2], ")");
            // Print 
            printf("%08x\n", opcode << 26 | getRegisterIndex(line[2]) << 21 | getRegisterIndex(line[1]) << 16 | abs(atoi(line[3])));
            break; 
          } else { 
            // instruction is >= 8, not a memtype, lets continue without special handling  
            // i type - opcode, rs, rt, immediate
	    if(index == 12){
             int count = 0;
             for(j = 0; j < 6; j++) {
              if(line[j][0] == '$') {
               if(validRegister(line[j])) {
               count++;
                }
               }        
              }
	     if(count >2)
	       flag0 = 3;
            }
            if(flag0 == 0)
            printf("%08x\n", 
             opcode << 26 | getRegisterIndex(line[2])  << 21 |
             getRegisterIndex(line[1])  << 16 | abs(atoi(line[3]))); 
          }

          break; // apparently this is an important line. do not remove. 
        }
        case 'j': {
          int a = abs((int)(strtol(line[1], NULL, 0)));
          // j type - opcode , address
	  if(flag0 == 0) 
          printf("%08x\n", opcode << 26 | a); 
          break;
        }
      }
    }
     
    if(flag0 == 1) {
      printf("ERROR - INVALID INSTRUCTION\n");
      // printf("buffer[%d]: %s\n", i, buffer[i]);
      flag0 = 0; 
    }
    if(flag0 == 2) {
      printf("ERROR - INVALID REGISTER\n"); 
      // printf("buffer[%d]: %s\n", i, buffer[i]); 
      flag0 = 0;
    }
    if(flag0 == 3) {
      printf("ERROR - INVALID FORMAT\n"); 
      // printf("buffer[%d]: %s\n", i, buffer[i]);
      flag0 = 0; 
    }
  } 
}

/** This function will take in the instruction (b) and 
  * compare it to each instruction name until if finds
  * a match. If it can, it returns the index it found
  * it at, and otherwise it will just return -1.   
  */
int getInstructionIndex(char *b) { 
  for(int i = 0; i < INSTRUCTIONS; i++) {
    char *tmp = instructions[i].name; 
    if(strcmp(tmp, b) == 0) { 
      return i; 
    } 
  }
  return -1; 
}

/** This function will return the instruction found at the 
  * instruction index given by getInstructionIndex. 
  */
char getInstrType(char *b) {
  /* Basically call get instruction index and return instruction[getinstruction] */
  return(instructions[getInstructionIndex(b)].type); 
}

/** This function compares each register to our register array
  * and returns 1 if the register is valid. If it can't find the
  * register, it returns a -1.   
  */
int validRegister(char *b) {
  // If b is null, automatically return -1. 
  if(b == NULL) {
    return -1; 
  }

  /* Compare b against all the registers until
     a match is found. */ 
  for(int i = 0; i < REGS; i++) {
    char *tmp = registers[i].name;
     if(strcmp(tmp, b) == 0) {
      /* Return 1 to indicate it is valid */ 
      return 1; 
     }
  }  
  /* If no match was found, return -1 to indicate so. */ 
  return -1; 
}

/** This function will search through the register array until it
  * finds its match and then return what position it found the match
  * at.  
  * - note, this function is only called if the registers pass 
  *   the validRegister portion.  
  */
int getRegisterIndex(char *b) {
  for(int i = 0; i < REGS; i++) {
    char *tmp = registers[i].name;
    if(strcmp(tmp, b) == 0) {
      return i; 
    }
  }
}

/* Normalize Function */ 
char *normalize(const char *string)
{
	enum norm_state state = WS1;
	
	// the destination buffer to hold a copy of the input string
	char *copy = malloc(strlen(string+1));
	
	// make the copy all nulls....
	memset(copy, 0, strlen(string+1));
	
	// use two pointers to walk through the input and output strings
	char *dest = copy;
	const char *curr = string;
	
	
	// sentinnel loop looking for the end of the string
	while ((*curr != 0) && (*curr != '\n')) {
		switch(state) {
		// eat up the spaces (if any) at the beginning of the line
		case WS1:
			if (isspace(*curr)) state = WS1;
			else {
				// dref the pointers for the assignment, use pointer
				// arithmetic to advnce the output pointer
				*dest++ = *curr;
				state = INSTR;
			}
			break;
			
		// copy the characters to the output until space
		case INSTR:
			if (isspace(*curr)) {
				*dest++ = ',';
				state = WS2;
			}
			else {
				*dest++ = *curr;
			}
			break;
			
		// skip spaces between fields
		case WS2: 
			if (isspace(*curr)) state = WS2;
			else {
				*dest++ = *curr;
				state = REG;
			}
			break;
		
		// copy the characters to the output until a space
		case REG: 
			if (isspace(*curr)) state = WS2;
			else {
				*dest++ = *curr;
				state = REG;
			}
			break;
		default:
			return NULL;
		}
		
		// go to next input character
		curr++;
	}
	
	// the last character might be a ',' but it should be a null
	if (*dest == ',') *dest = 0x00;
	
	// remember dest is pointing into copy's memory....
	return copy;
}
