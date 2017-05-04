//
//  main.c
//  macKeyboard_genareter
//
//  Created by huke on 4/19/17.
//  Copyright (c) 2017 com.cocoahuke. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <pthread/pthread.h>
#include <readline/readline.h>

static const char *arg_n = NULL;
static const char *arg_b = NULL;
static const char *arg_a = NULL;
static int arg_y = 0;

static char *generate_code_buf = NULL;
static size_t generate_code_buf_len = 0;

/*
 Arg:
 -n 指定字符串数组变量名字
 -b 读取文件以二进制
 -a 读取文件以ASCII
 -y 以数组形式输出
 */

static void usage(){
    printf("Generate string that format required for printf, use for Arduino Keyboard. Wrote by cocoahuke\n");
    printf("Usage:\t[-n <variable name>]\tspecifi name of variable\n");
    printf("\t[-b <file path>]\tread the file in binary and output it\n");
    printf("\t[-a <file path>]\tread the file in ASCII and output it\n");
    printf("\t[-y]\t\t\toutput in char array instead in one string\n");
}

uint32_t hash_lightly_str(void *buf, size_t size){
    unsigned hash = 0;
    uint32_t seed = 31;
    for (size_t i = 0; i < size; i++)
        hash = *(char*)buf + seed * hash;
    return hash;
}

char* string_format(char *fmt,...){
    if(!fmt)
        return NULL;
    char *str;
    va_list args;
    va_start (args, fmt);
    vasprintf(&str, (const char*)fmt, args);
    va_end (args);
    return str;
}

static void convert_fileInASCII(char *buf, size_t len){
    printf("\nKeyboard.print(F(\"");
    for(int i=0; i<len; i++){
        char c = buf[i];
        if(isascii(c)){
            if(iscntrl(c))
                printf("\"\"\\x%x\"\"", c);
            else
                printf("%c", c);
        }
        //printf("Keyboard.write(0x%x); //%c \n", c, iscntrl(c)?' ':c);
    }
    printf("\"));\n\n");
}

static void convert_fileInBinary(char *buf, size_t len){
    printf("\nKeyboard.print(F(\"");
    for(int i=0; i<len; i++){
        uint8_t d = buf[i];
        if(d>0x10)
            printf("%x", d);
        else
            printf("0%x", d);
    }
    printf("\"));\n\n");
}

static uint64_t file_getSize(const char *file_path){
    struct stat buf;
    
    if ( stat(file_path,&buf) < 0 )
    {
        perror(file_path);
        exit(1);
    }
    return buf.st_size;
}

