#include "tcp-para-get-pipe.h"

#define FIRST_RANGE(x) servers[x].range[(servers[x].next_range+servers[x].pipecap)%PIPENUM]
#define PFIRST_RANGE(x) (x)->range[((x)->next_range+(x)->pipecap)%PIPENUM]
/* PIPENUM is identical for each server in the current implementation */
#define MAX_SERV 50
#define DUP_TH 50//重複再要求の閾値
#define MAX_MYBUF 256

//#define DEBUG 1
//#define PROGRESSDEBUG 1
//#define DEBUGDUPLICATE 1
//#define OUTPUTRECVTIME
//#define OUTPUTFILEDLTIME
#define AVR_OUT_OF_ORDER_ARRIVALS
//#define BLOCK_RCV_TIME
#define BLOCK_REQ_REC
//#define REQPROCESSDEBUG

typedef enum {
  DUP_OFF = -1,
  DUP_TRIGGERED,
  DUP_INFLIGHT
} DupStat;

struct dupreq {
  //  int flag; /* -1:OFF 0:SELECTED 1:REQUESTING */
  DupStat flag;
  int blockid;
  int start;
  int end;
  int fd;
  int serverid;
} duplicated = { DUP_OFF, -1, -1, -1, -1, -1};

int process_buffers(struct st_servers *sv);

int remain_requests(void) {
  return !(fdv.ptr >= fdv.filesize ) || 0;
  /* need to check cancelled ones */
}

#include <err.h>//pthread
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;//pthread
		      
