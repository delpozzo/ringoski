// ###########################################################################
//          Title: RingoSki
//         Author: Mike Del Pozzo
//    Description: A skiing game for Ringo (MAKERphone).
//        Version: 1.0.0
//           Date: 08 July 2020
//        License: GPLv3 (see LICENSE)
//
//    RingoSki Copyright (C) 2020 Mike Del Pozzo
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    any later version.
// ###########################################################################

#include <MAKERphone.h>
#include "Sprite.h"

// ######################################################
//    Defines, Enums, Structs, and Globals
// ######################################################

#define MAXENTS 80 // maximum number of entities that can exist at one time
#define MAXFRAMES 8 // maximum number of sprites per entity
#define MAXFLAGS 4 // maximum number of flags per entity
#define MAPWIDTH 600 // map width
#define MAPHEIGHT 750 // map height
#define CAMERAXOFFSET LCDWIDTH/2 // camera X offset
#define CAMERAYOFFSET (LCDHEIGHT/2) + 16 // camera Y offset
#define XSTARTPOS (MAPWIDTH/2) // starting x coordinate for player
#define YSTARTPOS (LCDHEIGHT/2) // starting y coordinate for player
#define SCROLLPOS MAPHEIGHT - LCDHEIGHT + 40 // y coordinate at which obstacles refresh
#define XLIMITR MAPWIDTH - 16 // right-most x coordinate the player can travel
#define XLIMITL 0 // left-most x coordinate the player can travel
#define JUMPDURATION 12 // how long the player jumps for
#define GAMEOVERTIMER 16 // duration before entering game over loop
#define CRASHTIMER 25 // duration before getting back up after crashing
#define INVULTIMER 25 // duration player is invincible after crashing
#define YETIDASHDURATION 10 // duration of yeti dash (higher number makes game harder)
#define NUMTREES 33 // number of trees that exist at a given time
#define NUMLOGS 25 // number of logs that exist at a given time
#define NUMPENGUINS 10 // number of penguins that exist at a given time

enum ENTTYPES {PLAYER=0, OBSTACLE=1, YETI=2}; // types of entities
enum PLAYERFLAGS {JUMP=0, GAMEOVER=1, CRASH=2, INVUL=3}; // flags for player
enum YETIFLAGS {PURSUIT=0, YETIDASH=1, YETIANIMATEFWD=2}; // flags for yeti
enum LOOPTYPES {MENULOOP=0, GAMELOOP=1, PAUSELOOP=2, RESETSCORELOOP=3, GAMEOVERLOOP=4}; // determines which loop to execute

const char *highScorePath = "/RingoSki/hiscore.sav";

// Entity structure
typedef struct ENTITY_S
{
  byte frame; // sprite frame that will be rendered in the next drawAll() iteration
  Sprite *sprite[MAXFRAMES]; // array of sprites for entity of size MAXFRAMES
  int16_t x; // x coordinate of entity
  int16_t y; // y coordinate of entity
  byte type; // entity type (PLAYER or OBSTACLE)
  byte xspeed; // x movement speed
  byte yspeed; // y movement speed
  byte flag[MAXFLAGS]; // array of flags for entity of size MAXFLAGS
  boolean used; // whether this particular entity in the EntityList[] is in use or not
  void(*think)(struct ENTITY_S *self); // think function that will be called in the next thinkAll() iteration
} Entity;

Entity EntityList[MAXENTS]; // global array of entities

MAKERphone mp; //  MAKERphone object
Entity *player; // global pointer to the player's entity
Entity *yeti; // global pointer to the yeti's entity
int16_t cameraX, cameraY;// coordinates of the camera
int metersTraveled; // number of meters player has traveled (score)
int highScore; // most meters traveled without crashing
byte activeLoop; // determines which loop to execute
byte menuBlink; // flag for flashing menu text
boolean debugMode; // flag to determine if bounding boxes are drawn (for collision debugging)

// Sounds
MPTrack *sndJump; // jump sound
MPTrack *sndSlide; // power slide sound
MPTrack *sndCrash; // crash sound
MPTrack *sndGameOver; // game over sound
MPTrack *sndMenuMusic; // main menu music
MPTrack *sndStart; // menu start sound

// Sprites
Sprite spPlayerNormal;
Sprite spPlayerLeft;
Sprite spPlayerRight;
Sprite spPlayerLeftSlide;
Sprite spPlayerRightSlide;
Sprite spPlayerJump;
Sprite spPlayerSpeed;
Sprite spPlayerCrash;
Sprite spTree1;
Sprite spTree2;
Sprite spTree3;
Sprite spTree4;
Sprite spTree5;
Sprite spTree6;
Sprite spTree7;
Sprite spTree8;
Sprite spTree9;
Sprite spLog1;
Sprite spLog2;
Sprite spLog3;
Sprite spLog4;
Sprite spYeti1;
Sprite spYeti2;
Sprite spYeti3;
Sprite spYeti4;
Sprite spYeti5;
Sprite spYeti6;
Sprite spYeti7;
Sprite spYeti8;
Sprite spPenguin1;
Sprite spPenguin2;
Sprite spPenguin3;
Sprite spPenguin4;

// ######################################################
//    Game Setup and Main Logic
// ######################################################

// Entry Point
void setup()
{
  Serial.begin(115200);
  mp.begin();
  randomSeed(millis());

  initSprites();
  initEntities();
  initCamera();

  startMenu();
}

// Main loop
void loop()
{
  // activeLoop determines which loop to execute
  switch(activeLoop)
  {
    case GAMELOOP: gameLoop();
      break;
    case GAMEOVERLOOP: gameOverLoop();
      break;
    case MENULOOP: menuLoop();
      break;
    case RESETSCORELOOP: resetScoreLoop();
      break;
    case PAUSELOOP: pauseLoop();
      break;
  }
}

void startMenu()
{
  debugMode = 0;
  menuBlink = 0;
  activeLoop = MENULOOP;
  reloadSounds();
  playSound(sndMenuMusic);
}

// Menu Loop
void menuLoop()
{
  mp.update();
  mp.display.fillScreen(TFT_WHITE);
  drawMenu();

  // Button A - Start Game
  if(mp.buttons.released(BTN_A))
  {
    sndMenuMusic->stop();
    playSound(sndStart);
    while(sndStart->isPlaying()){}

    startGame();
  }

  // Function Left - Reset High Score
  else if(mp.buttons.released(BTN_FUN_LEFT))
  {
    highScore = getHighScore();
    activeLoop = RESETSCORELOOP;
  }

  // Function Right - Quit
  else if(mp.buttons.released(BTN_FUN_RIGHT))
  {
    mp.loader();
  }
}

