#include "board.h"

#define HORRIBLE -32000
#define GREAT 32000

static int boards;
static jmp_buf env;
static double total_time;
static int searching_to_end;
static int bx, by, bs;
static int limit;
static int IRQ;

static void bcpy(BOARD b1, BOARD b2);
static int rsearch(BOARD board, int is_white, int depth, int lvl);
static int maxi(BOARD board, int is_white, int depth, int a, int b);
static int mini(BOARD board, int is_white, int depth, int a, int b);

static void timeout(int signum) { IRQ = 1; }

static void print_with_commas(int n) {
  if (n < 1000) {
    printf("%d", n);
  } else {
    print_with_commas(n / 1000);
    printf(",%03d", n % 1000);
  }
}

static void print_duration(double duration) {
  int seconds = (int)duration;
  int minutes = seconds / 60;
  int millis = (duration - seconds) * 1000;
  seconds %= 60;
  printf("%d:%02d.%03d", minutes, seconds, millis);
}

int search(BOARD board, int is_white, int *bestx, int *besty) {
  int i, start, lvl, moves;

  signal(SIGALRM, timeout);

  // This is some rudimentry time management.  It turns out
  // that it's not very interesting to spent a lot of time
  // thinking about the early moves.  First they don't matter
  // much but second, in the old days the code easily got
  // to 6 ply and had no hope of finishing 8 with even
  // a more generous budget. So we deem it not worth it
  // and save the seconds for later in the game when it
  // will matter more.  We're also hoarding a big budget
  // so that when we hit turn = ENDGAME we can search
  // to the end without losing because we run out of time
  // In 1987 that was about 12 ply from the end.  So
  // turn 52.  Now we can do it at Turn 44 -- 20 ply.
  // We also do it faster now.  It was good 10 minutes in 1987.
  // On a modern CPU we can do the endgame search
  // in about 90s now.
  if (turn < 15)
    limit = 2;
  else if (turn < 30)
    limit = 4;
  else
    limit = 10;

  boards = 0;
  searching_to_end = 0;
  IRQ = 0;

  // arm the clock if we need to
  clock_t start_time = clock();
  if (turn <= ENDGAME) {
    alarm(limit);
    start = 2;
  } else {
    printf("Seeking to end of board\n");
    fflush(stdout);
    start = 64 - turn;
    if (start % 2)
      start++;
  }

  // when the code sees IRQ = 1 it will longjmp
  if (setjmp(env)) {
    // back from longjmp

    // save the running best into our output
    // compute the duration and make a little report
    *bestx = bx;
    *besty = by;
    clock_t end_time = clock();
    double duration = (end_time - start_time + 0.0) / CLOCKS_PER_SEC;

    // print some stats
    if (duration == 0.0) {
      duration = 0.001;
    }
    total_time += duration;

    printf("\nEvaluated ");
    print_with_commas(boards);
    printf(" boards in ");
    print_duration(duration);
    printf(" (");
    print_with_commas((int)(boards / duration));
    printf(" boards/sec).  Total time used: ");
    print_duration(total_time);
    printf("\n");
    fflush(stdout);

    return bs;
  }

  // if we have no moves... we have to pass
  if (!(moves = valid(board, is_white, start))) {
    bx = -1;
    by = -1;
    bs = HORRIBLE;
    goto no_moves;
  }

  // if we have some moves then seed the initial move list
  // normally we do these in order of goodness (best first)
  // but at this point we know nothing.  It all starts at
  // score == 0.
  reset_scored_moves(0);
  while (pop_move(bestx, besty, start)) {
    insert_scored_move(*bestx, *besty, 0, 0);
  }

  // we only resort the moves in best order at the top level
  // in principle we could do this at every level in the tree
  // but it wasn't deemed worth it.  We seem to get good value
  // out of our alpha/beta pruning just ordering the top
  // level and there's the cost of all that sorting.  It might
  // be worth it to try to order at every level some day.  For
  // now we just have the moves from the last result and
  // the ones we're working on, so we just need two levels of
  // sorted moves.
  lvl = 0;
  for (i = start; i < 65; i += 2) {
    printf("%2d: ", i);
    fflush(stdout);
    rsearch(board, is_white, i, lvl);
    if (i + 2 > 64 - turn)
      break;
    lvl = !lvl;
  }

  // disarm the clock
no_moves:
  alarm(0);
  longjmp(env, 1);
}

