# Liar's Dice
Liar’s Dice is a multiplayer dice game with a minimum of two players and no upper limit on the number
of participants. The goal is to make a correct guess or correctly call another player a liar.

# Scenario
Liar’s Dice uses 5 dice per player (so 15 dice for three people, 20 dice for four people, and so on). The object of the game is to guess how many of a particular die sides are showing among all the players, knowing only your own set. Each player’s guess has to increase the number of die sides on the table. Guessing continues until a player is called out as a “liar” and all the dice are revealed.

# Background
_Most of the information below came from this [article](https://www.thoughtco.com/how-to-play-liars-dice-687532)._

Throughout China, Liar’s Dice (說謊者的骰子, shuōhuǎng zhě de shǎizi) is played during holidays, especially Chinese New Year. The fast-paced game can be played by two or more players and the number of rounds is limitless. Players usually agree to a predetermined number of rounds or set a time limit but none of that is set in stone; new players and additional rounds can be added as the game goes along. While the number of players and rounds may be casual, Liar’s Dice can also be quite intense as it's traditionally a drinking game. In China, in addition to holiday celebrations, it's also common to see it being played at bars, in clubs, and even outdoors at sidewalk restaurants.

When played not digitally, you would typically have:
- One cup per player
- Five dice per player
- One table

# Rules
- The first player calls out two numbers: first, how many dice on the table he or she thinks have
been rolled as a number between one and six. For example, player one could say “two fives,”
which means he or she thinks there are at least two dice that are fives among all the players’
dice (including his or her own).
o At this point, all players can accept what has been called out and move on to player two
or call player one out, which will end the round and result in a winner or loser for the
round.
- If player one calls out “two fives,” it does not matter whether player one has a five or not as
bluffing is allowed in Liar’s Dice. It only matters if another player believes player one is bluffing
and calls him or her out on it. In that instance, all dice must be revealed. If player one is correct,
the player wins. If player one is wrong, then the player who called him/her out is the winner.
The round is then over.
- If player one’s call is accepted, then player two calls out a number. The first number must be
greater than what player one called. For example, if player one called out “two fives,” player
two must call out three or higher for his or her first number, so “three fives,” “three fours,” or
four twos” would all be acceptable. “One five” or “two sixes” would be unacceptable.
- Game play continues until someone is called out.

# Project Structure
```md
LiarDiceGame/
│
├── src/
│   ├── controller/
│   │   └── Game.cpp
│   ├── model/
│   │   ├── Player.cpp
│   │   └── Dice.cpp
│   ├── views/
│   └── main.cpp
│
├── include/
│   ├── controller/
│   │   └── Game.hpp
│   ├── model/
│   │   ├── Player.hpp
│   │   └── Dice.hpp
│   └── views/
│
├── assets/
│   └── rules.txt
│
├── build/ (or dist/)
│
├── CMakeLists.txt
│
└── README.md
```