int main(int argc, char **argv) {
  int sv, i;
  int th,mutex;
  float begin, finish, dltime; 
  int head_request=0;
  double basetime = 0.0;
  pthread_t initial_assignment_thread;//pthread

#ifdef REQ_TIME
  req_list.count = 0;
#endif
  rec_list.count = 0;

  for (i=0; i<argc; i++) {//コマンドライン引数チェック
    if (head_request) {
      argv[i-1] = argv[i];//複数オプションから --head を消去
    }
    if (!strncmp(argv[i], "--head", strlen("--head"))) {
      head_request=1;
    }
  }
  argc -= head_request;
  if (argc == 2) {
    divisions = atoi(argv[1]);//ファイル分割数を引数から読み込み
  }

#ifdef DEBUG
  fprintf(stderr, "number of divisions: %d\n", divisions);
#endif
  assert(MAX_BLK>divisions);  /* MAX_BLK must be larger than divisions */ /*条件不成立なら終了*/

  if (head_request) {
    filesize = http_head_check(0); /* server 0 */
  }

  begin = timer();//DL開始時間を取得
  new_filedivision(filesize);//ファイルサイズ設定
  initialization();
  if((th = pthread_create(&initial_assignment_thread, NULL, &initial_assignment, NULL)) != 0)//pthread
    err(EXIT_FAILURE, "can not create thread: %s", strerror(th) );

  while(1 != download_completion()) {
    int sv;

    sv = wait_completion();//リクエストできるコネクションの空いたサーバーを返す
    if (sv == -1)
      continue;
#ifdef PROGRESSDEBUG
    printf("%f main(): servers[%d].progress %d\n", timer()-begin, sv, servers[sv].progress);
#endif
    if (servers[sv].progress > 0) {
#ifdef PROGRESSDEBUG
      printf("progress %d pipecap %d %s\n", servers[sv].progress, servers[sv].pipecap, servers[sv].progress>servers[sv].pipecap?"### progress is larger":"");
#endif
    if (remain_requests() || duplicated.flag==DUP_OFF){
      if (servers[sv].pipecap > 0)
	      assignment(sv); /* issue repetitively till pipecap==0 */
    }

    mutex_lock(&m_servers);
    servers[sv].progress = 0;
    mutex_unlock(&m_servers);
    }
  }
  fclose(man_list.fp);
  finish = timer();

  pthread_detach(initial_assignment_thread);//pthread
  pthread_mutex_destroy(&m_servers);//pthread
  pthread_mutex_destroy(&m_fdv);//pthread

  //printf("######download complete!!#####\n%fsec\n",finish - begin);
  //fprintf(stderr,"#####download complete!#####\n");
#ifndef SHOWRECVTIME
  dltime = finish - begin;
  //printf("%f %f\n", dltime < 0 ? (filesize/1000000*8)/(3600 + dltime) : (filesize/1000000*8)/dltime, dltime < 0 ? 3600 + dltime : dltime);//goodput[Mbps],DL-time,式２は短針が数字を跨いだ場合
  //printf("%f\n", dltime < 0 ? (filesize/1000000*8)/(3600 + dltime) : (filesize/1000000*8)/dltime);//グッドプット[Mbps](ファイルサイズ/全ファイルのDL時間)表示
#endif
#ifdef SHOWTURNAROUND
  for (sv = 0; servers[sv].fqdn != NULL; sv++) 
    printf("server(%d) -- %d blocks\n", sv, servers[sv].get_blo_num);

  for(i=0; i<fdv.block_count; i++){
      printf("%d %f\n", i+1, fdv.turn_time[i]);
    //printf("%f %d\n", fdv.recv_time[i] - begin, i+1);
  }
#endif
#ifdef SHOWRECVTIME
  printf("block id --- received time --- server\n");
  for(i=0; i<fdv.block_count; i++){
    if (i>0 && fdv.recv_time[i] + basetime < fdv.recv_time[i-1] - 3000)
      basetime += 3600;
    fdv.recv_time[i] += basetime;
    printf("%d %f %d\n", i, fdv.recv_time[i] - begin, fdv.fromsv[i]);
  }
#endif
#ifdef OUTPUTRECVTIME
  int x=0,y=0;
  for(i=0; i<fdv.block_count; i++){
    if (i>0 && fdv.recv_time[i] + basetime < fdv.recv_time[i-1] - 3000)
        basetime += 3600;
    fdv.recv_time[i] += basetime;
    if(fdv.fromsv[i] <= 9 ){
	x++;
        printf("%f -0.100000 %d\n", fdv.recv_time[i] - begin,(i+1)*(filesize/divisions)); 
    }else{
        y++;
        printf("-0.100000 %f %d\n", fdv.recv_time[i] - begin,(i+1)*(filesize/divisions)); 
    }  
  }
  printf("---------------------\n");
  for(i=0; i<fdv.block_count; i++){
    if(fdv.fromsv[i] > 9 ){
        printf("%f -0.100000 %d\n", fdv.recv_time[i] - begin,(i+1)*(filesize/divisions));
    }else{
        printf("-0.100000 %f %d\n", fdv.recv_time[i] - begin,(i+1)*(filesize/divisions));
    } 
  }
  printf("from path1: %d form path2: %d\n",x,y);
#endif
#ifdef OUTPUTFILEDLTIME
  FILE *fp1;
  char buf[MAX_MYBUF];
  char *wfile = "result.txt";//write file
  float dltime;
  float goodput;

  if(finish - begin > 0)
    dltime = finish - begin;
  else
    dltime = 3600 + finish - begin;
  goodput = (filesize/1000000*8)/dltime;//グッドプット[Mbps](ファイルサイズ/全ファイルのDL時間)表示

  sprintf(buf, "%lf %lf\n", goodput, dltime);

  //ファイルオープン
    if ((fp1 = fopen(wfile,"a+")) == NULL){
        fprintf(stderr, "error: can't open file %s\n",wfile);
        exit(-1);
    }
    fwrite(buf, strlen(buf), 1, fp1);

    fclose(fp1);

#endif
#ifdef AVR_OUT_OF_ORDER_ARRIVALS
  int o_arrival=0;//out_of_order_arrival
  int j;
  for(i=0; i<fdv.block_count; i++){
    if (i>0 && fdv.recv_time[i] + basetime < fdv.recv_time[i-1] - 3000)
        basetime += 3600;
    fdv.recv_time[i] += basetime;
    for(j=0; j<i; j++){
      if(fdv.recv_time[i] < fdv.recv_time[j]){
        o_arrival++;
      }
    } 
  }
  //printf("average number of out-of-order arrivals %lf\n", (float)o_arrival/fdv.block_count);
  printf("%f,%f,%f\n", (float)o_arrival/fdv.block_count, dltime < 0 ? (filesize/1000000*8)/(3600 + dltime) : (filesize/1000000*8)/dltime, dltime < 0 ? 3600 + dltime : dltime);//goodput[Mbps],DL-time,式２は短針が数字を跨いだ場合
#endif

#ifdef REQ_TIME
  FILE *fp2;
  char buf2[MAX_MYBUF];
  char *wfile2 = "request-time.txt";//write file
  basetime = 0;

  //ファイルオープン
  if ((fp2 = fopen(wfile2,"a+")) == NULL){
    fprintf(stderr, "error: can't open file %s\n",wfile2);
    exit(-1);
  }

  for(i=0; i<req_list.count-1; i++){
    if (i>0 && req_list.req_time[i] + basetime < req_list.req_time[i-1] - 3000)
        basetime += 3600;
    req_list.req_time[i] += basetime;
    //printf("%d reqest-time:%lf, from:%d\n", i, req_list.req_time[i]-begin, req_list.sv[i]);
    sprintf(buf2, "%lf %d\n", req_list.req_time[i]-begin, req_list.sv[i]);
    fwrite(buf2, strlen(buf2), 1, fp2);
  }
    fclose(fp2);
#endif
#ifdef REC_TIME
  FILE *fp3;
  char buf3[MAX_MYBUF];
  char *wfile3 = "receive-time.txt";//write file
  basetime = 0;
  int from_path1_blk=0;
  int from_path2_blk=0;
  int sv_thresh=10;//thresh of path1 and path2
  float path1_userate;
  float path2_userate;

  //ファイルオープン
  if ((fp3 = fopen(wfile3,"a+")) == NULL){
    fprintf(stderr, "error: can't open file %s\n",wfile3);
    exit(-1);
  }

  for(i=0; i<rec_list.count; i++){
    if(rec_list.sv[i] < sv_thresh){
      from_path1_blk++;
    }
    else{
      from_path2_blk++;
    }
    if (i>0 && rec_list.rec_time[i] + basetime < rec_list.rec_time[i-1] - 3000)
        basetime += 3600;
    rec_list.rec_time[i] += basetime;
    //printf("%d receive-time:%lf, from:%d\n", i, rec_list.rec_time[i]-begin, rec_list.sv[i]);
    sprintf(buf3, "%lf %d\n", rec_list.rec_time[i]-begin, rec_list.sv[i]);
    fwrite(buf3, strlen(buf3), 1, fp3);
  }
  //printf("%d %d %d\n", from_path1_blk, from_path2_blk,rec_list.count);
  path1_userate=(float)from_path1_blk/rec_list.count; 
  path2_userate=(float)from_path2_blk/rec_list.count;
  sprintf(buf3, "%lf %lf\n", path1_userate, path2_userate);
  fwrite(buf3, strlen(buf3), 1, fp3);
  fclose(fp3);
#endif

#ifdef BLOCK_RCV_TIME
  FILE *fp4;
  char buf4[MAX_MYBUF];
  char *wfile4 = "block-receive-time";//write file
  basetime = 0;

  //ファイルオープン
  if ((fp4 = fopen(wfile4,"a+")) == NULL){
    fprintf(stderr, "error: can't open file %s\n",wfile4);
    exit(-1);
  }

for(i=0; i<rec_list.count; i++){
    if (i>0 && rec_list.rec_time[i] + basetime < rec_list.rec_time[i-1] - 3000)
        basetime += 3600;
    rec_list.rec_time[i] += basetime;
    //printf("%d receive-time:%lf, from:%d\n", i, rec_list.rec_time[i]-begin, rec_list.sv[i]);
    sprintf(buf4, "%d %lf\n", rec_list.num[i], rec_list.rec_time[i]-begin);
    fwrite(buf4, strlen(buf4), 1, fp4);
  }
    fclose(fp4);
#endif

#ifdef BLOCK_REQ_REC
  for(i=0;i<fdv.block_count;i++){
    printf("%d,%lf,%lf,%d\n",i,fdv.requ_time[i]-begin,fdv.recv_time[i]-begin,fdv.fromsv[i]);  
  }
#endif

  return 0; 
}

