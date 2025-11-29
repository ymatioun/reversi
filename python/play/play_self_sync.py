from subprocess import Popen, PIPE
from threading  import Thread
from queue import Queue
import atexit
from time import time, sleep
import random
import numpy as np
import pandas as pd
random.seed(int(1000000000*(time()%1))) # truly random start - need this to run parallel games

write_logs = False # write the two game log files???????????
games = int(8 * 164) # at 10 sec, 164 game pairs in 1 hour [both engined are running, so only start 8 processes!]

game_time = 10 # seconds **************************************
n0 = 14


engine1_name = './rev_old'
engine2_name = './rev_new'


min_time = 250 # give engines at least this much time for each move; need this to avoid crashes!
agent_process1 = agent_process2 = t1 = q1 = t2 = q2 = None

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
            if len(moves) == 0: # pass
                break # skip this one and try again
            # select one
            move = random.choice(moves)
            # make it
            b, legit = make_move(move, b, player)
            player = -player
            moves = get_moves(b, player) # what if there are no moves?
            if len(moves) == 0: # pass; skip this one and try again
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


def calc_score(scores):
    s = np.minimum(.99, np.maximum(.01, (scores[0] + scores[1] / 2) / max(.1, scores.sum()))) # score
    e = -400 * np.log10(1 / s - 1) # ELO
    s2 = scores / max(.1, scores.sum())
    es = np.sqrt((s2[0] + s2[2] - (s2[0] - s2[2])**2) / max(.1, scores.sum())) # std of score
    es *= 174 / s / (1 - s) # std of ELO
    return np.round(e, 0).astype(np.int32), np.round(es, 0).astype(np.int32)

# start the engines *********************************************************
# 1
agent_process1 = Popen([engine1_name], stdin=PIPE, stdout=PIPE, stderr=PIPE) # 1
atexit.register(cleanup_process1) # 1
q = Queue()
t1 = Thread(target=enqueue_output, args=(agent_process1.stderr, q1)) # 1
t1.daemon = True # 1
t1.start() # 1

# 2
agent_process2 = Popen([engine2_name], stdin=PIPE, stdout=PIPE, stderr=PIPE) # 2
atexit.register(cleanup_process2) # 2
q = Queue()
t2 = Thread(target=enqueue_output, args=(agent_process2.stderr, q2)) # 2
t2.daemon = True # 2
t2.start() # 2

sleep(1) # wait N seconds - for engines to initialize


# init log file(s)
with open('total_scores.csv', 'w') as f:
    f.write('c_win,c_draw,c_loss,ELO,stdELO\n')
if write_logs:
    with open('game_log.txt', 'w') as f:
        f.write('starting the run...\n')
    with open('game_log_detailed.txt', 'w') as f:
        f.write('starting the run...\n')