// High Score Reset Loop
void resetScoreLoop()
{
  mp.update();
  mp.display.fillScreen(TFT_WHITE);
  drawResetScore();

  // Button A - Confirm
  if(mp.buttons.released(BTN_A))
  {
    resetHighScore();
    startMenu();
  }

  // Button B - Back / Cancel
  else if(mp.buttons.released(BTN_B))
  {
    startMenu();
  }
}

// Start game
void startGame()
{
  activeLoop = GAMELOOP;
  reloadSounds();
  highScore = getHighScore();
  metersTraveled = 0;
  spawnObstacles();
  yeti = spawnYeti();
  player = spawnPlayer(XSTARTPOS, YSTARTPOS);
}

// Game Loop
void gameLoop()
{
  mp.update();
  mp.display.fillScreen(TFT_WHITE);

  thinkAll(); // call each entity's think function
  updateCamera(); // center camera on player
  drawAll(); // draw each entity's current frame
  drawScore(); // draw score

  if(debugMode) // draw debug information
  {
    drawDebug();
  }
}

void startGameOver()
{
  activeLoop = GAMEOVERLOOP;

  while(sndGameOver->isPlaying()){}

  if(metersTraveled > highScore)
  {
    saveHighScore();
  }
}

// Game Over Loop
void gameOverLoop()
{
  mp.update();
  mp.display.fillScreen(TFT_WHITE);
  drawGameOver();

  // Button A - Play Again
  if(mp.buttons.released(BTN_A))
  {
    clearEntities();
    startGame();
  }

  // Button B - Quit
  else if(mp.buttons.released(BTN_B))
  {
    clearEntities();
    startMenu();
  }
}

// Pause Loop
void pauseLoop()
{
  mp.update();
  mp.display.fillScreen(TFT_WHITE);
  drawPause();

  // Button A - Unpause
  if(mp.buttons.released(BTN_A))
  {
    activeLoop = GAMELOOP;
  }

  // Button B - Quit
  else if(mp.buttons.released(BTN_B))
  {
    clearEntities();
    startMenu();
  }
}

// ######################################################
//    Menu, Pause, and Game Over Functions
// ######################################################

void drawMenu()
{
  // RingoSki
  mp.display.setTextFont(2);
  mp.display.setTextSize(2);
  mp.display.setTextColor(TFT_RED);
  mp.display.setCursor(28, 2);
  mp.display.printf("Ringo");
  mp.display.setTextColor(TFT_BLUE);
  mp.display.printf("Ski");

  // Created By
  mp.display.setTextFont(1);
  mp.display.setTextSize(1);
  mp.display.setTextColor(TFT_LIGHTGREY);
  mp.display.setCursor(36, 40);
  mp.display.printf("  Created by");
  mp.display.setCursor(36, 48);
  mp.display.printf("Mike Del Pozzo");

  // Press A to Play
  mp.display.setTextFont(2);
  mp.display.setCursor(28, 60);
  if(menuBlink < 25)
  {
    mp.display.setTextColor(TFT_WHITE);
  }
  else
  {
    mp.display.setTextColor(TFT_MAGENTA);
  }
  mp.display.printf("Press A to Play");

  // Reset High Score
  mp.display.setTextFont(1);
  mp.display.setTextSize(1);
  mp.display.setTextColor(TFT_RED);
  mp.display.setCursor(4, LCDHEIGHT - 24);
  mp.display.printf("Reset");
  mp.display.setCursor(4, LCDHEIGHT - 16);
  mp.display.printf("Score");
  mp.display.setCursor(4, LCDHEIGHT - 8);
  mp.display.printf("|");

  // Quit
  mp.display.setTextFont(1);
  mp.display.setTextSize(1);
  mp.display.setCursor(132, LCDHEIGHT - 16);
  mp.display.printf("Quit");
  mp.display.setCursor(150, LCDHEIGHT - 8);
  mp.display.printf("|");

  // Draw Player
  mp.display.drawIcon(spPlayerRight.spData, 50, 85, spPlayerRight.w,
                      spPlayerRight.h, 1, TFT_TRANSPARENT);

  // Draw Tree
  mp.display.drawIcon(spTree5.spData, 80, 75, spTree5.w,
                      spTree5.h, 1, TFT_TRANSPARENT);

  // Drive Menu Blink
  menuBlink++;
  if(menuBlink >= 50)
  {
    menuBlink = 0;
  }
}

void drawResetScore()
{
  // Current High Score
  mp.display.setTextFont(1);
  mp.display.setTextSize(1);
  mp.display.setTextColor(TFT_BLACK);
  mp.display.setCursor(2, 2);
  mp.display.printf("High Score: %iM", highScore);

  // Are you sure?
  mp.display.setTextFont(2);
  mp.display.setTextColor(TFT_RED);
  mp.display.setCursor(10, 36);
  mp.display.printf("Reset the high score?");

  // A: Yes
  mp.display.setTextFont(1);
  mp.display.setTextSize(2);
  mp.display.setTextColor(TFT_BLACK);
  mp.display.setCursor(36, 65);
  mp.display.printf("A: Yes");

  // B: No
  mp.display.setCursor(36, 85);
  mp.display.printf("B: No");
}

void drawPause()
{
  // RingoSki
  mp.display.setTextFont(2);
  mp.display.setTextSize(2);
  mp.display.setTextColor(TFT_RED);
  mp.display.setCursor(28, 2);
  mp.display.printf("Ringo");
  mp.display.setTextColor(TFT_BLUE);
  mp.display.printf("Ski");

  // Paused
  mp.display.setTextFont(2);
  mp.display.setTextSize(1);
  mp.display.setTextColor(TFT_BLACK);
  mp.display.setCursor(36, 46);
  mp.display.printf("P A U S E D");

  // A: Resume
  mp.display.setTextFont(1);
  mp.display.setTextSize(2);
  mp.display.setCursor(24, 75);
  mp.display.printf("A: Resume");

  // B: Quit
  mp.display.setCursor(24, 95);
  mp.display.printf("B: Quit");
}