void new_filedivision(int size) {
  fdv.filesize = size;
  fdv.ptr = 0;
}

int get_filedivision(int size, int *num) {//次のブロックのバイト数を返す
  int div;

  div = size;//ブロックさいず
  mutex_lock(&m_fdv);
  fdv.ptr += size;//次のブロック開始位置をずらす
  if (fdv.ptr > fdv.filesize) {//ブロックの開始位置がファイルサイズを超えた時？
    div = fdv.filesize - fdv.ptr;//最後のブロックサイズをファイルサイズぴったりに調整
    fdv.ptr = fdv.filesize;
  }
  *num = ++fdv.block_count;
  mutex_unlock(&m_fdv);
  if (div > MAX_BUF)
    fprintf(stderr, "*** Danger; div %d > MAX_BUF %d\n", div, MAX_BUF);//エラー：バッファ以上のブロックサイズ
  return div;
}

void requesting_process(int sv, int blocknum, int blockstart, int blockend) {//次のブロックを要求する下準備＋ファイル要求まで
  struct st_servers *sptr;
  int i;
#ifdef REQPROCESSDEBUG
  printf("requesting_process(): server %d fd %d blocknum %d blockstart %d blockend %d\n", sv, servers[sv].fd, blocknum, blockstart, blockend);
#endif
  mutex_lock(&m_servers);
  sptr = &(servers[sv]);
  sptr->range[sptr->next_range].block_num = blocknum;
  for (i=0; i<divisions; i++)
    if (sptr->requested[i] == -1) {
      sptr->requested[i] = blocknum;
      sptr->requested[i+1] = -1;
      break;
    }
  mutex_unlock(&m_servers);
  getfile(sv, blockstart, blockend);
  mutex_lock(&m_servers);
  sptr->range[sptr->next_range].start = blockstart;
  sptr->range[sptr->next_range].end = blockend;
  (sptr->next_range)++;
  (sptr->next_range) %= PIPENUM;
  (sptr->pipecap)--;
  mutex_unlock(&m_servers);
}

void issue_dup_request(int sv) {//重複再要求機能,svは見つけてきた空きコネクション
  int i, cnt=0, first=-1;
  int j, found, all_found, sv_found;
  int blockstart, blockend, blocksize, blocknum;
  for (i=0; i<divisions; i++) {
    if (first == -1 && fdv.block_completion[i] == 0) {//不連続の最初のブロックを探索
      first = i;
      cnt = 0;
    }
    if (first != -1 && fdv.block_completion[i] == 1)//不連続後の取得済みブロック数をカウント
      cnt++;
  }
#ifdef DEBUGDUPLICATE
  fprintf(stderr, "cnt from issue_dup_request(): %d\n", cnt);
#endif
  if (first == -1)//全ブロック取得済みのためリターン
    return;

  all_found = 1;
  for (j=0; servers[j].fqdn != NULL; j++) {
    found = 0;
    mutex_lock(&m_servers);
    for (i=0; i<divisions && servers[j].requested[i]!=-1; i++)//取得が遅れている接続を見つける
      if (servers[j].requested[i] == first) {
	      found = 1;
	      break;
      }
    mutex_unlock(&m_servers);
    if (j == sv) sv_found = found;//空きコネクションsvが、取得が遅れているブロックをすでに要求している接続だった場合
    all_found *= found;//全ての接続で既に、取得が遅れているブロックが要求済みだった場合：１
  }
  if (sv_found && !all_found) /* some other server is not used *///要求を出すべきか決定（上記より）
    return;

  blocksize = filesize/divisions;
  if (first <= filesize%divisions)
    blockstart = (blocksize + 1)*first;
  else
    blockstart = blocksize*first + filesize%divisions;
  if (first < filesize%divisions)
    blockend = blockstart + blocksize;
  else
    blockend = blockstart + blocksize - 1;
  if (blockend >= filesize)
    blockend = filesize - 1;

  blocknum = first;
#ifdef DEBUGDUPLICATE
  fprintf(stderr, "issue_dup_request() start %d end %d server %d block %d\n", blockstart, blockend, sv, blocknum);
#endif
  requesting_process(sv, blocknum, blockstart, blockend);

  duplicated.flag = DUP_INFLIGHT;
  duplicated.blockid = blocknum;
  duplicated.start = blockstart;
  duplicated.end = blockend;
  duplicated.fd = servers[sv].fd;
  duplicated.serverid = sv;
}


