# 22-23Q1 EDA game tournament

This repository contains the source code of my AI player (`AIRekkles.cc`) which got into the finals of the tournament (out of 243 players).
Furthermore, it includes the rules, files and some AI players of the game.

##  Description

The game is called "The Walking Dead" created by professor Albert Oliveras at UPC for the Data Structures and Algorithms class. The goal is to earn the maximum points by killing other players, killing zombies and/or conquering terrain. All the mechanics of the game are explained in the [game-rules](./game_info/games-rules.pdf) file.

## Creating you own player

To create your player, it is recommended to start with the `AIDemo.cc` as base code and using the commands from the [API](./game_info/api.pdf) of the game to code it.

1. Create a copy of the `AIDemo.cc` file in the `game` folder.

```bash
cp AIDemo.cc AI<player-name>.cc
```

2. Change the player's name inside the `AI<player-name>.cc` file:

```cpp
/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME <player-name>  <--- HERE
```

## Running a single game

There are two modes of running a single game, which are "normal" or "debug" mode.

### Running a game in normal mode

To run a single game in "normal" mode:

1. Go to the `Makefile` file and make sure that the configuration section looks like this:

```cpp
# Configuration
OPTIMIZE = 3 # Optimization level    (0 to 3)
DEBUG    = 0 # Compile for debugging (0 or 1)
PROFILE  = 0 # Compile for profile   (0 or 1)
```

2. Also, in the EXTRA_OBJ section, add all the players `.o` you have (including yours):

```c++
# Add here any extra .o player files you want to link to the executable
EXTRA_OBJ = [<player1-name>.o] [<player2-name>.o] [...]
```

3. Before compiling, make a copy of some files with:

```bash
// For Linux
cp AIDummy.o.Linux64 AIDummy.o
cp Board.o.Linux64 Board.o

// For Intel-based macOS
cp AIDummy.o.MacOS AIDummy.o
cp Board.o.MacOS Board.o

// For ARM-based macOS
cp AIDummy.o.MacOS.ARM AIDummy.o
cp Board.o.MacOS.ARM Board.o
```

4. Compile everything with:

```bash
make all
```

5. Display all the players available:

```bash
./Game -l
```

6. To run a game:

```bash
./Game <player1> <player2> <player3> <player4> -i default.cnf -o default.res -s <seed-number>
```

###  Running a game in debug mode

To run a game in "debug" mode, follow all the same steps but changing step 1 and 3:

1. Go to the `Makefile` file and make sure that the configuration section looks like this:

```cpp
# Configuration
OPTIMIZE = 0 # Optimization level    (0 to 3)
DEBUG    = 1 # Compile for debugging (0 or 1)
PROFILE  = 0 # Compile for profile   (0 or 1)
```

3. Before compiling, make a copy of some files with:

```bash
// For Linux
cp AIDummy.o.Linux64.Debug AIDummy.o
cp Board.o.Linux64.Debug Board.o

// For Intel-based macOS
cp AIDummy.o.MacOS.Debug AIDummy.o
cp Board.o.MacOS.Debug Board.o

// For ARM-based macOS
cp AIDummy.o.MacOS.ARM.Debug AIDummy.o
cp Board.o.MacOS.ARM.Debug Board.o
```

### Watching a game in the browser

After running a game in either mode, there is the possibility to watch the game in the browser. To do so, just find the `default.res` file that has been generated and open it.

## Running multiple games

To play multiple rounds automatically, you can use the `tester.cc` script. When all the rounds are played, the results will be printed. The results contain for each player, the number of games the player got 1st place, 2nd place... Moreover, it prints the average points obtained between all the rounds. It can be extremely useful to see how your player performs against others without the randomness that playing only one game has. To execute it:

1. Copy the script to the [game](game) folder
2. To compile the script:

```bash
g++ -o tester tester.cc -pthread
```

3. To execute the script:

```bash
./tester
```

4. Enter the name of the players playing the rounds from the available players printed:

```bash
<player1> <player2> <player3> <player4>
```

5. Enter the number of rounds to be played:

```bash
<n>
```

6. Enter the initial seed for the game map generation:

```bash
<initial-seed>
```

7. Wait for the results to be printed! (it can take more or less time depending on the number of rounds)
