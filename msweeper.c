#include <curses.h>
#include <stdlib.h>
#include <time.h>
#define BOMB 9
#define BX 8
#define BY 4
#define NUM_CLR 4 // color_pairの数字の色が始まる位置
#define DEFAULT_CLR 3 // 通常色
#define SELECTED_DEKO 1 // 凸の色
#define SELECTED_BOKO 2 // 凹の色
#define DIRECTION_NUM 8
#define GAME_QUIT 'q'
#define GAME_OVER 'o'
#define GAME_CLEAR 'k'
#define GAME_CONTINUE 0
#define NOT_OPEN 0
#define OPEN 'z'
#define FLAG 'x'
#define UNKNOWN 'c'

// マインスイーパのボード
typedef struct board {
  int **data;
  int **board;
  int bomb_num;
  int x;
  int y;
} BOARD;

// 場所
typedef struct point {
  int x, y;
} POINT;

// 経過時間のための構造体
typedef struct time_var {
  time_t std_time;
  time_t pre_time;
  time_t diff_time;
} TIMES;

void init(BOARD * board, POINT *point, int x, int y, int bomb_num);
void init_color_pair();
void init_point(POINT *point);
void init_board(BOARD *board, int x, int y, int bomb_num);
void init_remake(BOARD *board, int x, int y, int bomb_num);

void set_option(int argc, char *argv[], BOARD *board);

int game_main(BOARD *board, POINT *point);
int set_flag_quest(BOARD *board, POINT *point, int key);
int move_point(BOARD *board, POINT *point, int key);
int before_start_key(BOARD *board, POINT *point);
int game_end_key();
int main_key(BOARD *board, POINT *point);

int open_block(BOARD *board, POINT point);
int open_surround(BOARD *board, POINT point);
void open_all(BOARD *board);
void open_loop(BOARD *board, POINT point);
void return_surroundings(int direction, POINT *point);

void set_board(BOARD *board, POINT point);
void set_board_num(BOARD *board);
void print_board(BOARD board, POINT point);

int check_in_scope(BOARD *board, POINT point);
int check_gameover(BOARD board, POINT point);
int disposal_gameover(BOARD *board, POINT *point, int game_flag);

void delete_board(BOARD *board);
void end(BOARD *board);

void init_time(TIMES *mytime);
void renew_time(TIMES *mytime);
void disp_time(TIMES mytime);

int main(int argc, char *argv[]) {
  BOARD board;
  POINT point;

  // デフォルト
  board.x = 9;
  board.y = 9;
  board.bomb_num = 10;

  set_option(argc, argv, &board);
  init(&board, &point, board.x, board.y, board.bomb_num);

  // ゲームを続けるとループ
  while(!disposal_gameover(&board, &point, game_main(&board, &point))) {
    init_remake(&board, board.x, board.y, board.bomb_num); // ボードを作り直し
  }

  end(&board);

  return 0;
}

void set_option(int argc, char *argv[], BOARD *board) {
  int num, error = 0;

  // オプションが無かったら
  if(argc == 1) return;
  else if(argc == 2)
    if(strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-help") == 0) {
      printf("%s [size_x] [size_y] [bomb_num] \nsousa_key: [z]=open, [x]=setflag, [c]=setquestion, [q]=quit\n", argv[0]);
      exit(0);
    }
    else error = 1;
  // オプションが多すぎる場合
  else if(argc > 4 || argc < 4) error = 1;
  // オプションの数が適切
  else if(argc == 4) {
    num = atoi(argv[1]);
    if(num == 0) error++;
    board->x = num;
    num = atoi(argv[2]);
    if(num == 0) error++;
    board->y = num;
    num = atoi(argv[3]);
    if(num == 0) error++;
    board->bomb_num = num;
    if(board->bomb_num >= board->x * board->y) error = 1;
  }
  for(num = 0; num < error; num++) {
    puts("引数が間違っています。");
    exit(1);
  }
}

int game_main(BOARD *board, POINT *point) {
  int flag = 1;
  int i = 0;
  TIMES game_time;

  // 最初のブロックを開くまでの処理
  while(flag = before_start_key(board, point) != 1) {
    if(flag != 0) print_board(*board, *point);
    refresh();
  }
  set_board(board, *point);
  open_block(board, *point);
  init_time(&game_time);
  flag = 1;

  // メインループ
  while(flag != GAME_OVER && flag != GAME_CLEAR) {
    if(flag != 0) print_board(*board, *point);
    renew_time(&game_time);
    disp_time(game_time);
    flag = main_key(board, point); 
    refresh();
  }
  
  return flag;
}

