/*
Homework:
lex - HW2 PL/0 lexical analyzer

Author(s): Gabriel Ross & Christoph Watson
Language: C only

To Compile:
    gcc -Wall -Wextra -std=c11 -O2 lex.c -o lex

To Execute (on Eustis):
    ./lex <input_file>

   where:
    <input_file> is the path to a text file holding a PL/0 source program

    Notes:
    - Implements the lexical analyzer described in the homework instructions.
    - Prints four sections to standard output: Source Program, Lexeme Table, Name Table and Token List.
    - Writes two files into the working directory: tokens.txt and nametable.txt.
    - Stops at the first lexical error, prints everything scanned before it, reports the error with its line and column, and exits with a non-zero status.
    - Exits with status 0 when the whole program scans without an error.
    - Tested on Eustis.

Class: COP 3402 - Systems Software

Instructor: Jie Lin, Ph.D.

Due Date: Oct 2, 2026
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct
{
  char lexeme[100];
  int token;
  int index;
} Token;

typedef struct
{
  char name[100];
  int line;
  int column;
} Name;

typedef struct
{
  const char* spelling;
  const char* name;
  int code;
} Table;

static const Table symbols[] =
{
  {"+", "plussym", 3},
  {"-", "minussym", 4},
  {"*", "multsym", 5},
  {"/", "slashsym", 6},
  {"==", "eqsym", 7},
  {"!=", "neqsym", 8},
  {"<", "lessym", 9},
  {"<=", "leqsym", 10},
  {">", "gtrsym", 11},
  {">=", "geqsym", 12},
  {"(", "lparentsym", 13},
  {")", "rparentsym", 14},
  {",", "commasym", 15},
  {";", "semicolonsym", 16},
  {".", "periodsym", 17},
  {"=", "assignsym", 18},
  {":=", "initsym", 19}
};

static const Table keywords[] =
{
  {"begin", "beginsym", 20},
  {"end", "endsym", 21},
  {"if", "ifsym", 22},
  {"fi", "fisym", 23},
  {"then", "thensym", 24},
  {"while", "whilesym", 25},
  {"elihw", "elihwsym", 26},
  {"do", "dosym", 27},
  {"od", "odsym", 28},
  {"odd", "oddsym", 29},
  {"call", "callsym", 30},
  {"const", "constsym", 31},
  {"var", "varsym", 32},
  {"procedure", "procsym", 33},
  {"write", "writesym", 34},
  {"read", "readsym", 35},
  {"else", "elsesym", 36}
};
const int tableSize = 17;

Token tokens[10000];
Name names[10000];

int tokenCount = 0;
int nameCount = 0;

static void addToken(char lexeme[], int token, int index)
{
  strcpy(tokens[tokenCount].lexeme, lexeme);
  tokens[tokenCount].token = token;
  tokens[tokenCount].index = index;

  tokenCount++;
}

/*
int checkKeyword(char word[]) {
    if (strcmp(word, "begin") == 0) return 20;
    if (strcmp(word, "end") == 0) return 21;
    if (strcmp(word, "if") == 0) return 22;
    if (strcmp(word, "fi") == 0) return 23;
    if (strcmp(word, "then") == 0) return 24;
    if (strcmp(word, "while") == 0) return 25;
    if (strcmp(word, "elihw") == 0) return 26;
    if (strcmp(word, "do") == 0) return 27;
    if (strcmp(word, "od") == 0) return 28;
    if (strcmp(word, "odd") == 0) return 29;
    if (strcmp(word, "call") == 0) return 30;
    if (strcmp(word, "const") == 0) return 31;
    if (strcmp(word, "var") == 0) return 32;
    if (strcmp(word, "procedure") == 0) return 33;
    if (strcmp(word, "write") == 0) return 34;
    if (strcmp(word, "read") == 0) return 35;
    if (strcmp(word, "else") == 0) return 36;

    return 0;

  for(int i = 0; i < 17; i++)
  {
    if(strcmp(word, keywords[i].spelling) == 0)
    {
      return keywords[i].code;
    }
    return -1;
  }
}
*/

int getCode(const Table *table, char* w)
{
  for(int i = 0; i < tableSize; i++)
  {
    if(strcmp( w, table[i].spelling) == 0)
    {
      return table[i].code;
    }
  }
  return -1; //no match
}

static void printError(int errorNum, int lineNum, int col) {
    switch(errorNum){
        case 1:
            printf("Error: %d at line %d, column %d: invalid character ’c’, with the character in place of c. A printable ASCII character that is not part of this language: ##, $, @, ?, _, ’, ‘, a bracket, a brace, a backslash.\n", errorNum, lineNum, col);
            break;

        case 2: 
            printf("Error: %d at line %d, column %d: identifier too long ’lexeme’, with the whole run in place of lexeme. A letter-led run longer than twelve characters.\n", errorNum, lineNum, col);
            break;

        case 3:
            printf("Error: %d at line %d, column %d: number too long 'lexeme'. A digit run longer than six digits.\n", errorNum, lineNum, col);
            break;

        case 4:
            printf("Error: %d at line %d, column %d: ':' must be followed by '='. A colon that is not part of :=.\n", errorNum, lineNum, col);
            break;

        case 5:
            printf("Error: %d at line %d, column %d: ’!’ must be followed by ’=’. An exclamation mark that is not part of !=.\n", errorNum, lineNum, col);
            break;

        case 6:
            printf("Error: %d at line %d, column %d: number followed by a letter ’lexeme’, with the whole alphanumeric", errorNum, lineNum, col);
            break;

        case 7:
            printf("Error: %d at line %d, column %d: comment is not closed before end of file. Reported at the position of the /* that opened it.\n", errorNum, lineNum, col);
            break;

        case 8: 
            printf("Error: %d at line %d, column %d: ’*/’ without a matching ’/*’.\n", errorNum, lineNum, col);
            break;

        case 9: 
            printf("Error: %d at line %d, column %d: ’/*’ inside a comment.\n", errorNum, lineNum, col);
            break;

        case 10:
            printf("Error: %d at line %d, column %d: byte 0xHH is not part of this language, with two upper-case hexadecimal digits in place of HH. Any byte outside the printable ASCII range 0x20 to 0x7E that is not one of the four whitespace characters.", errorNum, lineNum, col);
            break;

        case 11:
            printf("Error: %d at line %d, column %d: no tokens in the source program. Reported at line 1, column 1.", errorNum, lineNum, col);
            break;
    }
}

