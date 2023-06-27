//受信バイトをポート番号ごとに出力ファイルを分ける
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_BUF 256
#define MAX_FLOW 50

int main(int argc, char *argv[]){
    FILE *fpr;
    FILE *fpw;
    char *rfile;//souce file
    char *wfile;
    int ports[MAX_FLOW];
    int acc_byte[MAX_FLOW];
    char fname[MAX_BUF];
    char eth[10];
    double time_h,time_m,time_s,time_ms,time,start_time;
    int tmp[4];
    int i;
    int k;
    int num=0;

    rfile = argv[1];
    strcpy(fname,rfile);
    strcat(fname,"-acc");
    wfile = fname; 

    //ファイルオープン
    if ((fpr = fopen(rfile,"r")) == NULL){
        fprintf(stderr, "error: can't open file %s\n",rfile);
        exit(-1);
    }
    if ((fpw = fopen(wfile,"w")) == NULL){
        fprintf(stderr, "error: can't open file %s\n",wfile);
        exit(-1);
    }

    while (fscanf(fpr,"%lf %lf %lf %lf %s %d %d", &time_h, &time_m, &time_s, &time_ms, eth, &tmp[0], &tmp[1]) != EOF){
	time = time_h*3600 + time_m*60 + time_s + time_ms*0.000001;
	  if(num == 0){
	    start_time = time;
            ports[num] = tmp[0];
	    acc_byte[0] = tmp[1];
            num++; 
	    fprintf(fpw, "%f %d %s %d %d\n", time-start_time, acc_byte[0], eth, num, tmp[1]);
	  }
	  else{
	    for(k=0;k<num;k++){
              if(ports[k] != tmp[0]){
                if(k == num-1){
                  ports[num] = tmp[0];
                  acc_byte[num] = tmp[1];
		  fprintf(fpw, "%f %d %s %d %d\n", time-start_time, acc_byte[num], eth, num+1, tmp[1]);
		  num++;
                  break;
                }
	      }
	      else{
                acc_byte[k] += tmp[1];
		fprintf(fpw, "%f %d %s %d %d\n", time-start_time, acc_byte[k], eth, k+1, tmp[1]);
                break;
              }
            }
 	  }
    }
    fprintf(fpw, "Number of ports : %d\n", num);

    printf("Number of ports : %d\n", num);
    for(i=0;i<num;i++){
      printf("%d %d\n", i+1, ports[i]);
    }
    fclose(fpr);
    fclose(fpw);
    return 0;
}