int move_point(BOARD *board, POINT *point, int key) {
  switch(key) {
  case KEY_UP:
    point->y = ((point->y-1) + board->y) % board->y;
    break;
  case KEY_RIGHT:
    point->x = (point->x+1) % board->x;
    break;
  case KEY_DOWN:
    point->y = (point->y+1) % board->y;
    break;
  case KEY_LEFT:
    point->x = ((point->x-1) + board->x) % board->x;
    break;
  }
}

int set_flag_quest(BOARD *board, POINT *point, int key) {
  // 押されたキーが旗でも?でもなかったら
  if(key != FLAG && key != UNKNOWN) return 1;
  // 旗か?を設置
  if(board->board[point->y][point->x] != OPEN && board->board[point->y][point->x] != key) 
    board->board[point->y][point->x] = key;
  else if(board->board[point->y][point->x] == key) 
    board->board[point->y][point->x] = NOT_OPEN;
  return key;
}

// 最初のブロックを開くまでに使うキーの処理
int before_start_key(BOARD *board, POINT *point) {
  int key;
  key = getch();

  switch(key) {
  case KEY_UP:
  case KEY_RIGHT:
  case KEY_DOWN:
  case KEY_LEFT:
    move_point(board, point, key);
    return key;
    break;
  case OPEN:
    return 1;
  case GAME_QUIT:
    end(board);
    exit(0);
    break;
  }

  return 0;
}

// ゲームを終了するか選択する時のキーの処理
int game_end_key(BOARD *board) {
  int key, loop = 1;

  while(loop) {
    key = getch();
    switch(key) {
    case 'y':
      return 0;
      break;
    case 'n':
    case GAME_QUIT:
      return 1;
      break;
    }
  }

  return 1;
}

// ゲーム中のキーの処理
int main_key(BOARD *board, POINT *point) {
  int key, return_num = 0, dummy;
  key = getch();

  switch(key) {
  case KEY_UP:
  case KEY_RIGHT:
  case KEY_DOWN:
  case KEY_LEFT:
    move_point(board, point, key);
    return_num = key;
    break;
  case OPEN:
    return_num = open_block(board, *point);
    dummy = check_gameover(*board, *point);
    if(dummy != GAME_CONTINUE) return_num = dummy;
    break;
  case FLAG:
  case UNKNOWN:
    return_num = set_flag_quest(board, point, key);
    dummy = check_gameover(*board, *point);
    if(dummy != GAME_CONTINUE) return_num = dummy;
    break;
  case GAME_QUIT:
    end(board);
    exit(0);
    break;
  }

  return return_num;
}

// 全部初期化
void init(BOARD * board, POINT *point, int x, int y, int bomb_num) {
  srand((unsigned)time(NULL));

  // curses初期化
  initscr();
  start_color();
  init_color_pair();
  noecho();
  nonl();
  intrflush(stdscr, FALSE);
  cbreak();
  timeout(0);
  keypad(stdscr, TRUE);
  curs_set(0);
  clear();
  // ボード初期化
  init_board(board, x, y, bomb_num);
  // ポイント初期化
  init_point(point);
}

void init_point(POINT *point) {
  point->x = 0;
  point->y = 0;
}

void init_color_pair() {
  int i = 1;

  init_pair(1, COLOR_WHITE, COLOR_GREEN);  // 選択されてる凹んでるところ
  init_pair(2, COLOR_WHITE, COLOR_BLUE);   // 選択されてる出っ張ってるところ
  init_pair(3, COLOR_WHITE, COLOR_BLACK);  // デフォルト
  init_pair(4, COLOR_BLACK, COLOR_BLACK);  // 0
  init_pair(5, COLOR_RED, COLOR_BLACK);    // 1
  init_pair(6, COLOR_BLUE, COLOR_BLACK);   // 2
  init_pair(7, COLOR_GREEN, COLOR_BLACK);  // 3
  init_pair(8, COLOR_CYAN, COLOR_BLACK);   // 4
  init_pair(9, COLOR_YELLOW, COLOR_BLACK); // 5
  init_pair(10, COLOR_MAGENTA, COLOR_BLACK);// 6
  init_pair(11, COLOR_RED, COLOR_BLACK);    // 7
  init_pair(12, COLOR_BLUE, COLOR_BLACK);   // 8
  init_pair(13, COLOR_RED, COLOR_BLACK);    // BOMB
}

void init_board(BOARD *board, int x, int y, int bomb_num) {
  int i, j;
 
  board->x = x;
  board->y = y;
  board->bomb_num = bomb_num;
  // 2次元配列作成
  board->data = (int **)calloc(y, sizeof(int *));
  for(i = 0; i < y; i++) {
    board->data[i] = (int *)calloc(x, sizeof(int));
  }
  
  // 2次元配列作成
  board->board = (int **)calloc(y, sizeof(int *));
  for(i = 0; i < y; i++) {
    board->board[i] = (int *)calloc(x, sizeof(int));
    for(j = 0; j < x; j++) {
      board->board[i][j] = NOT_OPEN;
    }
  }
}