void drawGameOver()
{
  drawScore();

  if(metersTraveled > highScore)
  {
    mp.display.setTextFont(1);
    mp.display.setTextSize(1);
    if(menuBlink < 25)
    {
      mp.display.setTextColor(TFT_WHITE);
    }
    else
    {
      mp.display.setTextColor(TFT_MAGENTA);
    }
    mp.display.setCursor(35, 28);
    mp.display.printf("New High Score!");

    // Drive Menu Blink
    menuBlink++;
    if(menuBlink >= 50)
    {
      menuBlink = 0;
    }
  }

  // Game Over
  mp.display.setTextFont(2);
  mp.display.setTextSize(1);
  mp.display.setTextColor(TFT_RED);
  mp.display.setCursor(42, 42);
  mp.display.printf("GAME OVER");

  // A: Play Again
  mp.display.setTextFont(1);
  mp.display.setTextSize(2);
  mp.display.setTextColor(TFT_BLACK);
  mp.display.setCursor(24, 75);
  mp.display.printf("A: Retry");

  // B: Quit
  mp.display.setCursor(24, 95);
  mp.display.printf("B: Quit");
}

// ######################################################
//    Sound Functions
// ######################################################

void initSounds()
{
  // Load menu sounds if at the menu
  if(activeLoop == MENULOOP)
  {
    sndMenuMusic = new MPTrack("/RingoSki/menu.wav");
    sndMenuMusic->setVolume(map(mp.mediaVolume, 0, 14, 100, 300));
    addTrack(sndMenuMusic);

    sndStart = new MPTrack("/RingoSki/start.wav");
    sndStart->setVolume(map(mp.mediaVolume, 0, 14, 100, 300));
    addTrack(sndStart);
  }
  else // Otherwise load normal game sounds
  {
    sndJump = new MPTrack("/RingoSki/jump.wav");
    sndJump->setVolume(map(mp.mediaVolume, 0, 14, 100, 300));
    addTrack(sndJump);

    sndSlide = new MPTrack("/RingoSki/speed.wav");
    sndSlide->setVolume(map(mp.mediaVolume, 0, 14, 100, 300));
    addTrack(sndSlide);

    sndCrash = new MPTrack("/RingoSki/crash.wav");
    sndCrash->setVolume(map(mp.mediaVolume, 0, 14, 100, 300));
    addTrack(sndCrash);

    sndGameOver = new MPTrack("/RingoSki/gameover.wav");
    sndGameOver->setVolume(map(mp.mediaVolume, 0, 14, 100, 300));
    addTrack(sndGameOver);
  }
}

void reloadSounds()
{
  closeSounds();
  initSounds();
}

void closeSounds()
{
  for(int i = 0; i < 4; i++)
  {
    if(tracks[i] != NULL)
    {
      removeTrack(tracks[i]);
    }
  }
}

void playSound(MPTrack *snd)
{
  // safety check (sometimes things go wrong with sounds loaded from SD card)
  if(snd == NULL || snd->getSize() <= 0)
  {
    reloadSounds();
  }

  snd->rewind();
  snd->play();
}

// ######################################################
//    Score and Debug Functions
// ######################################################

void drawScore()
{
  mp.display.setTextFont(1);
  mp.display.setTextColor(TFT_BLACK);
  mp.display.setTextSize(1);
  mp.display.setCursor(2, 2);
  mp.display.printf("Score:%iM", metersTraveled);
  mp.display.setCursor(85, 2);

  if(metersTraveled > highScore)
  {
    mp.display.setTextColor(TFT_MAGENTA);
    mp.display.printf("  New Best!");
  }
  else
  {
    mp.display.printf("Best:%iM", highScore);
  }
}

int getHighScore()
{
  int value = 0;

  // Note: Need to unload sounds before accessing the SD card, otherwise the following error occurs:
  // ERROR: vfs_fat: open: no free file descriptors
  closeSounds();

  File file = SD.open(highScorePath);
  file.read((byte*)&value, sizeof(int)); // read 4 bytes
  file.close();

  reloadSounds();

  return value;
}

void saveHighScore()
{
  // Note: Need to unload sounds before accessing the SD card, otherwise the following error occurs:
  // ERROR: vfs_fat: open: no free file descriptors
  closeSounds();

  File file = SD.open(highScorePath, "w");
  file.write((byte*)&metersTraveled, sizeof(int)); // write 4 bytes
  file.close();

  reloadSounds();
}

void resetHighScore()
{
  int value = 0;

  // Note: Need to unload sounds before accessing the SD card, otherwise the following error occurs:
  // ERROR: vfs_fat: open: no free file descriptors
  closeSounds();

  File file = SD.open(highScorePath, "w");
  file.write((byte*)&value, sizeof(int)); // write 4 bytes
  file.close();

  reloadSounds();
}

void drawDebug()
{
  mp.display.setTextFont(1);
  mp.display.setTextColor(TFT_BLUE);
  mp.display.setTextSize(1);
  mp.display.setCursor(2, 16);
  mp.display.printf("DEBUG INFO");
  mp.display.setCursor(2, 26);
  mp.display.printf(" coords: (%i,%i)", player->x, player->y);
  mp.display.setCursor(2, 36);
  mp.display.printf(" xspeed: %i", player->xspeed);
  mp.display.setCursor(2, 46);
  mp.display.printf(" yspeed: %i", player->yspeed);
  mp.display.setCursor(2, 56);
  mp.display.printf("  frame: %i", player->frame);
}

// ######################################################
//    Entity Management, Collision, and Draw Functions
// ######################################################

// Initializes EntityList[] with default values, sets pointers to NULL
void initEntities()
{
  for(int i = 0; i < MAXENTS; i++)
  {
    EntityList[i].used = 0;
    EntityList[i].think = NULL;

    for(int j = 0; j < MAXFRAMES; j++)
    {
      EntityList[i].sprite[j] = NULL;
    }

    for(int k = 0; k < MAXFLAGS; k++)
    {
      EntityList[i].flag[k] = 0;
    }
  }
}