void printResults() {
    printf("\nLexeme Table:\n\nlexeme \t\ttoken\n");

    for(int i = 0; i < tokenCount; i++) {
        printf("%s\t%d\n", tokens[i].lexeme, tokens[i].token);

        if(tokens[i].token == 1) {
            printf(" %d", tokens[i].index);
        }

        else if(tokens[i].token == 2) {
            printf(" %s", tokens[i].lexeme);
        }
    }

    printf("\nName Table:\n\nindex  name\t\tline\tcolumn\n");

    for(int i = 0; i < nameCount; i++) {
        printf("%d\t%s\t\t%d\t%d", i, names[i].name, names[i].line, names[i].column);
    }

    printf("\n\nToken List:\n\n");
    for(int i = 0; i < tokenCount; i++) {
        printf("%d", tokens[i].token);
    }
    printf("\n");
}

int isLetter(char c) {
  return ((c >= 'A' && c <= 'Z') || (c >='a' && c <= 'z'));
}

int isNum(char c) {
  return c >= '0' && c <= '9';
}

int findName(char word[]) {
  for(int i = 0; i < nameCount; i++) {
    if(strcmp(names[i].name, word) == 0) {
      return i;
    }
  }

  return -1;
}

//void errorHelper(const char* lexeme, int errorNum, int row, int lineNum)
//{
    
//}

void writeTokens() {
    FILE* fp = fopen("tokens.txt", "w");

    if(fp == NULL) {
        return;
    }

    for(int i = 0; i < tokenCount; i++) {
        fprintf(fp, "%d", tokens[i].token);

        if(tokens[i].token == 1) {
            fprintf(fp, " %d", tokens[i].index);
        }

        else if(tokens[i].token == 2) {
            fprintf(fp, " %s", tokens[i].lexeme);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

void writeNames() {
    FILE* fp = fopen("nametable.txt", "w");

    if(fp == NULL) {
        return;
    }

    for(int i = 0; i < nameCount; i++) {
        fprintf(fp, "%d %s %d %d\n", i, names[i].name, names[i].line, names[i].column);
    }
    fclose(fp);
}

int main(int argc, char* argv[]) 
{
  if(argc != 2)
  {
    printf("Usage: ./lex <input file>\n");
    return 1;
  }

  FILE* fp = fopen(argv[1], "rb");
  if(fp == NULL)
  {
    printf("Error: unable to open input file '%s'\n", argv[1]);
    return 1;
  }

  char input[10000];

  //scan fp all the way to the end of the file and add the characters to an array

  int ch;
  int line = 1;
  int column = 1;

  int n = 0;
  while((ch = fgetc(fp)) != EOF && n < 9999)
  {
    //add every bite to the stored array then evaluate
    input[n++] = ch;
  }
  input[n] = '\0';

  int i = 0;
  while(i < n)
  {
    char buff[64];
    int len = 0;

    if(input[i] == ' ' || input[i] == '\t') {
        i++;
        column++;
        continue;
    }

    if(input[i] == '\r') {
        line++;
        column = 1;
        continue;
    }

    if(input[i] == '\n') {
        line++;
        column = 1;
        i++;
        continue;
    }

    if(isLetter(input[i]))
    {
      int startLine = line;
      int startColumn = column;

      while(isLetter(input[i]))
      {
        //build word
        buff[len] = input[i];
        len++, i++;
      }
      buff[len] = '\0';
      int code = getCode(keywords, buff);

      if(code == -1)
      { //not a keyword, number, or symbol.
        code = 1 /*identifier code*/ ;
      }

      int index = findName(buff);
      
      if(index == -1) {
        index = nameCount;

        strcpy(names[nameCount].name, buff);
        names[nameCount].line = startLine;
        names[nameCount].column = startColumn;

        nameCount++;
      }

      addToken(buff, code, index);
    }

    else if(isNum(input[i]))
    {
      //build number
      while(isNum(input[i]))
      {
        buff[len] = input[i];
        len++, i++;
      }
      buff[len] = '\0';
      if(len > 6)
      {
        printError(3, line, column);//(row, col);
        return 1;
      }
      
      addToken(buff, 2, 0);
    }

    //build symbol
    else
    {
      buff[len++] = input[i++];
      switch (buff[0])
      {
        case '!':
        case '=':
        case '<':
        case'>':
        case':':
          if (input[i] == '=')
          {
            buff[len] = input[i];
            len++, i++;
          }
        break;
      }
      buff[len] = '\0';
      int code = getCode(symbols, buff);
      if(code == -1)
      {
        printResults();
        printError(1,line, column);
        exit(1);
      }
      addToken(buff, code, 0);
    }
  }
  printf("Source Program:\n\n");
  for(int i = 0; i < n; i++) {
    printf("%c", input[i]);
  }
  printf("\n");

  if(tokenCount == 0) {
    printResults();
    printError(11, 1, 1);
  }

  printResults();

  writeTokens();
  writeNames();

  fclose(fp);
  return 0;
}