void assignment(int sv) {
  int filediff, blockstart, blockend, blocknum;
  if (servers[sv].pipecap <= 0) {
    fprintf(stderr, "Inapproriate pipecap number %d; check PIPENUM\n", servers[sv].pipecap);
    exit(-1);
  }
#ifdef DEBUG
  fprintf(stderr, "assignment Server %d Pipecap %d\n", sv, servers[sv].pipecap);
#endif
  assert(wait_pipe > 0);
  if (servers[sv].pipecap < wait_pipe)
    return;
  while (servers[sv].pipecap > 0) {
    if (duplicated.flag == DUP_TRIGGERED) {//重複再要求開始
      issue_dup_request(sv);
      if (duplicated.flag == DUP_INFLIGHT) /* dup req sent */
	continue;
    }
    mutex_lock(&m_fdv);
    blockstart = fdv.ptr;
    //    if (fdv.ptr >= fdv.filesize) /* finished */
    if (fdv.ptr >= fdv.filesize) { /* finished */
    mutex_unlock(&m_fdv);
#if DEBUG
      fprintf(stderr, "All requests have been issued.\n");
#endif
      return;
    }
    mutex_unlock(&m_fdv);
    filediff = filesize/divisions;
    if (filesize%divisions && fdv.block_count < filesize%divisions)
      filediff++;
    filediff = get_filedivision(filediff, &blocknum);
    blockend = fdv.ptr - 1;
    requesting_process(sv, blocknum, blockstart, blockend);
  }
}

void initialization(void){//struct server変数の初期化
  int sv;

  if((man_list.fp = fopen(servers[0].filename,"wb+")) == NULL){//新しいバイナリファイルを作る
    printf("cannot open file!\n");
    exit(1);
  }
  man_list.pre_id = 1;//ブロックを順番に書き込むための変数

  /*
   * current initial assignment policy:
   * for each server, fill the pipeline capacity first
   *
   * ALTERNATIVE should be rotate servers for one assignment and repeat
   * till pipeline capacity of all servers are filled up
   */
  for (sv = 0; servers[sv].fqdn != NULL; sv++) {
    if (sv >= MAX_SERV) {
      fprintf(stderr, "Warning: Unsupported number of servers > %d\n", MAX_SERV);//サーバ数上限チェック
    }
    servers[sv].requested[0] = -1;
    servers[sv].next_range = 0;
    servers[sv].conn_ptr = 0;
    servers[sv].head = EMPTY_NOT_FOUND;
    servers[sv].head_buf_ptr = servers[sv].header;
    if (servers[sv].pipecap <= 0)
      servers[sv].pipecap = PIPENUM;
    if (servers[sv].pipecap != PIPENUM) {
      fprintf(stderr, "Mismatched pipecap number %d against PIPENUM %d\n", servers[sv].pipecap, PIPENUM);
      exit(-1);
    }
  }
}

void *initial_assignment(void *p) {//要求するブロックの準備からファイル要求
  int filediff, blockstart, blockend, blocknum;
  int sv;
  int mutex;
  
  for (sv = 0; servers[sv].fqdn != NULL; sv++) {
    while (servers[sv].pipecap > 0) {
      mutex_lock(&m_fdv);
      blockstart = fdv.ptr;/* next assignment starts from here */
      filediff = filesize/divisions;//1ブロックサイズ
      if (filesize%divisions && fdv.block_count < filesize%divisions)
	      filediff++;
      mutex_unlock(&m_fdv);
      filediff = get_filedivision(filediff, &blocknum);//次に取得するブロックのバイト数を計算
      mutex_lock(&m_fdv);
      blockend = fdv.ptr - 1;//次に取得するブロックの最終バイト
      //      servers[sv].range[servers[sv].next_range].block_num = blocknum;
/*      servers[sv].current_block_number = blocknum; */
      fdv.block_completion[blocknum - 1] = 0;//初期化
      mutex_unlock(&m_fdv);
      requesting_process(sv, blocknum, blockstart, blockend);
      mutex_lock(&m_servers);
      servers[sv].progress = 0;//pthread
      mutex_unlock(&m_servers);
    }
  }

  return 0;//pthread
}

/* return server id if block completed */
int wait_completion(void) {
  static int i=0;
  int j, k, l;
  while(1){ 
    if(fdv.block_count >= divisions)//全てのブロックが取得できたら
      if(1 == download_completion())
	      return -1;

    j = i;
    if (servers[++i].fqdn == NULL)
      i=0;
#ifdef PROGRESSDEBUG
    printf("wait_completion(): servers[%d].progress %d\n", j, servers[j].progress);
#endif
    if (servers[j].progress > 0)
      return j;
    else if (servers[j].progress == 0)//pthread
      buffering_input();
    //if servers[j].progress = -1 nothing(wait for initial_assignment)
  } /* repeating till a block is completed */ 
}

void request_again(int sv) {
  int k, r;
  //  if (servers[sv].pipecap < PIPENUM) { /* requesting some blocks */
  //    r = (servers[sv].next_range - 1 + (servers[sv].next_range==0 ? PIPENUM : 0)) % PIPENUM;
#ifdef DEBUG
    fprintf(stderr, "request_again(): starting with range %d count %d\n", r, PIPENUM-servers[sv].pipecap);
#endif
    //    servers[sv].pipecap = 0;
    for (k=0; k<PIPENUM; k++) { /* requesting all */
    //    for (k=PIPENUM; k>servers[sv].pipecap; k--) {
      r = servers[sv].next_range + k;
      r %= PIPENUM;
      if (fdv.block_completion[servers[sv].range[r].block_num - 1] == 0)
        getfile(sv, servers[sv].range[r].start, servers[sv].range[r].end);
#ifdef REQAGAINDEBUG
      if (fdv.block_completion[servers[sv].range[r].block_num - 1] == 0)
	printf("yet ");
      else {
	printf("comp ");
	//	servers[sv].pipecap++;
      }
      printf("request_again(): server %d fd %d range[%d] start %d end %d\n", sv, servers[sv].fd, r, servers[sv].range[r].start, servers[sv].range[r].end);
#endif
      //      r = (r - 1 + (r==0?PIPENUM:0)) % PIPENUM;
      //    }
    }
}

