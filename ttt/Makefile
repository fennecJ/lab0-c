PROG = ttt
CFLAGS := -Wall -Wextra -std=c11
CFLAGS += -I. -MMD
LDFLAGS :=
TRAIN = train
RL = rl
MCTS = mcts
RL_CFLAGS := $(CFLAGS) -D USE_RL
MCTS_CFLAGS := $(CFLAGS) -D USE_MCTS
MCTS_LDFLAGS := $(LDFLAGS) -lm
ELO = elo
ELO_CFLAGS := $(CFLAGS)
ELO_LDFLAGS := $(LDFLAGS) -lm

all: $(PROG)

OBJS := \
	ttt.o \
	game.o \
	mt19937-64.o \
	zobrist.o \
	agents/negamax.o
deps := $(OBJS:%.o=.%.o.d)
deps += $(RL).d
deps += $(TRAIN).d
deps += $(MCTS).d
deps += $(ELO).d

$(PROG): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(RL): ttt.o agents/reinforcement_learning.c game.c
	$(CC) -o $@ $^ $(RL_CFLAGS)

$(TRAIN): $(TRAIN).c agents/reinforcement_learning.c game.c
	$(CC) $(CFLAGS) -o $@ $^

$(MCTS): ttt.o agents/mcts.c game.c
	$(CC) -o $@ $^ $(MCTS_CFLAGS) $(MCTS_LDFLAGS)

$(ELO): $(ELO).c agents/negamax.c agents/mcts.c agents/reinforcement_learning.c game.c mt19937-64.c zobrist.c
	$(CC) -o $@ $^ $(ELO_CFLAGS) $(ELO_LDFLAGS)

clean:
	-$(RM) $(PROG) $(OBJS) $(deps) $(TRAIN) $(RL) $(MCTS) $(ELO)
	-$(RM) *.bin