// Initialize game sprites and bounding boxes used for collisions
void initSprites()
{
  spPlayerNormal.spData = playerNormal;
  spPlayerNormal.w = 16;
  spPlayerNormal.h = 24;
  spPlayerNormal.numBbox = 1;
  spPlayerNormal.box[0].x = 4;
  spPlayerNormal.box[0].y = 14;
  spPlayerNormal.box[0].w = 8;
  spPlayerNormal.box[0].h = 8;

  spPlayerLeft.spData = playerLeft;
  spPlayerLeft.w = 16;
  spPlayerLeft.h = 24;
  spPlayerLeft.numBbox = 1;
  spPlayerLeft.box[0].x = spPlayerNormal.box[0].x;
  spPlayerLeft.box[0].y = spPlayerNormal.box[0].y;
  spPlayerLeft.box[0].w = spPlayerNormal.box[0].w;
  spPlayerLeft.box[0].h = spPlayerNormal.box[0].h;

  spPlayerRight.spData = playerRight;
  spPlayerRight.w = 16;
  spPlayerRight.h = 24;
  spPlayerRight.numBbox = 1;
  spPlayerRight.box[0].x = spPlayerNormal.box[0].x;
  spPlayerRight.box[0].y = spPlayerNormal.box[0].y;
  spPlayerRight.box[0].w = spPlayerNormal.box[0].w;
  spPlayerRight.box[0].h = spPlayerNormal.box[0].h;

  spPlayerLeftSlide.spData = playerLeftSlide;
  spPlayerLeftSlide.w = 16;
  spPlayerLeftSlide.h = 24;
  spPlayerLeftSlide.numBbox = 1;
  spPlayerLeftSlide.box[0].x = spPlayerNormal.box[0].x;
  spPlayerLeftSlide.box[0].y = spPlayerNormal.box[0].y;
  spPlayerLeftSlide.box[0].w = spPlayerNormal.box[0].w;
  spPlayerLeftSlide.box[0].h = spPlayerNormal.box[0].h;

  spPlayerRightSlide.spData = playerRightSlide;
  spPlayerRightSlide.w = 16;
  spPlayerRightSlide.h = 24;
  spPlayerRightSlide.numBbox = 1;
  spPlayerRightSlide.box[0].x = spPlayerNormal.box[0].x;
  spPlayerRightSlide.box[0].y = spPlayerNormal.box[0].y;
  spPlayerRightSlide.box[0].w = spPlayerNormal.box[0].w;
  spPlayerRightSlide.box[0].h = spPlayerNormal.box[0].h;

  spPlayerJump.spData = playerJump;
  spPlayerJump.w = 16;
  spPlayerJump.h = 24;
  spPlayerJump.numBbox = 1;
  spPlayerJump.box[0].x = spPlayerNormal.box[0].x;
  spPlayerJump.box[0].y = spPlayerNormal.box[0].y;
  spPlayerJump.box[0].w = spPlayerNormal.box[0].w;
  spPlayerJump.box[0].h = spPlayerNormal.box[0].h;

  spPlayerSpeed.spData = playerSpeed;
  spPlayerSpeed.w = 16;
  spPlayerSpeed.h = 24;
  spPlayerSpeed.numBbox = 1;
  spPlayerSpeed.box[0].x = spPlayerNormal.box[0].x;
  spPlayerSpeed.box[0].y = spPlayerNormal.box[0].y;
  spPlayerSpeed.box[0].w = spPlayerNormal.box[0].w;
  spPlayerSpeed.box[0].h = spPlayerNormal.box[0].h;

  spPlayerCrash.spData = playerCrash;
  spPlayerCrash.w = 16;
  spPlayerCrash.h = 24;
  spPlayerCrash.numBbox = 1;
  spPlayerCrash.box[0].x = spPlayerNormal.box[0].x;
  spPlayerCrash.box[0].y = spPlayerNormal.box[0].y;
  spPlayerCrash.box[0].w = spPlayerNormal.box[0].w;
  spPlayerCrash.box[0].h = spPlayerNormal.box[0].h;

  spTree1.spData = tree1;
  spTree1.w = 32;
  spTree1.h = 63;
  spTree1.numBbox = 2;
  spTree1.box[0].x = 8;
  spTree1.box[0].y = 10;
  spTree1.box[0].w = 14;
  spTree1.box[0].h = 30;
  spTree1.box[1].x = 12;
  spTree1.box[1].y = 40;
  spTree1.box[1].w = 3;
  spTree1.box[1].h = 20;

  spTree2.spData = tree2;
  spTree2.w = 32;
  spTree2.h = 40;
  spTree2.numBbox = 2;
  spTree2.box[0].x = 6;
  spTree2.box[0].y = 20;
  spTree2.box[0].w = 18;
  spTree2.box[0].h = 18;
  spTree2.box[1].x = 11;
  spTree2.box[1].y = 6;
  spTree2.box[1].w = 8;
  spTree2.box[1].h = 14;

  spTree3.spData = tree3;
  spTree3.w = 32;
  spTree3.h = 46;
  spTree3.numBbox = 2;
  spTree3.box[0].x = 7;
  spTree3.box[0].y = 20;
  spTree3.box[0].w = 16;
  spTree3.box[0].h = 20;
  spTree3.box[1].x = 11;
  spTree3.box[1].y = 6;
  spTree3.box[1].w = 8;
  spTree3.box[1].h = 14;

  spTree4.spData = tree4;
  spTree4.w = 32;
  spTree4.h = 38;
  spTree4.numBbox = 2;
  spTree4.box[0].x = 6;
  spTree4.box[0].y = 22;
  spTree4.box[0].w = 18;
  spTree4.box[0].h = 14;
  spTree4.box[1].x = 12;
  spTree4.box[1].y = 6;
  spTree4.box[1].w = 6;
  spTree4.box[1].h = 16;

  spTree5.spData = tree5;
  spTree5.w = 32;
  spTree5.h = 44;
  spTree5.numBbox = 2;
  spTree5.box[0].x = 6;
  spTree5.box[0].y = 20;
  spTree5.box[0].w = 19;
  spTree5.box[0].h = 18;
  spTree5.box[1].x = 12;
  spTree5.box[1].y = 6;
  spTree5.box[1].w = 8;
  spTree5.box[1].h = 14;

  spTree6.spData = tree6;
  spTree6.w = 32;
  spTree6.h = 56;
  spTree6.numBbox = 2;
  spTree6.box[0].x = 6;
  spTree6.box[0].y = 22;
  spTree6.box[0].w = 19;
  spTree6.box[0].h = 28;
  spTree6.box[1].x = 12;
  spTree6.box[1].y = 6;
  spTree6.box[1].w = 8;
  spTree6.box[1].h = 16;

  spTree7.spData = tree7;
  spTree7.w = 32;
  spTree7.h = 58;
  spTree7.numBbox = 2;
  spTree7.box[0].x = 6;
  spTree7.box[0].y = 22;
  spTree7.box[0].w = 19;
  spTree7.box[0].h = 28;
  spTree7.box[1].x = 12;
  spTree7.box[1].y = 6;
  spTree7.box[1].w = 8;
  spTree7.box[1].h = 16;

  spTree8.spData = tree8;
  spTree8.w = 32;
  spTree8.h = 46;
  spTree8.numBbox = 2;
  spTree8.box[0].x = 6;
  spTree8.box[0].y = 20;
  spTree8.box[0].w = 19;
  spTree8.box[0].h = 22;
  spTree8.box[1].x = 12;
  spTree8.box[1].y = 6;
  spTree8.box[1].w = 8;
  spTree8.box[1].h = 14;

  spTree9.spData = tree9;
  spTree9.w = 32;
  spTree9.h = 38;
  spTree9.numBbox = 2;
  spTree9.box[0].x = 6;
  spTree9.box[0].y = 8;
  spTree9.box[0].w = 20;
  spTree9.box[0].h = 20;
  spTree9.box[1].x = 15;
  spTree9.box[1].y = 28;
  spTree9.box[1].w = 2;
  spTree9.box[1].h = 8;

  spLog1.spData = woodLog1;
  spLog1.w = 32;
  spLog1.h = 12;
  spLog1.numBbox = 1;
  spLog1.box[0].x = 0;
  spLog1.box[0].y = 2;
  spLog1.box[0].w = 32;
  spLog1.box[0].h = 8;

  spLog2.spData = woodLog2;
  spLog2.w = 24;
  spLog2.h = 8;
  spLog2.numBbox = 1;
  spLog2.box[0].x = 0;
  spLog2.box[0].y = 0;
  spLog2.box[0].w = 24;
  spLog2.box[0].h = 8;

  spLog3.spData = woodLog3;
  spLog3.w = 24;
  spLog3.h = 10;
  spLog3.numBbox = 1;
  spLog3.box[0].x = 0;
  spLog3.box[0].y = 1;
  spLog3.box[0].w = 24;
  spLog3.box[0].h = 8;

  spLog4.spData = woodLog4;
  spLog4.w = 32;
  spLog4.h = 12;
  spLog4.numBbox = 1;
  spLog4.box[0].x = 0;
  spLog4.box[0].y = 2;
  spLog4.box[0].w = 32;
  spLog4.box[0].h = 8;

  spYeti1.spData = yeti1;
  spYeti1.w = 36;
  spYeti1.h = 36;
  spYeti1.numBbox = 1;
  spYeti1.box[0].x = 9;
  spYeti1.box[0].y = 14;
  spYeti1.box[0].w = 18;
  spYeti1.box[0].h = 18;

  spYeti2.spData = yeti2;
  spYeti2.w = 36;
  spYeti2.h = 36;
  spYeti2.numBbox = 1;
  spYeti2.box[0].x = spYeti1.box[0].x;
  spYeti2.box[0].y = spYeti1.box[0].y;
  spYeti2.box[0].w = spYeti1.box[0].w;
  spYeti2.box[0].h = spYeti1.box[0].h;

  spYeti3.spData = yeti3;
  spYeti3.w = 36;
  spYeti3.h = 36;
  spYeti3.numBbox = 1;
  spYeti3.box[0].x = spYeti1.box[0].x;
  spYeti3.box[0].y = spYeti1.box[0].y;
  spYeti3.box[0].w = spYeti1.box[0].w;
  spYeti3.box[0].h = spYeti1.box[0].h;

  spYeti4.spData = yeti4;
  spYeti4.w = 36;
  spYeti4.h = 36;
  spYeti4.numBbox = 1;
  spYeti4.box[0].x = spYeti1.box[0].x;
  spYeti4.box[0].y = spYeti1.box[0].y;
  spYeti4.box[0].w = spYeti1.box[0].w;
  spYeti4.box[0].h = spYeti1.box[0].h;

  spYeti5.spData = yeti5;
  spYeti5.w = 36;
  spYeti5.h = 36;
  spYeti5.numBbox = 1;
  spYeti5.box[0].x = spYeti1.box[0].x;
  spYeti5.box[0].y = spYeti1.box[0].y;
  spYeti5.box[0].w = spYeti1.box[0].w;
  spYeti5.box[0].h = spYeti1.box[0].h;

  spYeti6.spData = yeti6;
  spYeti6.w = 36;
  spYeti6.h = 36;
  spYeti6.numBbox = 1;
  spYeti6.box[0].x = spYeti1.box[0].x;
  spYeti6.box[0].y = spYeti1.box[0].y;
  spYeti6.box[0].w = spYeti1.box[0].w;
  spYeti6.box[0].h = spYeti1.box[0].h;

  spYeti7.spData = yeti7;
  spYeti7.w = 36;
  spYeti7.h = 36;
  spYeti7.numBbox = 1;
  spYeti7.box[0].x = spYeti1.box[0].x;
  spYeti7.box[0].y = spYeti1.box[0].y;
  spYeti7.box[0].w = spYeti1.box[0].w;
  spYeti7.box[0].h = spYeti1.box[0].h;

  spYeti8.spData = yeti8;
  spYeti8.w = 36;
  spYeti8.h = 36;
  spYeti8.numBbox = 1;
  spYeti8.box[0].x = spYeti1.box[0].x;
  spYeti8.box[0].y = spYeti1.box[0].y;
  spYeti8.box[0].w = spYeti1.box[0].w;
  spYeti8.box[0].h = spYeti1.box[0].h;

  spPenguin1.spData = penguin1;
  spPenguin1.w = 15;
  spPenguin1.h = 18;
  spPenguin1.numBbox = 1;
  spPenguin1.box[0].x = 3;
  spPenguin1.box[0].y = 3;
  spPenguin1.box[0].w = 10;
  spPenguin1.box[0].h = 12;

  spPenguin2.spData = penguin2;
  spPenguin2.w = 15;
  spPenguin2.h = 18;
  spPenguin2.numBbox = 1;
  spPenguin2.box[0].x = spPenguin1.box[0].x;
  spPenguin2.box[0].y = spPenguin1.box[0].y;
  spPenguin2.box[0].w = spPenguin1.box[0].w;
  spPenguin2.box[0].h = spPenguin1.box[0].h;

  spPenguin3.spData = penguin3;
  spPenguin3.w = 15;
  spPenguin3.h = 18;
  spPenguin3.numBbox = 1;
  spPenguin3.box[0].x = spPenguin1.box[0].x;
  spPenguin3.box[0].y = spPenguin1.box[0].y;
  spPenguin3.box[0].w = spPenguin1.box[0].w;
  spPenguin3.box[0].h = spPenguin1.box[0].h;

  spPenguin4.spData = penguin4;
  spPenguin4.w = 15;
  spPenguin4.h = 18;
  spPenguin4.numBbox = 1;
  spPenguin4.box[0].x = spPenguin1.box[0].x;
  spPenguin4.box[0].y = spPenguin1.box[0].y;
  spPenguin4.box[0].w = spPenguin1.box[0].w;
  spPenguin4.box[0].h = spPenguin1.box[0].h;
}