void init_remake(BOARD *board, int x, int y, int bomb_num) {
  delete_board(board);
  init_board(board, x, y, bomb_num);
  clear();
}

int open_surround(BOARD *board, POINT point) {
  POINT p;
  int i, num = 0, result;

  if(board->data[point.y][point.x] == 0 || board->data[point.y][point.x] == BOMB) return 0;
  // 選択している位置の周りに設置した旗の数を数える
  for(i = 0; i < DIRECTION_NUM; i++) {
    p.x = point.x;
    p.y = point.y;
    return_surroundings(i, &p);
    if(check_in_scope(board, p)) {
      if(board->board[p.y][p.x] == FLAG) num++;
      if(board->board[p.y][p.x] == UNKNOWN) return NOT_OPEN;
    }
  }
  // 数字と旗の数があっていたら
  if(num == board->data[point.y][point.x]) {
    for(i = 0; i < DIRECTION_NUM; i++) {
      p.x = point.x;
      p.y = point.y;
      return_surroundings(i, &p);
      if(check_in_scope(board, p)) {
	// ブロックを開く
	if(board->data[p.y][p.x] == BOMB && board->board[p.y][p.x] == NOT_OPEN) {
	  board->board[p.y][p.x] = OPEN;
	  return BOMB;
	}
      }
      // 自動的にブロックを開いていく
      open_loop(board, p);
    }
  }
  return OPEN;
}

int open_block(BOARD *board, POINT point) {
  POINT p;
  int i, result;
 
  if(board->board[point.y][point.x] == NOT_OPEN) {
    board->board[point.y][point.x] = OPEN;
  }else if(board->board[point.y][point.x] == OPEN) {
    // 開いている数字のブロックをを押したとき
    result = open_surround(board, point);
    return result;
  }
  // 数字が0以外の時は自動的に開かない
  if(board->data[point.y][point.x] != 0) return OPEN;
  if(board->data[point.y][point.x] == BOMB) return BOMB;
  for(i = 0; i < DIRECTION_NUM; i++) {
    p.x = point.x;
    p.y = point.y;
    return_surroundings(i, &p);
    open_loop(board, p);
  }

  return OPEN;
}

void open_loop(BOARD *board, POINT point) {
  POINT p;
  int i;
 
  if(!check_in_scope(board, point)) return;
  if(board->board[point.y][point.x] != NOT_OPEN) return;
  if(board->data[point.y][point.x] != BOMB) 
    board->board[point.y][point.x] = OPEN;
  // 数字があったら再起を止める
  if(board->data[point.y][point.x] != 0) return;
  for(i = 0; i < DIRECTION_NUM; i++) {
    p.x = point.x;
    p.y = point.y;
    return_surroundings(i, &p);
    open_loop(board, p);
  }
}

void open_all(BOARD *board) {
  int i, j;
  for(i = 0; i < board->y; i++) {
    for(j = 0; j < board->x; j++) {
      board->board[i][j] = OPEN;
    }
  }
}

void set_board(BOARD *board, POINT point) {
  int i, j, dummy, dx, dy, bomb_num = 0;

  for(i = 0; i < board->y; i++) {
    for(j = 0; j < board->x; j++) {
      if(bomb_num < board->bomb_num) {
	if(i != point.y && j != point.x) { 
	  // 最初に開けた場所は爆弾にならない
	  board->data[i][j] = BOMB;
	  bomb_num++;
	}
      }
    }
  }
  
  for(i = 0; i < board->y; i++) {
    for(j = 0; j < board->x; j++) {
      dummy = board->data[i][j];
      dx = rand()%board->x, dy = rand()%board->y;
      if(dy != point.y && dx != point.x && i != point.y && j != point.x) { 
	// 最初に開けた場所は爆弾にならない
	board->data[i][j] = board->data[dy][dx];
	board->data[dy][dx] = dummy;
      }
    }
  }
  set_board_num(board);
}

void set_board_num(BOARD *board) {
  POINT p;
  int i, j, k;
  int bomb_num = 0;


  for(i = 0; i < board->y; i++) {
    for(j = 0; j < board->x; j++) {
      if(board->data[i][j] != BOMB) {
	bomb_num = 0;
	for(k = 0; k < DIRECTION_NUM; k++) {
	  p.x = j;
	  p.y = i;
	  return_surroundings(k, &p);
	  // 周りの爆弾の数を入れる
	  if(check_in_scope(board, p))
	    if(board->data[p.y][p.x] == BOMB) bomb_num++;
	}
	board->data[i][j] = bomb_num;
      }
    }
  }
}

int check_in_scope(BOARD *board, POINT point) {
  if(point.x >= 0 && point.x < board->x && point.y >= 0 && point.y < board->y)
    return 1;
  else return 0;
}