void append_buf(char **appptr, char *start, int size) {
  memcpy(*appptr, start, size);
  *appptr += size;
  **appptr = '\0';
}

/* return pointer to body starting point if empty line found, otherwise NULL */
/* head_buf_ptr must be corrected out of this function */
char *check_header(struct st_servers *sv, char *buf, int bytes) {
  char *ptr;
  assert(*(buf+bytes)=='\0');
  sv->head = EMPTY_NOT_FOUND;
  if (NULL == (ptr = strstr(buf, EMPTY))) {//改行の位置を返す
    /* empty line not found */
    sv->completion = NEED_NEXT_READ;
  } else {
    sv->head = EMPTY_FOUND;
    *ptr = '\0'; /* for debug: print header */
    ptr += strlen(EMPTY);
    sv->completion = CHECK_BUFFER;
  }
  return ptr;
}

int receive_postprocess(struct st_servers *sv) {
  int count;
  int connnum;
  int block_num, start, end;
  mutex_lock(&m_servers);
  connnum = sv - servers;
  block_num = PFIRST_RANGE(sv).block_num;
  start = PFIRST_RANGE(sv).start;
  end = PFIRST_RANGE(sv).end;
  mutex_unlock(&m_servers);
  if (fdv.block_completion[block_num - 1] != 1) {
    count = file_wrap(block_num, end - start + 1, sv->block);//重複再要求用　不連続ブロックの数を返す
#ifdef PRINTBUFFERED
      fprintf(stderr, "receive_postprocess() file_wrap(%d) value: %d\n", block_num, count);
#endif
    mutex_lock(&m_fdv);
    fdv.block_completion[block_num - 1] = 1;
    fdv.turn_time[block_num - 1] = (timer() - sv->req_send_time);
    fdv.recv_time[block_num - 1] = timer();
    fdv.fromsv[block_num - 1] = connnum;
    mutex_unlock(&m_fdv);

    rec_list.rec_time[rec_list.count] = fdv.recv_time[block_num - 1];
    rec_list.sv[rec_list.count] = sv - servers;
    rec_list.num[rec_list.count] = block_num;
    //printf("%d receive-time:%lf, from:%d\n", rec_list.count, rec_list.rec_time[rec_list.count], rec_list.sv);//DL開始からの時間でないことに注意
    rec_list.count++;

  }
#ifdef REC_TIME
  else{
    rec_list.rec_time[rec_list.count] = timer();
    rec_list.sv[rec_list.count] = sv - servers;
    //printf("%d receive-time:%lf, from:%d\n", rec_list.count, rec_list.rec_time[rec_list.count], rec_list.sv);//DL開始からの時間でないことに注意
    rec_list.count++;
  }
#endif

  if (count > DUP_TH && duplicated.flag == DUP_OFF) {//重複再要求をするしない
    duplicated.flag = DUP_TRIGGERED;
  }
  if (duplicated.flag==DUP_INFLIGHT && block_num == duplicated.blockid) {//重複再要求完了
    duplicated.flag = DUP_OFF;
  }
  mutex_lock(&m_servers);
  if (++(sv->pipecap) > PIPENUM)
    sv->pipecap = PIPENUM;

  sv->get_blo_num++;
  sv->progress++;

  mutex_unlock(&m_servers);

#if PROGRESSDEBUG
    fprintf(stderr, "receive_postprocess(): block completion %d-%d from %d\n", start, end, sv-servers);
#endif
  return 0;
}

int range_debug(struct st_servers *sv, char *body_start, char *header_data) {
  int blockstart, blockend;
  int f_range;
  char t, *cr;
  int i;

  t = *(body_start - 1);
  *(body_start - 1) = '\0';
  cr = strstr(header_data, "Content-Range: ");
  if (cr != NULL) {
    sscanf(cr, "Content-Range: bytes %d-%d/%*d", &blockstart, &blockend);
    printf("Header Range: %d %d\n", blockstart, blockend);
    printf("server %ld fd %d first range: %d %d\n", sv-servers, sv->fd,
	   PFIRST_RANGE(sv).start, PFIRST_RANGE(sv).end);
    f_range = (sv->next_range+sv->pipecap)%PIPENUM;
    printf("sv->next_range %d sv->pipecap %d\n", sv->next_range, sv->pipecap);
    for (i=0; i<PIPENUM; i++) {
      if (i == sv->next_range) printf("+");
      if (i == f_range) printf("*");
      printf("range[%d]: start %d end %d num %d\n", i, sv->range[i].start, sv->range[i].end, sv->range[i].block_num);
    }
  } else {
    printf("Content-Range not found\n");
  }
  *(body_start - 1) = t;
  return 0;
}

int process_new_data(struct st_servers *sv) {
  char *body_start;
  int blocksize;
  int bsize;

  mutex_lock(&m_servers);
  assert(sv->buf_size > 0);
  sv->buf[sv->buf_size] = '\0';
  sv->completion = CHECK_BUFFER;
  sv->progress = 0; 

  if (sv->head == EMPTY_NOT_FOUND || sv->head == BUFFER_REMAIN) {//ヘッダの解析が済んでいない
    bsize = sv->head_buf_ptr - sv->header + sv->buf_size;
    if (bsize > MAX_BUF) fprintf(stderr, "DANGER: data size is larger than header buffer, %d > %d\n", bsize, MAX_BUF);
    append_buf(&(sv->head_buf_ptr), sv->buf, sv->buf_size);//バッファの先頭の位置を更新
  } else { /* sv->head==EMPTY_FOUND */
    char *tmp;
    tmp = sv->block + sv->conn_ptr;
    append_buf(&tmp, sv->buf, sv->buf_size);
    sv->conn_ptr = tmp - sv->block;
  }
  mutex_unlock(&m_servers);
  return 0;
}  

