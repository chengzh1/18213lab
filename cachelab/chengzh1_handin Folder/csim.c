/*
 * Name: Cheng Zhang
 * Andrew ID: chengzh1
 * File Name: csim.c
 * Usage: Simulator Caches
 */

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "cachelab.h"

#define MAX_LINE 30

/* Struct used to simulator cache */
typedef struct CacheSimulator
{
    int valid;
    int tag;
}Cache;

int stringToint(char * s);
int myPow(int n);
unsigned long hexToint(char * str);
int findLine(Cache* cacheLine, int tagNum, int length);
int getLine(int* rowRecent, int length);

int main(int argc, char **argv){
    
    bool v = false;
    int s = 0, E = 0, b = 0, S = 0;
    int hit = 0, miss = 0, eviction = 0, time = 0;
    int setNum = 0, tagNum, lineNum;
    int ch, i, j, len, mask;
    char * t = NULL;
    char buf[MAX_LINE];
    FILE *fp;
    
    /* read parameter from command and set the cache's configuration */
    while((ch=getopt(argc,argv,"vsEbt"))!=-1)
    {
       
        switch(ch)
        {
            case 'v':
                v = true;
                break;
            case 's':
                s = stringToint(argv[optind]);
                break;
            case 'E':
                E = stringToint(argv[optind]);
                break;
            case 'b':
                b = stringToint(argv[optind]);
                break;
            case 't':
                 t = argv[optind];
                break;
            default:
                break;
        }
    }
    /* Calculate the number's size */
    S = myPow(s);

    /* define cache array and LRU records */
    Cache** myCache = (Cache **) malloc(S * sizeof(Cache*));
    int ** recent = (int **) malloc (S * sizeof(int *));
    for (i = 0; i < S; i ++){
        myCache[i] = (Cache *) malloc(E * sizeof(Cache));
        recent[i] = (int *) malloc (E * sizeof(int));
        memset(myCache[i], 0, E * sizeof(Cache));
        memset(recent[i], 0, E * sizeof(int));
    }
    
    /* open file */
    if((fp = fopen(t,"r")) == NULL)
    {
        perror("fail to read");
        exit (1) ;
    }
    
    /* read file line by line and store it into buf */
    while(fgets(buf,MAX_LINE,fp) != NULL)
    {
        len = strlen(buf);
        buf[len-1] = '\0';
        /* Skip the I instruction */
        if (buf[0] != ' ')
            continue;
        
        char instruction = buf[1];
        char address[65];
        unsigned long addr = 0;
        j = 0;
        i = 3;
        /* get address from buf */
        while(buf[i] != ','){
            address[j] = buf[i];
            j ++;
            i ++;
        }
        address[j] = '\0';
        /* extract which set it is and what tag it is */
        addr = hexToint(address);
        mask = (1 << s) - 1;
        setNum = (int)((addr >> b) & mask);
        tagNum = (int)(addr >> (s + b));
        /* find which line it is */
        lineNum = findLine(myCache[setNum], tagNum, E);
        /* according to lineNum, determining it is hit or miss */
        if (lineNum != -1){
            if (instruction == 'M')
                hit += 2;
            else
                hit += 1;
            if (v)
                printf("%s hit\n",buf);
        }else{
            miss ++;
            /* if miss, get a line to store */
            lineNum = getLine(recent[setNum], E);
            /* if valid = 0, no evicition */
            if (myCache[setNum][lineNum].valid == 0){
                myCache[setNum][lineNum].valid = 1;
                myCache[setNum][lineNum].tag = tagNum;
                if (instruction == 'M'){
                    hit ++;
                    if (v)
                        printf("%s miss hit\n",buf);
                }else{
                    if (v)
                        printf("%s miss\n",buf);
                }
            }else{
            /* case of eviction */
                eviction ++;
                myCache[setNum][lineNum].tag = tagNum;
                if (instruction == 'M'){
                    hit ++;
                    if (v)
                        printf("%s miss eviction hit\n",buf);
                }else{
                    if (v)
                        printf("%s miss eviction\n",buf);
                }

            }
        }
        /* update LRU */
        recent[setNum][lineNum] = time;
        time ++;
    }
    printSummary(hit,miss,eviction);
}

/*
 * stringToint - Function used to change string to int
 */
int stringToint(char * str){
    int i = 0;
    int num = 0;
    while(str[i]){
        num *= 10;
        num += str[i] - '0';
        i ++;
    }
    return num;
}

/*
 * myPow, Function used to calculate 2^n
 */
int myPow(int n){
    return 1 << n;
}

/*
 * hexToint - Function used to convert hex format address to int address
 */
unsigned long hexToint(char * str){
    unsigned long num = 0;
    int i = 0;
    while(str[i]){
        num *= 16;
        switch(str[i])
        {
            case 'a':
                num += 10;
                break;
            case 'b':
                num += 11;
                break;
            case 'c':
                num += 12;
                break;
            case 'd':
                num += 13;
                break;
            case 'e':
                num += 14;
                break;
            case 'f':
                num += 15;
                break;
            default:
                num += str[i] - '0';
        }
        i ++;
    }
    return num;
}

/*
 * findLine - Function used to find if the tag has already in the set.
 * If yes, return the number of the line
 * If no, return -1
 */
int findLine(Cache* cacheLine, int tagNum, int length){
    for (int i = 0; i < length; i ++){
        if (cacheLine[i].valid == 1 && cacheLine[i].tag == tagNum)
            return i;
    }
    return -1;
}

/*
 * getLine - Function used to find which line in a set should the 
 * element be stored, according to LRU principle
 */
int getLine(int* rowRecent, int length){
    int index = 0;
    int oldest = rowRecent[0];
    for (int i = 0; i < length; i ++){
        /* find the further recent line in a set */
        if (rowRecent[i] < oldest){
            oldest = rowRecent[i];
            index = i;
        }
    }
    return index;
}
