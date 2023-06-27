#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define OUTPUTFILEDEBUG

struct manage_list{
  int pre_id;
  char filename;
  FILE *fp;
};

typedef struct link_list {
  int id;
  int buf_size;
  char *buf;
  struct link_list *next;
} LIST;

extern struct manage_list man_list;

LIST tail={32767, 3, "end", NULL};
LIST head={-32768, 5, "start", &tail};

int  insert_list(int, int, char *);
int  output_list(int);

int insert_list(int id, int buf_size, char *buf){
  LIST *inserted, *list_tem1, *list_tem2;
  size_t list_size;
  
  list_size = sizeof(LIST);//構造体の占有するバイト数
  //  inserted = (LIST *)malloc(list_size);
  if(NULL ==(inserted = (LIST *)malloc(list_size))){
    fprintf(stderr, "cannot secure memory: LIST\n");
    exit(1);
  }
  if(NULL == (inserted->buf = ((char *)malloc(sizeof(char) * (buf_size))))){
    fprintf(stderr, "cannot secure memory: buf\n");
    exit(1);
  }
  inserted->next = NULL;//以下連結リスト作成
  list_tem1 =& head;
  while(list_tem1->next != NULL){
    list_tem2 = list_tem1->next;
    if((id >= list_tem1->id)&&(id <= list_tem2->id))//連結リストに順番通りになるようにブロックを追加
      {
	inserted->id = id;
	inserted->buf_size = buf_size;
	memcpy(inserted->buf, buf, buf_size);
	inserted->next = list_tem1->next;
	list_tem1->next = inserted;
	break;
      }
    list_tem1 = list_tem2;
  }  
  return 1;
}

int output_list(int id){
  LIST *list_tem1, *list_tem2;
  list_tem1 =& head;
  while(1){
    list_tem2 = list_tem1->next;
    if(id == list_tem2->id)
      {
	list_tem1->next = list_tem2->next;//書き込んだブロックをリストから排除
	free(list_tem2->buf);
	free(list_tem2);
	break;
      }
    list_tem1 = list_tem2;
  } 
  return 1;
}

int file_wrap(int id, int size, char *buf){
  LIST *list_tem, *list_org;
  static int test = 0;
  int cnt = 0;
  insert_list(id, size, buf);
  list_tem =& head;
  //  while(list_tem->next != NULL && list_tem->next != &tail){
#ifdef OUTPUTFILEDEBUG//連結リスト表示
  while(list_tem->next != NULL) {
    list_tem = list_tem->next;
    printf("%d->", list_tem->id);
  }
  printf("\n");
  list_tem = &head;
#endif
  while(list_tem->next != NULL){
    list_org = list_tem;
    list_tem = list_tem->next;//リストを後方に１つずらす
    if(list_tem->id != man_list.pre_id){//ブロックが順番通りに届いていない場合は書き込まない(man_list.pre_id=次にファイルに書き込むブロック番号)
      //printf("list_tem->id:%d man_list.pre_id:%d\n",list_tem->id, man_list.pre_id);
      break;
    } else{ ////////writing///////
      fwrite(list_tem->buf, list_tem->buf_size, 1, man_list.fp);
      test += list_tem->buf_size;
#ifdef OUTPUTFILEDEBUG
      printf("block:%d,%d,write byte:%d \n",list_tem->id, man_list.pre_id, test);
#endif
      output_list(man_list.pre_id);//ファイルに書き込んだブロックをリストから排除
      list_tem = list_org;

      man_list.pre_id ++;//次に書き込むべきファイルを更新
    }
  }
  //  while(list_tem->next != NULL && list_tem->next != &tail) {
  while(list_tem->next != NULL) {//連結リストの中にあるブロック数をカウント
    cnt++;
    list_tem = list_tem->next;
  }
  return cnt;
}

