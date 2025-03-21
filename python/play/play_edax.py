from subprocess import Popen, PIPE
from threading  import Thread
from queue import Queue, Empty
import atexit
from time import time, sleep
import random
import numpy as np
import pandas as pd
s = int(1000000000*(time()%1))
print(s)
random.seed(s) # truly random start - need this to run parallel games

print_scr  = False
write_logs = False
game_time = 10 # seconds **************************************

engine1_name = "./rev"
engine2_name = "./lEdax -q -l 50 -ponder off -n 1 -h 22 -game-time " + str(game_time)



agent_process1 = None
agent_process2 = None
t1 = None
q1 = None
t2 = None
q2 = None

def cleanup_process1():
    global agent_process1
    if agent_process1 is not None:
        agent_process1.kill()
        
def cleanup_process2():
    global agent_process2
    if agent_process2 is not None:
        agent_process2.kill()
        
def enqueue_output(out, queue):
    for line in iter(out.readline, b''):
        queue.put(line)
    out.close()

# make move function *****************************************************
def make_move(move, b, player):
    b1 = b.copy()
    legit = 0
    if move > 63:
        return b, legit
    x = move % 8
    y = move // 8
    x0, y0 = x, y
    for dx in [-1, 0, 1]:
        for dy in [-1, 0, 1]:
            if dx == 0 and dy == 0:
                continue
            x, y = x0, y0
            x += dx
            y += dy
            if x >= 0 and x < 8 and y >= 0 and y < 8 and b1[y, x] == -player:
                x += dx
                y += dy
                if not (x >= 0 and x < 8 and y >= 0 and y < 8):
                    continue
                while b1[y, x] == -player:
                    x += dx
                    y += dy
                    if not (x >= 0 and x < 8 and y >= 0 and y < 8):
                        break
                if not (x >= 0 and x < 8 and y >= 0 and y < 8):
                    continue
                if b1[y, x] == player:
                    legit = 1
                    while True:
                        x -= dx
                        y -= dy
                        b1[y, x] = player # change
                        if x == x0 and y == y0:
                            break
    return b1, legit

# get moves function *****************************************************
def get_moves(b, player):
    moves = []
    for x in range(8):
        for y in range(8):
            if b[y,x] == 0:
                b0 = b.copy() # save
                _, legit = make_move(y * 8 + x, b0, player)
                if legit:
                    moves.append(y * 8 + x)
    return moves

# create random starting board
def starting_board(n): # get random starting board with n pieces on it
    while True: # loop until it comes out right
        b = np.zeros([8,8], dtype=np.int32)
        b[3,3] = b[4,4] = 1
        b[3,4] = b[4,3] = -1
        player = 1
        # loop over random moves
        for i in range(n-4):
            # get all moves
            moves = get_moves(b, player) # what if there are no moves?
            if len(moves) == 0: # pass; skip this one and try again
                break
            # select one
            move = random.choice(moves)
            # make it
            b, legit = make_move(move, b, player)
            player = -player
            moves = get_moves(b, player) # what if there are no moves?
            if len(moves) == 0: # pass; skip this one and try again
                b = np.zeros([8,8], dtype=np.int32)
                break
        if np.abs(b).sum() == n:
            return b, player
    
def get_fen(b, player):
    f = []
    for i in range(64):
        if b.ravel()[i] == 0:
            f.append('-')
        elif b.ravel()[i] == 1:
            f.append('O')
        else:
            f.append('*')
    f.append(' ')
    if player == 1:
        f.append('O')
    else:
        f.append('*')
    f = ''.join(f)
    return f

def board_from_fen(f, b):
    b = b.ravel()
    if f[-1] == 'O':
        player = 1
    else:
        player = -1
    for i in range(64):
        if f[i] == '-':
            b[i] = 0
        elif f[i] == 'O':
            b[i] = 1
        else:
            b[i] = -1
    b = b.reshape(8,8)
    return b, player

# start the engines
# 1
agent_process1 = Popen([engine1_name], stdin=PIPE, stdout=PIPE, stderr=PIPE) # 1
atexit.register(cleanup_process1) # 1

q1 = Queue()
t1 = Thread(target=enqueue_output, args=(agent_process1.stderr, q1)) # 1
t1.daemon = True # 1
t1.start() # 1

