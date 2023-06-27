//culcurate initial buffuring time
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_BUF 256
#define MAX_FLOW 50

#define FILE_SIZE 160000000//20MB 
#define DIVISION 1000 
#define RATE 5000000


int main(int argc, char *argv[]){
    FILE *fpr;
    FILE *fpw;
    char *rfile;//souce file
    char *wfile;
    int block_num;
    int block_size;
    double init_buf_time;
    double max_init_buf_time=-1000;
    double ideal_blc_rcvtime;
    double rcv_time;
    int i;
    int k;
    int num=0;

    block_size=FILE_SIZE/DIVISION;

    rfile = argv[1];
    //strcpy(fname,rfile);
    //strcat(fname,"-acc");
    //wfile = fname; 

    //ファイルオープン
    if ((fpr = fopen(rfile,"r")) == NULL){
        fprintf(stderr, "error: can't open file %s\n",rfile);
        exit(-1);
    }
/*
    if ((fpw = fopen(wfile,"w")) == NULL){
        fprintf(stderr, "error: can't open file %s\n",wfile);
        exit(-1);
    }
*/

    while (fscanf(fpr,"%d %lf", &block_num, &rcv_time) != EOF){
      //printf("block_size/RATE= %lf\n", (double)block_size/RATE);
      ideal_blc_rcvtime = block_num*(double)block_size/RATE;
      init_buf_time = rcv_time-ideal_blc_rcvtime; 
      if(max_init_buf_time < init_buf_time){
        max_init_buf_time = init_buf_time;
      }
      printf("ideal_time=%lf rcv_time=%lf in_time=%lf\n", ideal_blc_rcvtime, rcv_time,init_buf_time);
    }
    //fprintf(fpw, "Number of ports : %d\n", num);
    printf("initial buffering time: %lf\n", max_init_buf_time);
    
    fclose(fpr);
    //fclose(fpw);
    return 0;
}
