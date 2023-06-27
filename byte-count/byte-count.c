#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_BUF 256

int main(int argc, char *argv[]){
    FILE *fpr;
    FILE *fpw;
    char *rfile;//souce file
    char *wfile;
    int ports[MAX_BUF];
    int byte[MAX_BUF];
    char fname[MAX_BUF];
    int tmp[4];
    int i;
    int k;
    int num=0;

    rfile = argv[1];
    strcpy(fname,rfile);
    strcat(fname,"-result");
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

    while (fscanf(fpr,"%d %d %d", &tmp[0], &tmp[1], &tmp[2]) != EOF){
        for(i=0;i<2;i++){
            if(tmp[i] != 80){
                if(num == 0){
                    ports[num] = tmp[i];
		    byte[0] = tmp[2];
                    num++; 
		    break;
                }
                for(k=0;k<num;k++){
                    if(ports[k] != tmp[i]){
                        if(k == num-1){
                            //printf("ports%d tmp%d num-1%d k%d\n",ports[k],tmp[i],num-1,k);
                            ports[num] = tmp[i];
			    byte[num] = tmp[2];
                            num++; 
			    break;
                        }
                       
                    }else{
           	          byte[k] += tmp[2];	
                          break;
                    }
                } 
            }
        }
    }

    fprintf(fpw, "Number of ports : %d\n", num);
    for(i=0;i<num;i++){
        fprintf(fpw, "%d %d\n", ports[i], byte[i]);
    }

    fclose(fpr);
    fclose(fpw);
    return 0;
}