# loop over game sets
scores = np.array([0, 0, 0]) # w / d / l for player 1 (new)
rr = [] # result list of game pairs
time_start = time()
for game in range(games):
    b2, player2 = starting_board(n0) # get starting board

    e, es = calc_score(scores)
    print(game, '/', games, scores, 'ELO', e, '+/-', es, int(time()-time_start), 'sec') # print current progress ***********************************************
    if write_logs:
        with open('game_log_detailed.txt', 'a') as f:
            f.write('game,' + str(game) + ',fen,' + get_fen(b2, player2) + '\n')

    # play two games (_a, _b) at the same time
    b_a = b2.copy()
    b_b = b2.copy()
    player_a = player_b = player2
    
    # play 1 sync game
    # init the engines
    c = 'newgame\n'
    agent_process1.stdin.write(c.encode())
    agent_process1.stdin.flush()
    agent_process2.stdin.write(c.encode())
    agent_process2.stdin.flush()

    # time - N sec per game ******************************************************
    time1_a = time2_a = time1_b = time2_b = 1000 * game_time
        
    # loop over moves
    symm_scores = np.array([99, 99]) # 2 games. Init to 99 -> 'games in progress'
    for i in range(np.abs(b2).sum(), 75):
        moves_a, moves_b = get_moves(b_a, player_a), get_moves(b_b, player_b)
            
        # check if endgame - 'a'
        if symm_scores[0] == 99 and len(moves_a) == 0 and len(get_moves(b_a, -player_a)) == 0:
            s1, s2, r = (b_a==1).sum(), (b_a==-1).sum(), 0
            if s1 > s2:
                r = 64 - 2 * s2
            elif s1 < s2:
                r = 2 * s1 - 64
            if write_logs:
                print('a score', r, 'time left', time1_a, time2_a, '*********************')
                with open('game_log_detailed.txt', 'a') as f:
                    f.write('game,' + str(game) + ',a,score,' + str(r) + ',time left,' + str(time1_a) + ',' + str(time2_a) + '**********************\n')
                with open('game_log.txt', 'a') as f:
                    f.write('game,' + str(game) + ',a,score,' + str(r) + ',time left,' + str(time1_a) + ',' + str(time2_a) + '**********************\n')
            # update score of symm set
            symm_scores[0] = r # 0 -> a

        # check if endgame - 'b'
        if symm_scores[1] == 99 and len(moves_b) == 0 and len(get_moves(b_b, -player_b)) == 0:
            s1, s2, r = (b_b==1).sum(), (b_b==-1).sum(), 0
            if s1 > s2:
                r = 64 - 2 * s2
            elif s1 < s2:
                r = 2 * s1 - 64
            if write_logs:
                print('b score', r, 'time left', time1_b, time2_b, '*********************')
                with open('game_log_detailed.txt', 'a') as f:
                    f.write('game,' + str(game) + ',b,score,' + str(r) + ',time left,' + str(time1_b) + ',' + str(time2_b) + '**********************\n')
                with open('game_log.txt', 'a') as f:
                    f.write('game,' + str(game) + ',b,score,' + str(r) + ',time left,' + str(time1_b) + ',' + str(time2_b) + '**********************\n')
            # update score of symm set
            symm_scores[1] = r # 1 -> b

        # get score of game - both sync parts *************************
        if symm_scores.max() < 99: # both games ended
            r = symm_scores[0] - symm_scores[1] # scores are 'old', 'new'; so [0] - [1] is score for old
            rr.append(r)
            #if abs(r) >= 16: # stop on first bad game
            #    stop
            if r == 0: # draw
                scores[1] += 1
            elif r > 0: # win
                scores[0] += 1
            else: # loss
                scores[2] += 1
            if write_logs:
                print('game', game, '/', games, 'score', r, symm_scores, '*********************')
                with open('game_log_detailed.txt', 'a') as f:
                    f.write('game,' + str(game) + ',total,score,' + str(r) + ',' + str(scores) + '**********************\n')
                with open('game_log.txt', 'a') as f:
                    f.write('game,' + str(game) + ',total,score,' + str(r) + ',' + str(scores) + '**********************\n')
            break

        fen_a, fen_b = get_fen(b_a, player_a), get_fen(b_b, player_b) # get fens
        t1 = time() # time when calc starts
            
        # start the calc **********************************************
        # game a: player_a = 1 means agent1 -> 'old'
        ag_a = [agent_process2, agent_process1][max(player_a, 0)]
        if len(moves_a) > 0 : # skip if pass
            c = 'position fen ' + fen_a + '\n'
            ag_a.stdin.write(c.encode())
            ag_a.stdin.flush()
            c = 'go time ' + str(max(min_time, [time2_a, time1_a][max(player_a, 0)])) + '\n'
            ag_a.stdin.write(c.encode())
            ag_a.stdin.flush()
        # game b: player_b = 1 means agent2 -> 'new'
        ag_b = [agent_process1, agent_process2][max(player_b, 0)] # for game b agents are exchanged!
        if ag_a == ag_b:
            print('trying to run the same engine twice!!!')
            stop
        if len(moves_b) > 0 : # skip if pass
            c = 'position fen ' + fen_b + '\n'
            ag_b.stdin.write(c.encode())
            ag_b.stdin.flush()
            c = 'go time ' + str(max(min_time, [time1_b, time2_b][max(player_b, 0)])) + '\n'
            ag_b.stdin.write(c.encode())
            ag_b.stdin.flush()
      
        # get responses ******************************************
        # game a
        if len(moves_a) > 0 : # skip if pass
            while True:
                response = (ag_a.stdout.readline()).decode()
                #if ag_a == agent_process2:
                #    print(game, '***a new ' + response) # print all new ***********************************
                #if ag_a == agent_process1:
                #    print(game, '***a old ' + response) # print all new ***********************************
                tokens = response.split()
                if tokens[0] == 'bestmove': # bestmove xx score[0] yy time zz depth aa
                    break
            if write_logs:
                print('a ' + str([time2_a, time1_a][max(player_a, 0)]) + ' ' + ['r2 ', 'r1 '][max(player_a, 0)] + str(i) + ' ' + response[:-1]) # print **************************************************************
                with open('game_log_detailed.txt', 'a') as f:
                    f.write('a,' + str(game) + ',' + str([time2_a, time1_a][max(player_a, 0)]) + ' ' + ['r2,', 'r1,'][max(player_a, 0)] + str(i) + ' ' + response)
            # adjust time - after the move
            if player_a == 1:
                time1_a = time1_a - int(tokens[5])
            else:
                time2_a = time2_a - int(tokens[5])
            # make the move
            b_a, legit = make_move(int(tokens[1]), b_a, player_a)
            if legit == 0:
                print('a: illegal move!')
                stop
        else: # pass
            if write_logs:
                print('a ' + str([time2_a, time1_a][max(player_a, 0)]) + ' ' + ['r2 ', 'r1 '][max(player_a, 0)] + str(i) + ' pass') # print **************************************************************
                with open('game_log_detailed.txt', 'a') as f:
                    f.write('a,' + str(game) + ',' + str([time2_a, time1_a][max(player_a, 0)]) + ' ' + ['r2,', 'r1,'][max(player_a, 0)] + str(i) + ',pass\n')
        player_a = -player_a # switch players even if pass
        # game b: for game b agents are exchanged!
        if len(moves_b) > 0 : # skip if pass
            while True:
                response = (ag_b.stdout.readline()).decode()
                #if ag_b == agent_process2:
                #    print(game, '***b new ' + response) # print all new ***********************************
                #if ag_b == agent_process1:
                #    print(game, '***b old ' + response) # print all new ***********************************
                tokens = response.split()
                if tokens[0] == 'bestmove': # bestmove xx score[0] yy time zz depth aa
                    break
            if write_logs:
                print('b ' + str([time2_b, time1_b][max(-player_b, 0)]) + ' ' + ['r2 ', 'r1 '][max(-player_b, 0)] + str(i) + ' ' + response[:-1]) # print **************************************************************
                with open('game_log_detailed.txt', 'a') as f:
                    f.write('b,' + str(game) + ',' + str([time2_b, time1_b][max(-player_b, 0)]) + ' ' + ['r2,', 'r1,'][max(-player_b, 0)] + str(i) + ' ' + response)
            # adjust time - after the move
            if player_b == -1: # for game b agents are exchanged!
                time1_b = time1_b - int(tokens[5])
            else:
                time2_b = time2_b - int(tokens[5])
            # make the move
            b_b, legit = make_move(int(tokens[1]), b_b, player_b)
            if legit == 0:
                print('b: illegal move!')
                stop
        else: # pass: for game b agents are exchanged!
            if write_logs:
                print('b ' + str([time2_b, time1_b][max(-player_b, 0)]) + ' ' + ['r2 ', 'r1 '][max(-player_b, 0)] + str(i) + ' pass') # print **************************************************************
                with open('game_log_detailed.txt', 'a') as f:
                    f.write('b,' + str(game) + ',' + str([time2_b, time1_b][max(-player_b, 0)]) + ' ' + ['r2,', 'r1,'][max(-player_b, 0)] + str(i) + ',pass\n')
        player_b = -player_b # switch players even if pass