// This sets up the real search, one real search
static int rsearch(BOARD board, int is_white, int depth, int lvl) {
  int x, y, sc;
  BOARD brd;
  int moves;

  reset_scored_moves(!lvl);
  moves = 0;
  searching_to_end = 0;

  // this is a bit of defensive coding, this can't happen
  // the `search` above us already checked.  We definitely
  // have a move to consider.
  if (!remove_scored_move(&x, &y, lvl))
    return 0;

  // try the first move, this will become our working best
  // bx and by.  The score is in bs.
  printf("%c%c=", x + 'a', '8' - y);
  fflush(stdout);
  bcpy(brd, board);
  flip(brd, is_white, x, y);

  // we get the score by looking at the worst outcome
  // for us, the mini in minimax.  The alpha beta
  // pruning starts with no cap -- HORRIBLE, GREAT
  // we're building up the moves in the new sorted
  // order for the next deeper pass.
  bs = mini(brd, !is_white, depth - 1, HORRIBLE, GREAT);
  bx = x;
  by = y;
  printf("%d  ", bs - ((turn > ENDGAME) ? 0 : SCORE_BIAS));
  fflush(stdout);
  insert_scored_move(x, y, bs, !lvl);

  // grab the next scored move and start considering it
  // we print them as we go.
  while (remove_scored_move(&x, &y, lvl)) {
    printf("%c%c", x + 'a', '8' - y);
    fflush(stdout);
    bcpy(brd, board);
    flip(brd, is_white, x, y);
    sc = mini(brd, !is_white, depth - 1, bs, GREAT);

    // re-insert this move on the other sorted list
    // print xx=nn if this is a new best score
    // print xx<nn if it isn't.  We don't know
    // the exact score if it's lower because it
    // might have been pruned.  All we know for sure
    // is that it can't be better than what we have.
    insert_scored_move(x, y, sc, !lvl);
    if (sc > bs) {
      putchar('=');
      bx = x;
      by = y;
      bs = sc;
    } else
      putchar('<');

    printf("%d  ", sc - ((turn > ENDGAME) ? 0 : SCORE_BIAS));
    fflush(stdout);
  }
  printf("\n");
  fflush(stdout);
  return bs;
}

// this is the mini part of minimax.  We're going to pick
// the worst score here (subject to alpha/beta pruning)
static int mini(BOARD board, int is_white, int depth, int a, int b) {
  if (IRQ)
    longjmp(env, 1);
  boards++;

  // if we get to the bottom of the recursion, use the scoring function
  if (!depth)
    return score(board, is_white);
  else {
    BOARD brd;
    int x, y, sc;

    // clear any moves in this stack and compute new valid moves
    reset_move_stack(depth);

    int found_anything = valid(board, is_white, depth);
    if (!found_anything) {
      // this means there are no moves, we have to pass

      if (turn > ENDGAME && !searching_to_end) {
        // we're searching to the end, we had to pass
        // so the board didn't fill in. That means the game
        // might go one turn longer.  Continue the search
        // with one extra level so that we get to the end still.
        // This is imperfect because there could be more passes
        // along the way... oh well.  Room for improvement.
        searching_to_end = 1;
        sc = maxi(board, !is_white, depth + 1, a, b);
      } else {
        // normal case, just keep scoring from here
        // we don't give ourselves a penalty for passing
        sc = maxi(board, !is_white, depth - 1, a, b);
      }
      return sc;
    }
    searching_to_end = 0;

    // now process the moves, this is the mini pass
    // so we take the worst of the best
    while (pop_move(&x, &y, depth)) {
      bcpy(brd, board);
      flip(brd, is_white, x, y);
      sc = maxi(brd, !is_white, depth - 1, a, b);
      // in this pass we're minning the maxes
      if (sc < b) {
        // so this says we found a new min
        b = sc;

        // if this happens then that means this min will
        // be even worse than a previous known min
        // from the max pass above us
        // that means the max pass will not use this
        // score for sure it's too low so just stop
        // computing, we can prune here.
        if (b <= a) {
          return b;
        }
      }
    }

    return b;
  }
}

// this is the maxi part of minimax.  We're going to pick
// the best score here (subject to alpha/beta pruning)
static int maxi(BOARD board, int is_white, int depth, int a, int b) {
  if (IRQ)
    longjmp(env, 1);
  boards++;

  // if we get to the bottom of the recursion, use the scoring function
  if (!depth)
    return score(board, is_white);
  else {
    BOARD brd;
    int x, y, sc;

    // clear any moves in this stack and compute new valid moves
    reset_move_stack(depth);

    int found_anything = valid(board, is_white, depth);
    if (!found_anything) {
      // this means there are no moves, we have to pass

      if (turn > ENDGAME && !searching_to_end) {
        // we're searching to the end, we had to pass
        // so the board didn't fill in. That means the game
        // might go one turn longer.  Continue the search
        // with one extra level so that we get to the end still.
        // This is imperfect because there could be more passes
        // along the way... oh well.  Room for improvement.
        searching_to_end = 1;
        sc = mini(board, !is_white, depth + 1, a, b);
      } else {
        // normal case, just keep scoring from here
        // we don't give ourselves a penalty for passing
        sc = mini(board, !is_white, depth - 1, a, b);
      }
      return sc;
    }
    searching_to_end = 0;

    // now process the moves, this is the maxi pass
    // so we take the best of the worst
    while (pop_move(&x, &y, depth)) {
      bcpy(brd, board);
      flip(brd, is_white, x, y);
      sc = mini(brd, !is_white, depth - 1, a, b);
      // in this pass we're maxxing the mins
      if (sc > a) {
        // so this says we found a new max
        a = sc;

        // if this happens then that means this max will
        // be even better than a previous known max
        // from the min pass above us
        // that means the min pass will not use this
        // score for sure it's too high so just stop
        // computing, we can prune here.
        if (a >= b)
          return a;
      }
    }
    return a;
  }
}

static void bcpy(BOARD b1, BOARD b2) { memcpy(b1, b2, sizeof(BOARD)); }

void move(BOARD board, int is_white, int x, int y) {
  board[is_white][y] |= (1 << x);
  flip(board, is_white, x, y);
  display(board);
}

static void show(int depth, int score) {
  int i;

  for (i = 0; i < depth; i++)
    putchar('\t');

  printf("%d\n", score);
  fflush(stdout);
}