void buffering_input(void) {
  fd_set rset;
  int i, max_fd;
  int flag;
  struct timeval tv = {60, 0};
  int nfds;
  char *body_start;

  //  if (fdv.ptr >= fdv.filesize) { /* finished */
  ///*    fprintf(stderr, "finished process buffers\n"); */
  flag = 0;
  for (i=0; servers[i].fqdn != NULL; i++) {
      //      if (servers[i].head || servers[i].head_buf_ptr != servers[i].header)
#if PROGRESSDEBUG
    fprintf(stderr, "servers[%d].completion %d ", i, servers[i].completion);
    if (servers[i].completion == CHECK_BUFFER) {
      int tmp;
      tmp = process_buffers(&servers[i]); /* count conn. checking buffers */
      fprintf(stderr, "process_buffers returned %d", tmp);
      flag += tmp;
    }
    fprintf(stderr, "\n");
#else
    if (servers[i].completion == CHECK_BUFFER) /* more data to be processed */
      flag += process_buffers(&servers[i]); /* count conn. checking buffers */
#endif
  }
    //    if (1 == download_completion())
    //      return;
    //  }
  if (flag) {
#ifdef DEBUG
    fprintf(stderr, "More block in buffer: flag %d server 0 completion %d head_buf_ptr-header %d\n", flag, servers[0].completion, servers[0].head_buf_ptr-servers[0].header);
#endif
    return;
  }

  max_fd = 0;
  FD_ZERO(&rset);//ファイルディスクリプタの集合を空にする(selectの準備)
  mutex_lock(&m_servers);
  for (i=0; servers[i].fqdn != NULL; i++) {//ファイルディスクリプタの集合にファイルディスクリプタをセット
    if (servers[i].fd != -1)
      FD_SET(servers[i].fd, &rset);
    if (servers[i].fd > max_fd)
      max_fd = servers[i].fd;
  }
  mutex_unlock(&m_servers);
  
  //  nfds = select(max_fd+1, &rset, NULL, NULL, NULL);
#ifndef USESTDIO
  nfds = select(max_fd+1, &rset, NULL, NULL, &tv);
#endif
#ifdef PROGRESSDEBUG
  fprintf(stderr, "select() returned %d\n", nfds);
#endif
  if (nfds == 0) {//select timeout
    fprintf(stderr, "Timeout occurs\n");
  // if timeout then sweep buffers
    fflush(stderr);
    exit(1);
  } 
  
  for (i=0; servers[i].fqdn != NULL; i++) {//サーバー番号の若い順にディスクリプタを確認
    mutex_lock(&m_servers);
#ifdef USESTDIO
    {
#else
    if (servers[i].fd == -1){
      mutex_unlock(&m_servers);
      continue;
    }
    if (FD_ISSET(servers[i].fd, &rset)) {
#endif
#ifdef USESTDIO
      if (0 >= (servers[i].buf_size = read(0, servers[i].buf, MAX_BUF-1))) {
#else
      if (0 >= (servers[i].buf_size = read(servers[i].fd, servers[i].buf, MAX_BUF-1))) {//上手く読み込めなかった場合は再リクエスト
#endif
#ifdef DEBUG
	fprintf(stderr, "*** Maybe read error occured: %d %s closing server %d\n", servers[i].buf_size, strerror(errno), i);
#endif

	close(servers[i].fd);
	servers[i].fd = -1;
  mutex_unlock(&m_servers);
	request_again(i);
  mutex_lock(&m_servers);
	servers[i].head_buf_ptr=servers[i].header;
	servers[i].conn_ptr=0;
	servers[i].head=EMPTY_NOT_FOUND; // correct?
	servers[i].completion = NEED_NEXT_READ; // correct?
  mutex_unlock(&m_servers);
	//	servers[i].progress = 0;
	continue; /* nothing to be processed now */
      }
      mutex_unlock(&m_servers);
      process_new_data(&servers[i]);
      process_buffers(&servers[i]);
    }
    else
      mutex_unlock(&m_servers);
  }
}

int setup_connection(int sv) {
  struct hostent *host_ptr;
  struct in_addr **addr_ptr;
  struct sockaddr_in server;
  int sock;

  if ((host_ptr = gethostbyname(servers[sv].fqdn)) == NULL) {//IPアドレスから複数のサーバーIPアドレスを取得
    fprintf(stderr, "Error occured in gethostbyname()\n");
    return -1;
  }
  addr_ptr = (struct in_addr **)host_ptr->h_addr_list;

  for ( ; *addr_ptr != NULL; addr_ptr++) {
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
      fprintf(stderr, "Cannot open socket: %s\n", strerror(errno));
    }
    memset((char *)&server, 0, sizeof(server));
    server.sin_family = AF_INET;                   /* Address Family */
    server.sin_port = htons(servers[sv].port);     /* Server port */
    memcpy(&server.sin_addr, *addr_ptr, sizeof(struct in_addr));
    /* Server IP address */
  
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) == 0) {
      break; /* succeeded */
    }
    fprintf(stderr, "Cannot connect to server: %s\n", strerror(errno));
    close(sock);
  }
  if (*addr_ptr == NULL) {
    fprintf(stderr, "Cannot connect to any server\n");
    return -1;
  }
  return sock;
}

