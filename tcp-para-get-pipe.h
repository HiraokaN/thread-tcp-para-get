#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_BUF 4000000
#define CRLF "\r\n"
#define EMPTY "\r\n\r\n"
#define MAX_BLK 50001
#define MALLOC(type) ((type *)malloc(sizeof(type))
#define MALLOC2(type, size) ((type *)malloc(sizeof(type) * (size)))
#define PIPENUM 1 //連続で同じ経路にリクエストする数

//#define REQ_TIME
//#define REC_TIME

struct range_req {
  int start;
  int end;
  int block_num;
};

typedef enum {
  EMPTY_NOT_FOUND,
  EMPTY_FOUND,
  BUFFER_REMAIN
} ProcessStat;

typedef enum {
  CHECK_BUFFER,//チェックしてねバッファを
  NEED_NEXT_READ//次行っていいよ
} ConnStat;

struct st_servers {
  char *fqdn;                 /* server FQDN */
  int port;                   /* server port */
  char *path;                 /* path to the directory (terminate with /) */
  char *filename;             /* file name */
  int fd;                     /* file descriptor of socket */ 
  char buf[MAX_BUF];          /* input buffer */
  int conn_ptr;               /* 0: parsing header, show progress */
  int buf_size;               /* reading data size */
  char *buf_offset;           /* start point of effective data */
  int progress;               /* progress in a block *///この経路で処理中のブロックの有無 0:リクエストあり
  ConnStat completion;        /* need next read or not */
  int current_block_number;   /* number of the assigned block */
  int pipecap;//連続で同じ経路にリクエストする数
  ProcessStat head;       /* EMPTY_NOT_FOUND, EMPTY_FOUND, or BUFFER_REMAIN */
  char header[MAX_BUF];       /* to store HTTP header */
  char *head_buf_ptr;         /* to manage header strings *///バッファの先頭ポインタ？
  int get_blo_num;
  char block[MAX_BUF];             /* to save block */
  float req_send_time;
  struct range_req range[PIPENUM];
  int next_range;
  int requested[MAX_BLK];
};

#include "tcp-para-get-servers.h"

struct filedivision {
  int filesize;                        /* file size */
  int ptr;                             /* next assignment starts from here */
  int block_completion[MAX_BLK];       /* 01 array for checking completion */ 
                                       /* [0]- */
  int block_count;                     /* block_number (1 - ) */
  float turn_time[MAX_BLK];            /*turn around time*/
  float requ_time[MAX_BLK];
  float recv_time[MAX_BLK];
  int fromsv[MAX_BLK];
} fdv;

struct manage_list{
  int pre_id;//?
  char filename;
  FILE *fp;
} man_list;

#ifdef REQ_TIME
struct reqest_list{
  int count;
  int sv[MAX_BLK];//from server
  float req_time[MAX_BLK];//ブロックをリクエストした時間
} req_list;
#endif 
 
#ifdef REC_TIME
struct receive_list{
  int count;
  int sv[MAX_BLK];//from server
  int num[MAX_BLK];//block number
  float rec_time[MAX_BLK];//ブロックをレシーブした時間
} rec_list;
#endif

void new_filedivision(int);
int get_filedivision(int, int *);
void assignment(int);
void initialization(void);
void *initial_assignment(void *);
int setup_connection(int);
void getfile(int, int, int);
void issue_request(int, int, int);
int make_request(char *, int, int, int);
int wait_completion(void);
void buffering_input(void);
void process_header(int);
void process_body(int);
int download_completion(void);
float timer(void);
int file_wrap(int, int, char *);
int http_head_check(int sv);
void mutex_lock(pthread_mutex_t *);
void mutex_unlock(pthread_mutex_t *);

int filesize=200000000; 
int divisions=10000;

int wait_pipe=1; /* non-stop */
/* int wait_pipe=PIPENUM  */ /* wait for PIPENUM blocks */

//pthread
pthread_mutex_t m_servers = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m_fdv = PTHREAD_MUTEX_INITIALIZER;
