/**
 * From the OpenGL Programming wikibook: http://en.wikibooks.org/wiki/OpenGL_Programming
 * This file is in the public domain.
 * Contributors: Sylvain Beucler
 */

#include <math.h>
#include <iostream>
#include <unordered_map>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "utils/utils.cpp"
#include "config.cpp"

using namespace std;

Config config;

const float columns = 40.0f;
const float rows = 40.0f;

const float width = 1200.0f;
const float height = 1200.0f;

int frame_rate = 15;

const float widthPxVal = 1.0f / (width / 2);
const float heightPxVal = 1.0f / (height / 2);

bool isGameOver = false;
int playerScore = 0;

float snake_xPos[(int)width], snake_yPos[(int)height];
float snake_head_xPos = 0.0f, snake_head_yPos = 0.0f;
float snake_width = widthPxVal * (width / columns);
float snake_height = heightPxVal * (height / rows);
int snake_length = 1;

float snake_speed = snake_width / 3;
string snake_direction = "space";

float getRandomCords() {
  float randFloat = ((double) rand() / (RAND_MAX)) * 2 - 1;
  if (randFloat >= (1.0f - snake_width)) randFloat = 1.0f - (snake_width + (snake_width / 2));
  if (randFloat <= (-1.0f + snake_width)) randFloat = -1.0f + (snake_width / 2);
  return randFloat;
}

float fruitCordX = getRandomCords();
float fruitCordY = getRandomCords();

float pCords[2];
float * prevCords(float x, float y) {
  if (snake_direction == "left") {
    pCords[0] = x; // + snake_speed;
    pCords[1] = y;
  }
  if (snake_direction == "right") {
    pCords[0] = x; // - snake_speed;
    pCords[1] = y;
  }
  if (snake_direction == "up") {
    pCords[0] = x; //;
    pCords[1] = y; // - snake_speed;
  }
  if (snake_direction == "down") {
    pCords[0] = x; //;
    pCords[1] = y; // + snake_speed;
  }
  return pCords;
}

float nCord;
float nextCord(float cord) {
  if (snake_direction == "left") nCord = cord - widthPxVal;
  if (snake_direction == "right") nCord = cord + widthPxVal;
  if (snake_direction == "up") nCord = cord + heightPxVal;
  if (snake_direction == "down") nCord = cord - heightPxVal;
  return nCord;
}

void moveSnake() {
  if (snake_direction == "space" || isGameOver) return;
  float prevX = prevCords(snake_xPos[0], snake_yPos[0])[0];
  float prevY = prevCords(snake_xPos[0], snake_yPos[0])[1];
  float prev2X, prev2Y;

  snake_xPos[0] = snake_head_xPos;
  snake_yPos[0] = snake_head_yPos;
  for (int i = 1; i < snake_length; i++) {
    prev2X = prevCords(snake_xPos[i], snake_yPos[i])[0];
    prev2Y = prevCords(snake_xPos[i], snake_yPos[i])[1];
    snake_xPos[i] = prevX;
    snake_yPos[i] = prevY;
    prevX = prev2X;
    prevY = prev2Y;
  }

  if (snake_direction == "left") {
    if (snake_head_xPos <= 1.0f * -1) snake_head_xPos = 1.0f;
    snake_head_xPos -= snake_width;
  }
  if (snake_direction == "right") {
    if (snake_head_xPos >= 1.0f) snake_head_xPos = -1.0f;
    snake_head_xPos += snake_width;
  }
  if (snake_direction == "up") {
    if (snake_head_yPos >= 1.0f) snake_head_yPos = -1.0f;
    snake_head_yPos += snake_height;
  }
  if (snake_direction == "down") {
    if (snake_head_yPos <= 1.0f * -1) snake_head_yPos = 1.0f;
    snake_head_yPos -= snake_height;
  }
  // Log Stats
  // cout<<"X Position: "<<snake_xPos[0]<<" Width: "<<snake_width<<endl;
  // cout<<"Y Position: "<<snake_yPos[0]<<" Width: "<<snake_height<<endl;
  // cout<<"FruitCordX: "<<fruitCordX<<endl;
  // cout<<"FruitCordY: "<<fruitCordY<<endl;
  // cout<<"Player Score: "<<playerScore<<endl;
  // cout<<"Game Over: "<<isGameOver<<"\n"<<endl;
}
void ruleCheck() {
  // Checks for collision with the tail (o)
  for (int i = 1; i < snake_length; i++) {
    if (snake_xPos[i] == snake_head_xPos && snake_yPos[i] == snake_head_yPos)
      isGameOver = true;
  }

  // Checks for snake's collision with the food (#)
  bool hitX = ((snake_head_xPos + snake_width) >= fruitCordX && snake_head_xPos <= (fruitCordX + snake_width));
  bool hitY = ((snake_head_yPos + snake_height) >= fruitCordY && snake_head_yPos <= (fruitCordY + snake_height));

  if (hitX && hitY) {
    playerScore += 10;
    fruitCordX = getRandomCords();
    fruitCordY = getRandomCords();
    snake_length += 1;
  }
}

