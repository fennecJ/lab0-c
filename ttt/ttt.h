#ifndef _TTT_H_
#define _TTT_H_
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "game.h"
#ifdef USE_RL
#include "reinforcement_learning.h"
#elif defined(USE_MCTS)
#include "mcts.h"
#else
#include "agents/negamax.h"
#endif

int ttt();

#endif  // _TTT_H_