char getcha() {
    char buf = 0;
    struct termios old = {0};
    if (tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0)
        perror ("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
        perror ("tcsetattr ~ICANON");
    return (buf);
}

void printAsArray(){
    uint64_t r_n = arc4random();
    char *r_varn = string_format("_%x", hash_lightly_str(&r_n, sizeof(r_n)));
    printf("[print result and end program]:\n");
    printf("\nstatic const char %s[] PROGMEM = {", arg_n?arg_n:r_varn);
    
    for(size_t chari=0;chari<generate_code_buf_len;chari++){
        char s = ((char*)generate_code_buf)[chari];
        
        if(s=='\\')
            printf("\'\\\\\'");
        else if(s=='\'')
            printf("\'\\'\'");
        else if(s=='\n')
            printf("\'\\n\'");
        else if(s=='\t')
            printf("\'\\t\'");
        else
            printf("\'%c\'",s);
        
        if(chari==generate_code_buf_len-1)
            printf("");
        else
            printf(", ");
    }
    printf("};\n");
    printf("Keyboard.print((const __FlashStringHelper *)%s);\n\n", arg_n?arg_n:r_varn);
    free(r_varn);
    
}

void printAsOne(){
    uint64_t r_n = arc4random();
    char *r_varn = string_format("_%x", hash_lightly_str(&r_n, sizeof(r_n)));
    printf("[print result and end program]:\n");
    printf("\nconst __FlashStringHelper *%s = \"", arg_n?arg_n:r_varn);
    
    char repl_slash[] = {'"', '\\'};
    //{{'\a', 0x07}, {'\b', 0x08}, {'\f', 0x0C}, {'\n', 0x0A}, {'\r', 0x0D}, {'\t', 0x09}, {'\v', 0x0B}};
    
    struct ctlChar_stru{
        char ctlChar;
        char *ctlStr;
    };
    
    struct ctlChar_stru repl_ctlChar[] = {{'\a', "\\a"}, {'\b', "\\b"}, {'\f', "\\f"}, {'\n', "\\n"}, {'\r', "\\r"}, {'\t', "\\t"}, {'\v', "\\v"}};
    
    for(size_t chari=0;chari<generate_code_buf_len;chari++){
        char s = ((char*)generate_code_buf)[chari];
        
        for(int i=0;i < sizeof(repl_slash);i++)
            if(repl_slash[i]==s){
                printf("\\%c", s);
                goto repl_done;
            }
        
        for(int i=0;i < sizeof(repl_ctlChar)/sizeof(repl_ctlChar[0]);i++)
            if(repl_ctlChar[i].ctlChar==s){
                printf("%s", repl_ctlChar[i].ctlStr);
                goto repl_done;
            }
        
        
        printf("%c",s);
    repl_done:
        continue;
    }
    printf("\";\n");
    printf("Keyboard.print(%s);\n\n", arg_n?arg_n:r_varn);
    free(r_varn);
    
}

static void sig_catch(int sig){
    
    if(!generate_code_buf_len)
        return;
    
    char op = '\0';
    printf("[Whether to add Return to the end] (Y/N/C): ");
    scanf("%c",&op);
    if(toupper(op)=='Y'){
        
    }
    else if(toupper(op)=='N'){
        generate_code_buf_len-=1;
    }
    else if (toupper(op)=='C'){
        return;
    }
    
    if(arg_y)
        printAsArray();
    else
        printAsOne();
    exit(1);
}

int main(int argc, const char * argv[]) {
    
    for(int i=0;i<argc;i++){
        if(!strcmp(argv[i],"-h")){
            usage();exit(1);
        }
        if(!strcmp(argv[i],"-n")){
            arg_n = (i=i+1)>=argc?NULL:argv[i];
        }
        if(!strcmp(argv[i],"-y")){
            arg_y = 1;
        }
        if(!strcmp(argv[i],"-b")){
            arg_b = (i=i+1)>=argc?NULL:argv[i];
            FILE *fp = fopen(arg_b, "r");
            if(!fp){
                printf("file(%s) is not exist\n", arg_b); return 0;
            }
            uint64_t fsize = file_getSize(arg_b);
            void *buf = malloc(fsize);
            fread(buf, 1, fsize, fp);
            convert_fileInBinary(buf, fsize);
            free(buf);
            return 0;
        }
        if(!strcmp(argv[i],"-a")){
            arg_a = (i=i+1)>=argc?NULL:argv[i];
            FILE *fp = fopen(arg_a, "r");
            if(!fp){
                printf("file(%s) is not exist\n", arg_a); return 0;
            }
            uint64_t fsize = file_getSize(arg_a);
            void *buf = malloc(fsize);
            fread(buf, 1, fsize, fp);
            convert_fileInASCII(buf, fsize);
            free(buf);
            return 0;
        }
    }
    
    rl_bind_key('\t', rl_insert);
    signal(SIGINT, sig_catch);
    
    while(1){
        char *inputStr = readline(NULL);
        size_t inputStr_len = inputStr?strlen(inputStr):0;
        
        if(!generate_code_buf)
            generate_code_buf = malloc(inputStr_len+1);
        else
            generate_code_buf = realloc(generate_code_buf, generate_code_buf_len+inputStr_len+1);
        
        if(inputStr_len)
            memcpy(generate_code_buf+generate_code_buf_len, inputStr, inputStr_len);
        generate_code_buf_len+=inputStr_len;
        memset(generate_code_buf+generate_code_buf_len, '\n', 1);
        generate_code_buf_len+=1;
        
        free(inputStr);
    }
    
    return 0;
}