void drawSnake() {
  moveSnake();
  ruleCheck();

  glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 0.0f);
    glVertex2f(fruitCordX, fruitCordY); // top left
    glVertex2f(fruitCordX + snake_width, fruitCordY); // top right
    glVertex2f(fruitCordX + snake_width, fruitCordY + snake_height); // bottom right
    glVertex2f(fruitCordX, fruitCordY + snake_height); //
  glEnd();

  glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(snake_head_xPos, snake_head_yPos); // top left
    glColor3f(1.0f, 1.0f, 0.0f);
    glVertex2f(snake_head_xPos + snake_width, snake_head_yPos); // top right
    glColor3f(1.0f, 0.0f, 1.0f);
    glVertex2f(snake_head_xPos + snake_width, snake_head_yPos + snake_height); // bottom right
    glColor3f(0.0f, 1.0f, 1.0f);
    glVertex2f(snake_head_xPos, snake_head_yPos + snake_height); //
  glEnd();

  for (int i = 0; i < snake_length; i++) {
    glBegin(GL_QUADS);
      glColor3f(1.0f, 1.0f, 1.0f);
      glVertex2f(snake_xPos[i], snake_yPos[i]); // top left
      glColor3f(1.0f, 1.0f, 0.0f);
      glVertex2f(snake_xPos[i] + snake_width, snake_yPos[i]); // top right
      glColor3f(1.0f, 0.0f, 1.0f);
      glVertex2f(snake_xPos[i] + snake_width, snake_yPos[i] + snake_height); // bottom right
      glColor3f(0.0f, 1.0f, 1.0f);
      glVertex2f(snake_xPos[i], snake_yPos[i] + snake_height); //
    glEnd();
  }
}

void renderSpacedBitmapString(float x, float y, void *font, char *string) {
  int i, len;
  float x1 = x;
  for (i = 0, len = strlen(string); i < len; i++) {
    glRasterPos2f(x1, y);
    glutBitmapCharacter(font, (int)string[i]);
    float widthOfChar = (float)(widthPxVal * glutBitmapWidth(font, (int)string[i]));
    x1 = x1 + widthOfChar;
  }
}

float getWidthOfSpacedBitmapString(float x, float y, void *font, char *string) {
  int i, len;
  float x1 = x;
  float widthOfText = 0.0f;
  for (i = 0, len = strlen(string); i < len; i++) {
    float widthOfChar = (float)(widthPxVal * glutBitmapWidth(font, (int)string[i])) / 2;
    widthOfText = widthOfText + widthOfChar;
    x1 = x1 + widthOfChar;
  }
  return widthOfText;
}

void drawText(float x, float y, bool centerText, void *font, char *string) {
  const float x1 = centerText ? (getWidthOfSpacedBitmapString(x, y, font, string)) * -1.0f : x;
  renderSpacedBitmapString(x1, y, font, string);
}

void restartGame() {
  playerScore = 0;
  snake_length = 1;
  snake_head_xPos = 0.0f;
  snake_head_xPos = 0.0f;
  memset(snake_xPos, 0.0f, sizeof(snake_xPos));
  memset(snake_yPos, 0.0f, sizeof(snake_yPos));
  fruitCordX = getRandomCords();
  fruitCordY = getRandomCords();
  isGameOver = false;
  snake_direction = " ";
}

void drawScore(float y) {
  char scoreText[15] = "Score: ";
  const char * scoreAsChars = intToStr(playerScore);
  char * playerScoreText = strcat(scoreText, scoreAsChars);
  drawText(0.0f, y, true, GLUT_BITMAP_HELVETICA_18, playerScoreText);
}

void gameOver() {
  char gameOverText[11] = "Game Over!";
  char scoreText[15] = "Score: ";
  const char * scoreAsChars = intToStr(playerScore);
  char * playerScoreText = strcat(scoreText, scoreAsChars);
  char infoText[29] = "press 'space' to start again";
  glColor3f(0.5f, 1.0f, 0.0f);
  drawText(0.0f, 0.8f, true, GLUT_BITMAP_HELVETICA_18, gameOverText);
  drawText(0.0f, 0.73f, true, GLUT_BITMAP_HELVETICA_18, playerScoreText);
  drawText(0.0f, 0.66f, true, GLUT_BITMAP_HELVETICA_18, infoText);
}

void onDisplay()
{
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glLoadIdentity();

  if (isGameOver) {
    gameOver();
    glFlush();
    if (snake_direction == "space") {
      restartGame();
      glFlush();
    }
  } else {
    drawSnake();
    glFlush();
    drawScore(0.8f);
    glFlush();
  }
  glutSwapBuffers();
}

void timer(int) {
  onDisplay();
  glutTimerFunc(1000/frame_rate, timer, 0);
}

bool isIlligelMove(string direction) {
  if (snake_direction == "left" && direction == "right") return true;
  if (snake_direction == "right" && direction == "left") return true;
  if (snake_direction == "up" && direction == "down") return true;
  if (snake_direction == "down" && direction == "up") return true;
  if (direction != "left" && direction != "right" && direction != "up" && direction != "down" && direction != "space") return true;
  return false;
}

void handleKeypress(unsigned char key, int x, int y) {
  if (isIlligelMove(config.key_mappings.get(key))) return;
  snake_direction = config.key_mappings.get(key);
  onDisplay();
}
void handleSpecialKeypress(int key, int x, int y) {
  if (isIlligelMove(config.skey_mappings.get(key))) return;
  snake_direction = config.skey_mappings.get(key);
  onDisplay();
}

int main(int argc, char* argv[]) {
  glutInit(&argc, argv);
  glutInitContextVersion(2,0);
  glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);
  glutInitWindowSize(width, height);
  glutCreateWindow("Snake");
  timer(0);

  GLenum glew_status = glewInit();
  if (glew_status != GLEW_OK) {
    fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));
    return 1;
  }

  glutDisplayFunc(onDisplay);
  glutKeyboardFunc(handleKeypress);
  glutSpecialFunc(handleSpecialKeypress);
  glutMainLoop();

  return 0;
}