void getfile(int sv, int start, int end) {
#ifndef USESTDIO
  int k, r;

  if (servers[sv].fd == -1) {
    if ((servers[sv].fd = setup_connection(sv)) == -1) {//ソケット接続
      fprintf(stderr, "Connection setup failed for server %d\n", sv);
      exit(-1);
    }
#ifdef RECONNDEBUG
    printf("reconnect............................. server %d fd %d pipecap %d\n", sv, servers[sv].fd, servers[sv].pipecap);
    /* request again */
    printf("reconnecting status:\n");
#endif
    mutex_lock(&m_servers);
    for (k=0; k<PIPENUM-1; k++) {
      r = servers[sv].next_range + 1 + k;
      r %= PIPENUM;
      if (fdv.block_completion[servers[sv].range[r].block_num - 1] == 0)
	//	issue_request(sv, servers[sv].range[r].start, servers[sv].range[r].end);
#ifdef RECONNDEBUG
	printf("yet ")
#endif
	  ;
      else
#ifdef RECONNDEBUG
	printf("comp ")
#endif
	  ;
      //      printf("re-issueing start %d end %d\n", servers[sv].range[r].start, servers[sv].range[r].end);
#ifdef RECONNDEBUG
      printf("[%d] start %d end %d\n", r, servers[sv].range[r].start, servers[sv].range[r].end);
#endif
    }
    mutex_unlock(&m_servers);
  }
  
#endif
#if PROGRESSDEBUG
  fprintf(stderr, "getfile() fd: %d start: %d end: %d\n", 
	  servers[sv].fd, start, end);
#endif
  issue_request(sv, start, end);
}

void issue_request(int sv, int start, int end) {
  char req_buf[2000];
  int req_size;
  int write_size;
  int block_num;
  
  req_size = make_request(req_buf, sv, start, end);//リクエストヘッダ作成
  servers[sv].req_send_time = timer();
  block_num = servers[sv].range[0].block_num;
  fdv.requ_time[block_num-1] = servers[sv].req_send_time;
#ifdef REQ_TIME
  req_list.req_time[req_list.count] = servers[sv].req_send_time;
  req_list.sv[req_list.count] = sv;
  //printf("%d reqest-time:%lf, from:%d\n", req_list.count, req_list.req_time[req_list.count], sv);//DL開始からの時間でないことに注意
  req_list.count++;
#endif

#ifdef USESTDIO
  write_size = write(1, req_buf, req_size);
#else
  write_size = write(servers[sv].fd, req_buf, req_size); // issue the request 
#endif
#ifdef DEBUG
  if (write_size<0) {
    fprintf(stderr, "issue_request(): write error %s\n", strerror(errno));
    exit(1);
  } else
    fprintf(stderr, "issue_request(): wrote %d bytes to fd %d\n", write_size, servers[sv].fd);
#endif


  /* initialize data managing variables */
//  servers[sv].conn_ptr = 0;
//  servers[sv].head = 0;
//  servers[sv].head_buf_ptr = servers[sv].header;

}

int make_request(char *buf, int sv, int start, int end) {
  sprintf(buf, "GET %s%s HTTP/1.1%sHost: %s%sRange: bytes=%d-%d%s%s",
	  servers[sv].path, servers[sv].filename, CRLF,
	  servers[sv].fqdn, CRLF,
	  start, end, CRLF,
	  CRLF);
  return strlen(buf);
}

int make_head_request(char *buf, int sv) {
  sprintf(buf, "HEAD %s%s HTTP/1.1%sHost: %s%s%s",
	  servers[sv].path, servers[sv].filename, CRLF,
	  servers[sv].fqdn, CRLF,
	  CRLF);
  return strlen(buf);
}

int http_head_check(int sv) {
  char req_buf[MAX_BUF];
  int length;
  int read_bytes;
  char response_buf[MAX_BUF];
  char *ptr;
  int n, response_code;

  length = make_head_request(req_buf, sv);
  if ((servers[sv].fd = setup_connection(sv)) == -1) {
    fprintf(stderr, "Connection setup failed for server %d\n", sv);
    exit(-1);
  }
  write(servers[sv].fd, req_buf, length);
  ptr = response_buf;
  while(1) {
    if (0 >= (read_bytes = read(servers[sv].fd, ptr, MAX_BUF-1))) {
      fprintf(stderr, "Error occurred on receiving head information\n");
      exit(-1);
    }
    *(ptr+read_bytes) = '\0';
#ifdef DEBUG
    fprintf(stderr, "response buffer %s\n", response_buf);
#endif
    n = sscanf(response_buf, "%*s %d %*s", &response_code);
    if (n != 1 || response_code != 200) {
      fprintf(stderr, "HTTP response with unexpected format or code %03d\n", response_code);
      exit(-1);
    }
    if (NULL!= strstr(response_buf, EMPTY))
      break;
    ptr += read_bytes;
  }
  if (NULL == (ptr = strstr(response_buf, "Content-Length: "))) {
    fprintf(stderr, "Something wrong with http head response\n");
    exit(-1);
  }
  if (1 != sscanf(ptr+strlen("Content-Length: "), "%d", &length)) {
    fprintf(stderr, "Read error on http head response\n");
    exit(-1);
  }
  return length;
}

void dump_servers_struct(int sv) {
  fprintf(stderr, "conn_ptr=%d\n", servers[sv].conn_ptr);
  fprintf(stderr, "buf_size=%d\n", servers[sv].buf_size);
  fprintf(stderr, "completion=%d\n", servers[sv].completion);
  fprintf(stderr, "pipecap=%d\n", servers[sv].pipecap);
  fprintf(stderr, "head=%d\n", servers[sv].head);
  fprintf(stderr, "head_buf_ptr-header=%ld\n", servers[sv].head_buf_ptr-servers[sv].header);
  fprintf(stderr, "block=%s\n", servers[sv].block);
  fprintf(stderr, "next_range=%d\n", servers[sv].next_range);
}