print('finished', games, 'games. Time elapsed', int(time()-time_start), 'sec.', int(.5 + games * 3600 / (time()-time_start)), 'games per hour')
e, es = calc_score(scores)
print('results for  current engine:', scores, 'ELO', e, '+/-', es)
print(pd.Series(rr).value_counts().sort_index())
with open('total_scores.csv', 'a') as f:
    f.write(str(scores[0]) + ',' +  str(scores[1]) + ',' + str(scores[2]) + ',' + str(np.round(e, 3)) + ',' + str(es) + '\n')

# non-sync:         other changes, rerun: -33+-7 - assume OK
# sync: -34+-5 - pretty much the same.
# faster endgame: -34+-8
# levelized timing v1:          -16+-11 - worse, undo!
# odd depth: -38+8 - slightly better? 
# eval hash, alp0: -46+-7 - better!
# odd/even depth, based on n: -52+-4 - better! **************************
# even/odd - the other way: -41+-7 -worse, undo
# use TT score as window center: -50+-9 - a wash. Keep.
# 2 TTs/hists for sync games: -66+-7 - better! **************************
# 7 threads,  same timing:                      -179+-8; impr(7)=113=.58 of max
# 15 threads, same timing:                      -224+-12; impr(15)=158=.57 of max; 15 vs 7 = +45
# faster bitmob, always inline: -78+-10 - better! **************************
# 1.5x time for first search for synch games: -75+-5 - not better, undo
# CRC hash, AVX2 flip: -80+-7 - better! [+2] **************************
# timing v2 - narrow the range: -70+-8 = -10, worse, undo!
# timing v3 - incr initial time by 1.1: -86 +-7 - better! [+6] **************************
# no MPC when d0 - d <= 1:-82+-9 - a wash, undo
# MPC/eval for d=[1:2]: -77+-9 - worse, undo
# MPC/eval for d=[2:5]: -72 - worse, undo
# do not use move of fail low: seems to reduce bad score tail!!!!!!!!!!!!!!!!
#           ELO = -87+-6 - better! [+1] **************************
# 15 threads, same timing:                      -261+-19; impr(15) = 174 = .65 of max
# open window:-58+-6 - worse, undo
# window = 3: -94+-8 - better! [+7 from last, +36 from open window] **************************
# window = 2: -102+-8- better! [+8] **************************
# window = 1: -98+-6 - worse, undo
# more agressive timing: -101+-6 - a wash, keep
# parallel: synch after each window:            -281 - good.
# switch to endgame at different N: 60: -98+-9 - slightly worse, undo
# 58: -86+-8 - slightly worse, undo
# no early stopping on fail low, incr t1 by 1.2: -100+-8 - a wash, accept
# no early stopping if move changed:-99+-7 - a wash, accept