// Finds first available spot in the EntityList[] and returns a pointer to that Entity
// Returns NULL if the EntityList is full, this happens if MAXENTS is reached
Entity* spawnEntity()
{
  for(int i = 0; i < MAXENTS; i++)
  {
    if(EntityList[i].used == false)
    {
      EntityList[i].used = true;
      return &EntityList[i];
    }
  }
  return NULL;
}

// Calls the think function that is set for each active entity in the EntityList[]
void thinkAll()
{
  for(int i = 0; i < MAXENTS; i++)
  {
    if(EntityList[i].used && EntityList[i].think != NULL)
    {
      EntityList[i].think(&EntityList[i]);
    }
  }
}

// Frees (removes) specific entity, sets pointers to NULL
void freeEntity(Entity *ent)
{
  ent->used = false;
  ent->think = NULL;

  for(int i = 0; i < MAXFRAMES; i++)
  {
    ent->sprite[i] = NULL;
  }
}

// Frees all entities in the EntityList[]
void clearEntities()
{
  for(int i = 0; i < MAXENTS; i++)
  {
    freeEntity(&EntityList[i]);
  }
}

// Draws the current sprite frame for each active entity in the EntityList[]
void drawAll()
{
  for(int i = 0; i < MAXENTS; i++)
  {
    if(EntityList[i].used)
    {
      if(EntityList[i].sprite[EntityList[i].frame] != NULL)
      {
        mp.display.drawIcon(EntityList[i].sprite[EntityList[i].frame]->spData,
                            EntityList[i].x - cameraX, EntityList[i].y - cameraY,
                            EntityList[i].sprite[EntityList[i].frame]->w,
                            EntityList[i].sprite[EntityList[i].frame]->h,
                            1, TFT_TRANSPARENT);

        // Draw collision boxes (debug mode)
        if(debugMode)
        {
          for(int j = 0; j < EntityList[i].sprite[EntityList[i].frame]->numBbox; j++)
          {
            mp.display.drawRect(EntityList[i].x - cameraX + EntityList[i].sprite[EntityList[i].frame]->box[j].x,
                                EntityList[i].y - cameraY + EntityList[i].sprite[EntityList[i].frame]->box[j].y,
                                EntityList[i].sprite[EntityList[i].frame]->box[j].w,
                                EntityList[i].sprite[EntityList[i].frame]->box[j].h, TFT_MAGENTA);
          }
        }
      }
    }
  }
}