# 2
agent_process2 = Popen(engine2_name.split(), stdin=PIPE, stdout=PIPE, stderr=PIPE) # 2
atexit.register(cleanup_process2) # 2

q2 = Queue()
t2 = Thread(target=enqueue_output, args=(agent_process2.stderr, q2)) # 2
t2.daemon = True # 2
t2.start() # 2

sleep(2) # wait N seconds - for engines to initialize


# init log file(s)
if write_logs:
    with open('game_log.txt', 'w') as f:
        f.write('starting the run...\n')
    with open('game_log_detailed.txt', 'w') as f:
        f.write('starting the run...\n')



# loop over game sets
scores = np.array([0,0,0]) # w/d/l for player 1 (new)
rr = [] # result list of game pairs
tt = [] # list of time left
time_start = time()
for game in range(150): # at 10 sec, 100 game pairs in 1 hour
    # get starting board
    b2, player2 = starting_board(14) # starting number of pieces

    fen = get_fen(b2, player2)
    s = np.minimum(.99, np.maximum(.01, (scores[0] + scores[1] / 2) / np.maximum(1, scores.sum()))) # score
    print(scores, np.round(-400 * np.log10(1 / s -  1), 0).astype(np.int32), fen) # print current progress ***********************************************
    if write_logs:
        with open('game_log_detailed.txt', 'a') as f:
            f.write('game,' + str(game) + ',fen,' + fen + '\n')


    # play two games, with board flipped after the first one
    symm_scores = np.array([0,0]) # 2 games. Scores are always for player 1 - new
    for symm_game in [0, 1]:
        b = b2.copy()
        player = player2
        if symm_game == 1: # flip the board
            b = b * -1
            player = player * -1

        # play 1 game
        # init the engines
        # me
        c = "newgame\n"
        agent_process1.stdin.write(c.encode())
        agent_process1.stdin.flush()

        # edax
        c = "xboard\n"
        agent_process2.stdin.write(c.encode())
        agent_process2.stdin.flush()
        if print_scr:
            print('tell edax:', c[:-1]) # print ******************
        
        c = "new\n"
        agent_process2.stdin.write(c.encode())
        agent_process2.stdin.flush()
        if print_scr:
            print('tell edax:', c[:-1]) # print ******************

        fen = get_fen(b, player)
        c = "setboard " + fen + "\n"
        agent_process2.stdin.write(c.encode())
        agent_process2.stdin.flush()
        if print_scr:
            print('tell edax:', c[:-1]) # print ******************
        
        if player == -1:
            c = "go\n"
            agent_process2.stdin.write(c.encode())
            agent_process2.stdin.flush()
            if print_scr:
                print('tell edax:', c[:-1]) # print ******************

        if print_scr:
            print('started new game***************************************************************') # print ******************

        # time - N sec per game ******************************************************
        time1 = 1000 * game_time
        time2 = 1000 * game_time

        # loop over moves
        for i in range(np.abs(b).sum(), 65):
            # check if pass
            moves = get_moves(b, player)
            if len(moves) == 0:
                if np.abs(b).sum() < 64:
                    if write_logs:
                        with open('game_log_detailed.txt', 'a') as f:
                            f.write('pass\n')
                    if print_scr:
                        print('pass***************************************************************') # print ******************
                    if player == 1: # i pass, tell edax
                        agent_process2.stdin.write('ps\n'.encode()) # **********************************************************
                        agent_process2.stdin.flush()
                        if print_scr:
                            print('tell edax: i pass *********************************') # ***************************************
                    else: # edax passes - still need to process its response
                        if print_scr:
                            print('2 waiting for ', ag) # print ************************************************************************
                        response = (agent_process2.stdout.readline()).decode()
                        if print_scr:
                            print(response[:-1]) # print ************************************************************************
    
                player = -player
                moves = get_moves(b, player)
                if len(moves) == 0:
                    # end of game
                    # get game result from edax
                    #response = (agent_process2.stdout.readline()).decode()
                    #if print_scr:
                    #    print('get game result from edax:', response[:-1]) # print ************************************************************************
                    s1 = (b==1).sum()
                    s2 = (b==-1).sum()
                    r = 0
                    if s1 > s2:
                        r = 64 - 2 * s2
                    elif s1 < s2:
                        r = 2 * s1 - 64
                    print('game', game, 'symm_game', symm_game, 'score', r, 'time left', time1, time2, '************************')
                    tt.append(time1)
                    tt.append(time2)
                    if write_logs:
                        with open('game_log_detailed.txt', 'a') as f:
                            f.write('score,' + str(r) + ',time left,' + str(time1) + ',' + str(time2) + '*************************\n')
                        # log game result
                        with open('game_log.txt', 'a') as f:
                            f.write('game,'+str(game)+',symm,' + str(symm_game) + ',score,' + str(r) + '*********************\n')
                    # update score of symm set. Always from POV of player 1 (new)
                    symm_scores[symm_game] = r
                    # update score of game set. Always from POV of player 1 (new)
                    if symm_game == 1:
                        r = symm_scores.sum()
                        rr.append(r)
                        if r == 0: # draw
                            scores[1] += 1
                        elif r > 0: # win
                            scores[0] += 1
                        else: # loss
                            scores[2] += 1
                    break

            # get fen
            fen = get_fen(b, player)
            t1 = time()
            
            # start the calc **********************************************
            if player == 1: # me
                ag = agent_process1
                
                c = "position fen " + fen + "\n"
                ag.stdin.write(c.encode())
                ag.stdin.flush()
                    
                c2 = "go time " + str(time1) + "\n"
                ag.stdin.write(c2.encode())
                ag.stdin.flush()
                if print_scr:
                    print(c[:-1]) # print ************************************************************************
                    print(c2[:-1]) # print ************************************************************************
                if write_logs:
                     with open('game_log_detailed.txt', 'a') as f:
                        f.write(c)
                        f.write(c2)
            else:
                ag = agent_process2


                
            # get responce
            while True:
                if print_scr:
                    print('1waiting for ', ag) # print ************************************************************************
                response = (ag.stdout.readline()).decode()
                if response.split()[0] != 'info':
                    if print_scr:
                        print(response[:-1]) # print ************************************************************************
                    if write_logs:
                         with open('game_log_detailed.txt', 'a') as f:
                            f.write(response)
                tokens = response.split()
                if len(tokens) >= 2 and (tokens[0] == "bestmove" or tokens[0] == "move"):
                    break
            # process the responce
            if player == 1: # me
                move = int(tokens[1])

                # pass my move to deax, it starts thinking immediately
                coord = ['a','b','c','d','e','f','g','h'][move%8] + str(move//8 + 1)
                c = coord + '\n'
                agent_process2.stdin.write(c.encode())
                agent_process2.stdin.flush()
                if print_scr:
                    print('tell edax:', c) # print ************************************************************************
                if write_logs:
                    with open('game_log_detailed.txt', 'a') as f:
                        f.write(c)
                    
            else: # edax: move P@b5
                move = tokens[1] # P@b5
                move = move[2:]  # b5
                di = {'A':0, 'B':1, 'C':2, 'D':3, 'E':4, 'F':5, 'G':6, 'H':7}
                move = di[move[0].upper()] + 8 * (int(move[1]) - 1)


            # adjust time
            t2 = time()
            if player == 1:
                time1 = int(time1 + 1000 * (t1 - t2))
            else:
                time2 = int(time2 + 1000 * (t1 - t2))
            
            
            # make the move
            if print_scr:
                print(i, move, player, time1, time2, fen) # print **************************************************
            if write_logs:
                with open('game_log_detailed.txt', 'a') as f:
                    msg = str(i) + ', ' + str(move) + ', ' + str(player) + ', ' + str(time1) + ', ' + str(time2) + ', ' + fen + '\n'
                    f.write(msg)
            b, legit = make_move(move, b, player)
            if legit == 0:
                print('illegal move!')
                stop
            player = -player
        
print('finished', game+1, 'games. Time elapsed', int(time()-time_start), 'sec')
s = (scores[0] + scores[1] / 2) / scores.sum() # score
print(scores, 'win rate', np.round(s, 3), 'ELO', np.round(-400 * np.log10(1 / s-  1), 0).astype(np.int32))
print('average time per engine', np.round(game_time - np.array(tt).reshape(-1, 2).mean(0) / 1000, 1))
print(pd.Series(rr).value_counts().sort_index())
