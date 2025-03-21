src/: C++ source code of the "reversi" program.


src/project/rev.cpb - project file for Code::Blocks IDE under Linux, compiles with GCC compiler.


features: parallel search - simplified ABDADA, from http://www.tckerrigan.com/Chess/Parallel_Search/Simplified_ABDADA/

"make move" and "bit mob" logic - from Edax, at https://github.com/abulmo/edax-reversi

GGS interface - from Chris Welty, at https://skatgame.net/mburo/ggs/chris/ODK.zip

command line interface is a simplified version of chess UCI interface; main commands are:
	position FEN ------------O---*-**O*--OOOOO----O*OO-----**O---OO*--O----*----- * - set the board, including side to move
	go time XXXX - start thinking, using time in milliseconds for the rest of the game
	ggs - connects to GGS at "skatgame.net"; put logic and password in file ODK/GGSmain.cpp, line 24

options:
	size of main hash table is set in line 1 of file hash.h
	number of calculation threads (in addition to main calculation thread) is set in line 7 of file main.cpp

evaluation function: features (mobilities, corner 10, corner 9, all straight lines of length 4+), with parameters that do not depend on number of discs on the board, calculated as a vector of length 32, then floored at 0, and summed-up (last 16 with opposite sign, to enforce some sort of +- symmetry)
evaluation parameters developed using Python script python/eval/train_eval.py

search: the only non-trivial addition is modifying the width of the multi-prob-cut search based on current position. See file "sigmas.cpp" for implementation. Based on the neural network similar to evaluation function. Training script is python/sigma_mult/train_sigmas.py



python/: scripts for training evaluation function, MPC sigma multiplier function, and for play against Edax or a different version of myself



data/: the final parameters used by the program