// Initializes the camera position
void initCamera()
{
  cameraX = 0;
  cameraY = 0;
}

// Centers the camera position on the player
void updateCamera()
{
  if(!player->used) return;

  cameraX = player->x - CAMERAXOFFSET;
  if(cameraX < 0) cameraX = 0;
  if(cameraX > (MAPWIDTH - LCDWIDTH)) cameraX = MAPWIDTH - LCDWIDTH;

  cameraY = player->y - CAMERAYOFFSET;
  if(cameraY < 0) cameraY = 0;
  if(cameraY > (MAPHEIGHT - LCDHEIGHT)) cameraY = MAPHEIGHT - LCDHEIGHT;
}

// Returns true if any bounding boxes of the two entities are colliding
bool collide(Entity *ent1, Entity *ent2)
{
  for(int i = 0; i < ent1->sprite[ent1->frame]->numBbox; i++)
  {
    for(int j = 0; j < ent2->sprite[ent2->frame]->numBbox; j++)
    {
      if((ent1->x + ent1->sprite[ent1->frame]->box[i].x + ent1->sprite[ent1->frame]->box[i].w >= ent2->x + ent2->sprite[ent2->frame]->box[j].x) &&
         (ent1->x + ent1->sprite[ent1->frame]->box[i].x <= ent2->x + ent2->sprite[ent2->frame]->box[j].x + ent2->sprite[ent2->frame]->box[j].w) &&
         (ent1->y + ent1->sprite[ent1->frame]->box[i].y + ent1->sprite[ent1->frame]->box[i].h >= ent2->y + ent2->sprite[ent2->frame]->box[j].y) &&
         (ent1->y + ent1->sprite[ent1->frame]->box[i].y <= ent2->y + ent2->sprite[ent2->frame]->box[j].y + ent2->sprite[ent2->frame]->box[j].h))
         {
           return true;
         }
    }
  }

  return false;
}

// ######################################################
//    Player Functions
// ######################################################

Entity* spawnPlayer(int x, int y)
{
  Entity *self = spawnEntity();
  if(self == NULL) return NULL;

  self->think = playerThink;
  self->x = x;
  self->y = y;
  self->type = PLAYER;
  self->xspeed = 0;
  self->yspeed = 0;
  self->sprite[0] = &spPlayerNormal;
  self->sprite[1] = &spPlayerLeft;
  self->sprite[2] = &spPlayerRight;
  self->sprite[3] = &spPlayerLeftSlide;
  self->sprite[4] = &spPlayerRightSlide;
  self->sprite[5] = &spPlayerJump;
  self->sprite[6] = &spPlayerSpeed;
  self->sprite[7] = &spPlayerCrash;
  self->frame = 0;
  self->flag[JUMP] = 0;
  self->flag[GAMEOVER] = GAMEOVERTIMER;
  self->flag[CRASH] = 0;
  self->flag[INVUL] = 0;

  return self;
}