/* return 1 if some data remain unprocessed */
/* buf_size is no more effective */
int process_buffers(struct st_servers *sv) {
  char *ptr;
  int blocksize;

  mutex_lock(&m_servers);
  sv->buf_size = 0;
  assert(sv->head_buf_ptr - sv->header >= 0);//?
  if (sv->head == EMPTY_NOT_FOUND || sv->head == BUFFER_REMAIN) {//ヘッダーが未解析
    /* first process without any read() ? */
    if (sv->head == EMPTY_NOT_FOUND && sv->head_buf_ptr == sv->header) {//
      sv->completion = NEED_NEXT_READ;
      mutex_unlock(&m_servers);
      return 0;
    }
    *(sv->head_buf_ptr) = '\0';
    if  (NULL == (ptr = check_header(sv, sv->header, sv->head_buf_ptr - sv->header))) {//本文の開始位置を返す
      sv->completion = NEED_NEXT_READ;
      mutex_unlock(&m_servers);
      return 0;
    }
    sv->head = EMPTY_FOUND;
#ifdef DEBUG
    fprintf(stderr, "RECEIVED header:\n%s\n\n", sv->header);
#endif
#ifdef RANGEDEBUG
    printf("RANGEDEBUG from process_buffers()\n");
    range_debug(sv, ptr, sv->header);
#endif
    /* header process end */
    if (sv->head_buf_ptr - ptr > 0) {
      char *tmp;
      tmp = sv->block;
      append_buf(&tmp, ptr, sv->head_buf_ptr - ptr);
      sv->conn_ptr = tmp - sv->block;
      assert(sv->conn_ptr == sv->head_buf_ptr - ptr);
    }
    mutex_unlock(&m_servers);
  }
  else 
    mutex_unlock(&m_servers);

#ifdef DEBUG
  fprintf(stderr, "process_buffers() servers[0].conn_ptr %d servers[0].buf_size %d\n", servers[0].conn_ptr, servers[0].buf_size);
#endif    
  mutex_lock(&m_servers);
  blocksize = PFIRST_RANGE(sv).end-PFIRST_RANGE(sv).start+1;
  assert(sv->head == EMPTY_FOUND);
  if (sv->conn_ptr < blocksize) {
#if DEBUG
    fprintf(stderr, "buf_size %d conn_ptr %d are smaller than block size %d\n", sv->buf_size, sv->conn_ptr, blocksize);
#endif
    sv->completion = NEED_NEXT_READ;
    mutex_unlock(&m_servers);
    return 0;
  }
  /* now we have more than one blocks to be processed */
  sv->head_buf_ptr = sv->header;
#ifdef DEBUG
  fprintf(stderr, "copy %d bytes to servers[sv].header\n", (sv->conn_ptr - blocksize));
#endif

  if (sv->conn_ptr - blocksize > 0) {
    append_buf(&(sv->head_buf_ptr), sv->block+blocksize, sv->conn_ptr-blocksize);
  }
  mutex_unlock(&m_servers);
#ifdef DEBUG
  fprintf(stderr, "servers[%d].head_buf_ptr-servers[%d].header=%d servers[%d].head: %d\n", sv-servers, sv-servers, (sv->head_buf_ptr-sv->header), sv-servers, sv->head);
  fprintf(stderr, "head_buf_ptr: %d ahead from header; header buffer status:\n%s**\n%d bytes\n", sv->head_buf_ptr-sv->header, sv->header, strlen(sv->header));
#endif

  receive_postprocess(sv);

  mutex_lock(&m_servers);
  sv->conn_ptr -= blocksize;
  mutex_unlock(&m_servers);
  if (sv->conn_ptr > 0) {
    sv->head = BUFFER_REMAIN;
    sv->completion = CHECK_BUFFER;
    return 1; /* more data to be processed */
  } else if (sv->conn_ptr == 0) { /* no remainder */
    sv->completion = NEED_NEXT_READ;
    sv->head = EMPTY_NOT_FOUND;
    return 0;
  }
  fprintf(stderr, "sv->conn_ptr must not be smaller than zero\n");
  exit(1);
}

int download_completion(void) {
  int i, completion=1;

  //  printf("fdv.block_count = %d\n", fdv.block_count);
/*  for (i=2; i<fdv.block_count; i++) { */
  mutex_lock(&m_fdv);
  for (i=0; i<divisions; i++) {
    completion *= fdv.block_completion[i];
#if 0
    printf("%d ", fdv.block_completion[i]);
#endif
  }
  mutex_unlock(&m_fdv);
#if 0
  printf("\n");
#endif
  return completion;
}

void mutex_lock(pthread_mutex_t *m){
  int mutex;
  if((mutex = pthread_mutex_lock(m)) != 0)
    errx(EXIT_FAILURE,"can not lock");
}

void mutex_unlock(pthread_mutex_t *m){
  int mutex;
  if((mutex = pthread_mutex_unlock(m)) != 0)
    errx(EXIT_FAILURE,"can not unlock");
}


 float timer(void){//時間取得
   char                date_c[9];      /* $BF|IU(B */
   char                time_c[9];      /* $B;~9o(B */
   char timer1[50];
   float timer;
   
   struct timeval   tv_t;
   struct timezone  tz_t;
   struct tm       *tm_p;

   memset( date_c, '\0', sizeof(date_c) );
   memset( time_c, '\0', sizeof(time_c) );
   
   gettimeofday( &tv_t, &tz_t );//現在の時刻取得
    tm_p = (struct tm *)localtime( (time_t *)&tv_t.tv_sec );//データを現地(日本)時間に変換
    /* $BF|IU(B/$B;~9o<hF@(B*/
    sprintf(timer1,"%d.%06ld", tm_p->tm_min * 60 + tm_p->tm_sec, tv_t.tv_usec);//時間を文字列で表現
    sscanf(timer1,"%f", &timer);//文字列を変数で表現
    return timer;
 }
 