int check_gameover(BOARD board, POINT point) {
  int i, j;

  // ゲームオーバー判定
  for(i = 0; i < board.y; i++) {
    for(j = 0; j < board.x; j++) {
      if(board.board[i][j] == OPEN && board.data[i][j] == BOMB) return GAME_OVER;
    }
  }
  // ゲームクリア判定
  for(i = 0; i < board.y; i++) {
    for(j = 0; j < board.x; j++) {
      if(board.board[i][j] == UNKNOWN) return 0;
      if(board.board[i][j] != OPEN && board.data[i][j] != BOMB) return GAME_CONTINUE;
    }
  }

  return GAME_CLEAR;
}

void return_surroundings(int direction, POINT *point) {
  switch(direction) {
  case 0: // 上
    (point->y)--;
    break;
  case 1: // 右斜め上
    (point->y)--; (point->x)++;
    break;
  case 2: // 右
    (point->x)++;
    break;
  case 3: // 右斜め下
    (point->y)++; (point->x)++;
    break;
  case 4: // 下
    (point->y)++;
    break;
  case 5: // 左斜め下
    (point->y)++; (point->x)--;
    break;
  case 6: // 左
    (point->x)--;
    break;
  case 7: // 左斜め上
    (point->y)--; (point->x)--;
    break;
  }
}

// 時間を測る関数を使う準備
void init_time(TIMES *mytime) {
  time(&mytime->std_time);
  mytime->pre_time = mytime->std_time;
}

// 経過時間を表示
void disp_time(TIMES mytime) {
  attron(COLOR_PAIR(DEFAULT_CLR));
  mvprintw(1, BX, "%2d:%2d:%2d", (mytime.diff_time)/3600 % 100, (mytime.diff_time)/60 % 60, (mytime.diff_time) % 60);
}

// 経過時間を更新
void renew_time(TIMES *mytime) {
  time(&mytime->pre_time);
  mytime->diff_time = difftime(mytime->pre_time, mytime->std_time);
}

void print_board(BOARD board, POINT point) {
  int i, j;
  
  for(i = 0; i < board.y; i++) {
    for(j = 0; j < board.x; j++) {
      switch(board.board[i][j]) {
      case OPEN: // 開いている場所だったら
	// 色の設定
	attron(COLOR_PAIR(NUM_CLR + board.data[i][j]));
	if(point.x == j && point.y == i) attron(COLOR_PAIR(SELECTED_BOKO));
	// 文字表
	if(board.data[i][j] == 0) mvprintw(BY+i, BX+j*2, "  "); // 0
	else if(board.data[i][j] == BOMB) mvprintw(BY+i, BX+j*2, "◎"); // BOMB
	else mvprintw(BY+i, BX+j*2, "%d ", board.data[i][j]); // NUM
	break;
      case NOT_OPEN: // 開いていない場所だったら
	// 色の設定
	if(point.x == j && point.y == i) attron(COLOR_PAIR(SELECTED_DEKO));
	else attron(COLOR_PAIR(DEFAULT_CLR));
	// 文字表示
	mvprintw(BY+i, BX+j*2, "■");
	break;
      case FLAG: // 旗のブロック
	if(point.x == j && point.y == i) attron(COLOR_PAIR(SELECTED_DEKO));
	else attron(COLOR_PAIR(DEFAULT_CLR));
	// 文字表示
	mvprintw(BY+i, BX+j*2, "P ");
	break;
      case UNKNOWN: // ?のブロック
	if(point.x == j && point.y == i) attron(COLOR_PAIR(SELECTED_DEKO));
	else attron(COLOR_PAIR(DEFAULT_CLR));
	// 文字表示
	mvprintw(BY+i, BX+j*2, "？");
	break;
      }
    }
  }
}

int disposal_gameover(BOARD *board, POINT *point, int game_flag) {
  int flag = 0;
  
  switch(game_flag) {
  case GAME_OVER:
    open_all(board);
    print_board(*board, *point);
    attron(COLOR_PAIR(DEFAULT_CLR));
    mvprintw(BY*2+board->y, BX, "Game over... Do you play new game?(y/n)");
    break;
  case GAME_CLEAR:
    print_board(*board, *point);
    attron(COLOR_PAIR(DEFAULT_CLR));
    mvprintw(BY*2+board->y, BX, "Game clear! Do you play new game?(y/n)");
    break;
  }
 
  return game_end_key(board);
}

// ボードに使っているメモリの開放
void delete_board(BOARD *board) {
  int i;

  for(i = 0; i < board->y; i++) {
    free(board->data[i]);
    free(board->board[i]);
  }
  free(board->data);
  free(board->board);
}

void end(BOARD *board) {
  delete_board(board);
  endwin();
}