void playerThink(Entity *self)
{
  // Always ski downwards at yspeed
  self->y += self->yspeed;

  if(self->yspeed == 4) // Render frame 4 (playerSpeed) if speeding
  {
    self->frame = 6;
  }
  else // By default render frame 0 (playerNormal)
  {
    self->frame = 0;
  }

  // Joystick Right - Turn Right
  if(mp.buttons.repeat(BTN_RIGHT, 1) && !self->flag[JUMP])
  {
    if(self->xspeed == 4)
    {
      self->frame = 4;
    }
    else
    {
      self->frame = 2;
    }
    self->x += self->xspeed;
  }

  // Joystick Left - Turn Left
  if(mp.buttons.repeat(BTN_LEFT, 1) && !self->flag[JUMP])
  {
    if(self->xspeed == 4)
    {
      self->frame = 3;
    }
    else
    {
      self->frame = 1;
    }
    self->x -= self->xspeed;
  }

  // Button B - Jump
  if(mp.buttons.pressed(BTN_B) && !self->flag[JUMP])
  {
    playSound(sndJump);
    self->flag[JUMP] = JUMPDURATION;
  }

  // Button A - Speed / Power Slide
  if(mp.buttons.pressed(BTN_A))
  {
    playSound(sndSlide);
  }
  if(mp.buttons.repeat(BTN_A, 1) && !self->flag[JUMP])
  {
    self->xspeed = 4;
    self->yspeed = 4;
  }
  else
  {
    self->xspeed = 2;
    self->yspeed = 3;
  }

  // Jumping logic
  if(self->flag[JUMP])
  {
    self->flag[JUMP]--;
    self->frame = 5;
  }
  else
  {
    self->flag[JUMP] = 0;
  }

  // Keep player within map bounds
  if(self->x < XLIMITL)
  {
    self->x = XLIMITL;
  }

  if(self->x > XLIMITR)
  {
    self->x = XLIMITR;
  }

  // Switch to crash think function if player has collided
  if(self->flag[CRASH] > 0)
  {
    self->think = crashThink;
  }

  if(self->flag[INVUL] > 0)
  {
    self->flag[INVUL]--;
  }

  // Refresh obstacles and warp player back to top
  if(self->y > SCROLLPOS)
  {
    clearObstacles();
    int tmpY = self->y;
    self->y = YSTARTPOS;
    if(yeti->flag[PURSUIT])
    {
      yeti->y = self->y - abs(tmpY - yeti->y);
    }
    spawnObstacles();
  }

  // Update meters traveled
  metersTraveled += self->yspeed;

  // Function buttons pause game
  if(mp.buttons.released(BTN_FUN_LEFT) || mp.buttons.released(BTN_FUN_RIGHT))
  {
    activeLoop = PAUSELOOP;
  }

  // Btn 0 toggles debug mode
  if(mp.buttons.released(BTN_0))
  {
    debugMode = 1 - debugMode;
  }
}

void crashThink(Entity *self)
{
  self->frame = 7;
  self->flag[INVUL] = INVULTIMER;

  self->flag[CRASH]--;

  if(self->flag[CRASH] <= 0)
  {
    self->flag[CRASH] = 0;
    self->flag[JUMP] = 0;
    self->think = playerThink;
  }

  // Function buttons pause game
  if(mp.buttons.released(BTN_FUN_LEFT) || mp.buttons.released(BTN_FUN_RIGHT))
  {
    activeLoop = PAUSELOOP;
  }

  // Btn 0 toggles debug mode
  if(mp.buttons.released(BTN_0))
  {
    debugMode = 1 - debugMode;
  }
}

void gameOverThink(Entity *self)
{
  self->frame = 7;

  self->flag[GAMEOVER]--;

  if(self->flag[GAMEOVER] <= 0)
  {
    startGameOver();
  }

  // Function buttons pause game
  if(mp.buttons.released(BTN_FUN_LEFT) || mp.buttons.released(BTN_FUN_RIGHT))
  {
    activeLoop = PAUSELOOP;
  }

  // Btn 0 toggles debug mode
  if(mp.buttons.released(BTN_0))
  {
    debugMode = 1 - debugMode;
  }
}

// ######################################################
//    Obstacle Functions
// ######################################################

// Spawns random obstacles
void spawnObstacles()
{
  for(int i = 0; i < NUMLOGS; i++)
  {
    spawnLog(random(MAPWIDTH), random(LCDHEIGHT, MAPHEIGHT - LCDHEIGHT - 48), random(4) + 1);
  }

  for(int i = 0; i < NUMPENGUINS; i++)
  {
    spawnPenguin(random(MAPWIDTH), random(LCDHEIGHT, MAPHEIGHT - LCDHEIGHT - 48), random(2) + 1);
  }

  for(int i = 0; i < NUMTREES; i++)
  {
    spawnTree(random(MAPWIDTH), random(LCDHEIGHT, MAPHEIGHT - LCDHEIGHT - 48), random(9) + 1);
  }
}

// Frees all obstacles in the EntityList[]
void clearObstacles()
{
  for(int i = 0; i < MAXENTS; i++)
  {
    if(EntityList[i].type == OBSTACLE)
    {
      freeEntity(&EntityList[i]);
    }
  }
}

// Trees
Entity* spawnTree(int16_t x, int16_t y, byte type)
{
  Entity *self = spawnEntity();
  if(self == NULL) return NULL;

  self->think = treeThink;
  self->x = x;
  self->y = y;
  self->type = OBSTACLE;
  self->frame = 0;

  switch(type)
  {
    case 1: self->sprite[0] = &spTree1;
      break;
    case 2: self->sprite[0] = &spTree2;
      break;
    case 3: self->sprite[0] = &spTree3;
      break;
    case 4: self->sprite[0] = &spTree4;
      break;
    case 5: self->sprite[0] = &spTree5;
      break;
    case 6: self->sprite[0] = &spTree6;
      break;
    case 7: self->sprite[0] = &spTree7;
      break;
    case 8: self->sprite[0] = &spTree8;
      break;
    case 9: self->sprite[0] = &spTree9;
      break;
  }

  return self;
}

