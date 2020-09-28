# Tic-Tac-Toe-AI

Compile line: g++ linkage.c -lX11 -lm

Project description:
A less known strategy game named "linkage". It is interesting because it has an asymmetric winning condition. See the rules at

www.iggamecenter.com/info/en/linkage.html

https://boardgamegeek.com/thread/643724/deceptively-simple-abstract

The game linkage is played on a 7 by 7 board with a hole in the middle. There is a common pool of 2x1 rectangles in four colors, with six pieces in each color. Players alternately places pieces in unoccupied spaces (we drop the condition that it should not be neighbor of the piece played by the opponent in the previous move; this condition was only later added, but the author writes it works also without), as long as possible. Then the game ends, the connected components of each color are counted (pieces of the same color connected along a side) If the number of connected components is at least 12, the "More" player wins. If it is 11 or less, the "less" player wins.

More description of the project is included on file "Project 1.pdf".