void treeThink(Entity *self)
{
  // Collision with player
  if(!player->flag[INVUL] && collide(self, player))
  {
    for (uint8_t i = 0; i < 8; i++)
    {
      mp.leds[i] = CRGB(255,0,0);
    }

    playSound(sndCrash);
    player->flag[CRASH] = CRASHTIMER;
  }
}

// Logs
Entity* spawnLog(int16_t x, int16_t y, byte type)
{
  Entity *self = spawnEntity();
  if(self == NULL) return NULL;

  self->think = logThink;
  self->x = x;
  self->y = y;
  self->type = OBSTACLE;
  self->frame = 0;

  switch(type)
  {
    case 1: self->sprite[0] = &spLog1;
      break;
    case 2: self->sprite[0] = &spLog2;
      break;
    case 3: self->sprite[0] = &spLog3;
      break;
    case 4: self->sprite[0] = &spLog4;
      break;
  }

  return self;
}

void logThink(Entity *self)
{
  // Ignore player if jumping
  if(player->flag[JUMP]) return;

  // Collision with player
  if(!player->flag[INVUL] && collide(self, player))
  {
    for (uint8_t i = 0; i < 8; i++)
    {
      mp.leds[i] = CRGB(255,0,0);
    }

    playSound(sndCrash);
    player->flag[CRASH] = CRASHTIMER;
  }
}

// ######################################################
//    Yeti Functions
// ######################################################

Entity* spawnYeti()
{
  Entity *self = spawnEntity();
  if(self == NULL) return NULL;

  self->think = yetiWaitThink;
  self->x = 9999;
  self->y = 9999;
  self->xspeed = 3;
  self->yspeed = 3;
  self->type = YETI;
  self->sprite[0] = &spYeti1;
  self->sprite[1] = &spYeti2;
  self->sprite[2] = &spYeti3;
  self->sprite[3] = &spYeti4;
  self->sprite[4] = &spYeti5;
  self->sprite[5] = &spYeti6;
  self->sprite[6] = &spYeti7;
  self->sprite[7] = &spYeti8;
  self->frame = 0;
  self->flag[PURSUIT] = 0;
  self->flag[YETIDASH] = 0;
  self->flag[YETIANIMATEFWD] = 1;

  return self;
}

void yetiWaitThink(Entity *self)
{
  // Remain off the map while waiting
  self->x = 9999;
  self->y = 9999;

  // Start chasing player
  if(metersTraveled >= 1000)
  {
    self->x = player->x;
    self->y = player->y - LCDHEIGHT;
    self->flag[PURSUIT] = 1;
    self->think = yetiPursuitThink;
  }
}

void yetiPursuitThink(Entity *self)
{
  // Chase left/right
  if(player->x >= (self->x + 8))
  {
    self->x += self->xspeed;
  }
  if(player->x < (self->x + 8))
  {
    self->x -= self->xspeed;
  }

  // Chase up/down
  if(player->y >= (self->y + 8))
  {
    self->y += self->yspeed;
  }
  if(player->y < (self->y + 8))
  {
    self->y -= self->yspeed;
  }

  // Don't let yeti fall too far behind
  if(abs(player->y - self->y) > 200)
  {
    self->y = player->y - 150;
  }

  // Yeti will dash once in awhile to make game harder
  self->flag[YETIDASH]++;

  if(self->flag[YETIDASH] >= 100)
  {
    self->flag[YETIDASH] = 0;
  }

  if(self->flag[YETIDASH] <= YETIDASHDURATION)
  {
    self->xspeed = 4;
    self->yspeed = 4;
  }
  else
  {
    self->xspeed = 3;
    self->yspeed = 3;
  }

  // Animation

  // Animate Forward
  if(self->flag[YETIANIMATEFWD])
  {
    self->frame++;

    if(self->frame >= 3)
    {
      self->frame = 3;
      self->flag[YETIANIMATEFWD] = 0;
    }
  }
  else // Animate Backwards
  {
    self->frame--;

    if(self->frame <= 0)
    {
      self->frame = 0;
      self->flag[YETIANIMATEFWD] = 1;
    }
  }

  // Caught player
  if(collide(self, player))
  {
    for (uint8_t i = 0; i < 8; i++)
    {
      mp.leds[i] = CRGB(255,0,0);
    }

    playSound(sndGameOver);
    player->think = gameOverThink;
    self->frame = 4;
    self->think = yetiEatThink;
  }
}

void yetiEatThink(Entity *self)
{
  if(self->frame < 7)
  {
    self->frame++;
  }
}

// ######################################################
//    Penguin Functions
// ######################################################

Entity* spawnPenguin(int16_t x, int16_t y, byte type)
{
  Entity *self = spawnEntity();
  if(self == NULL) return NULL;

  self->x = x;
  self->y = y;
  self->xspeed = 1;
  self->type = OBSTACLE;
  self->frame = 0;

  switch(type)
  {
    // Walks right to left
    case 1: self->sprite[0] = &spPenguin1;
            self->sprite[1] = &spPenguin2;
            self->think = penguinThink1;
      break;
    // Walks left to right
    case 2: self->sprite[0] = &spPenguin3;
            self->sprite[1] = &spPenguin4;
            self->think = penguinThink2;
      break;
  }

  return self;
}

// Walks right to left
void penguinThink1(Entity *self)
{
  self->x -= self->xspeed;

  // Warp to other side if past map edge
  if(self->x < 0)
  {
    self->x = MAPWIDTH;
  }

  // Animate penguin
  if(self->x % 5 == 0)
  {
    self->frame = 1 - self->frame;
  }

  // Collision with player
  if(!player->flag[INVUL] && collide(self, player))
  {
    for (uint8_t i = 0; i < 8; i++)
    {
      mp.leds[i] = CRGB(255,0,0);
    }

    playSound(sndCrash);
    player->flag[CRASH] = CRASHTIMER;
  }
}

// Walks left to right
void penguinThink2(Entity *self)
{
  self->x += self->xspeed;

  // Warp to other side if past map edge
  if(self->x > MAPWIDTH)
  {
    self->x = 0;
  }

  // Animate penguin
  if(self->x % 5 == 0)
  {
    self->frame = 1 - self->frame;
  }

  // Collision with player
  if(!player->flag[INVUL] && collide(self, player))
  {
    for (uint8_t i = 0; i < 8; i++)
    {
      mp.leds[i] = CRGB(255,0,0);
    }

    playSound(sndCrash);
    player->flag[CRASH] = CRASHTIMER;
  }
}
