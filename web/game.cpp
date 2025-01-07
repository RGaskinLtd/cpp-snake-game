#include <math.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>
#include <random>
#include <GL/glew.h>
#include <GL/glut.h>
#include <emscripten.h>

using namespace std;
using namespace chrono;

// Configurations
const int columns = 40;
const int rows = 40;
const float width = 1200.0f;
const float height = 1200.0f;
const int frame_rate = 30;
int snakeSpeed = 10;
const float widthPxVal = 1.0f / (width / 2);
const float heightPxVal = 1.0f / (height / 2);

// Snake Properties
struct SnakeSegment
{
  float x, y;
};
vector<SnakeSegment> snakeBody = {{0.0f, 0.0f}};
float snakeWidth = widthPxVal * (width / columns);
float snakeHeight = heightPxVal * (height / rows);
enum Direction
{
  NONE,
  LEFT,
  RIGHT,
  UP,
  DOWN
};
Direction snakeDirection = NONE;

// Game State
bool isGameOver = false;
int playerScore = 0;
float fruitX, fruitY;

// Random Number Generator
float getRandomCord()
{
  static random_device rd;
  static mt19937 gen(rd());
  uniform_real_distribution<float> dis(-1.0f + snakeWidth / 2, 1.0f - snakeWidth / 2);
  return dis(gen);
}

void placeFruit()
{
  fruitX = getRandomCord();
  fruitY = getRandomCord();
}

// Movement Logic
void moveSnake()
{
  if (snakeDirection == NONE || isGameOver)
    return;

  float prevX = snakeBody[0].x;
  float prevY = snakeBody[0].y;

  // Move head
  switch (snakeDirection)
  {
  case LEFT:
    snakeBody[0].x -= snakeWidth;
    break;
  case RIGHT:
    snakeBody[0].x += snakeWidth;
    break;
  case UP:
    snakeBody[0].y += snakeHeight;
    break;
  case DOWN:
    snakeBody[0].y -= snakeHeight;
    break;
  default:
    break;
  }

  // Wrap around
  if (snakeBody[0].x < -1.0f)
    snakeBody[0].x = 1.0f;
  if (snakeBody[0].x > 1.0f)
    snakeBody[0].x = -1.0f;
  if (snakeBody[0].y < -1.0f)
    snakeBody[0].y = 1.0f;
  if (snakeBody[0].y > 1.0f)
    snakeBody[0].y = -1.0f;

  // Move body
  for (size_t i = 1; i < snakeBody.size(); ++i)
  {
    float tempX = snakeBody[i].x;
    float tempY = snakeBody[i].y;
    snakeBody[i].x = prevX;
    snakeBody[i].y = prevY;
    prevX = tempX;
    prevY = tempY;
  }

  snakeSpeed = 10 + (snakeBody.size() / 4);
}

// Collision Logic
void checkCollisions()
{
  // Check collision with itself
  for (size_t i = 1; i < snakeBody.size(); ++i)
  {
    if (snakeBody[0].x == snakeBody[i].x && snakeBody[0].y == snakeBody[i].y)
    {
      isGameOver = true;
      return;
    }
  }

  // Check collision with fruit
  if (abs(snakeBody[0].x - fruitX) < snakeWidth && abs(snakeBody[0].y - fruitY) < snakeHeight)
  {
    playerScore += 10;
    placeFruit();
    snakeBody.push_back({snakeBody.back().x, snakeBody.back().y});
  }
}

// Rendering Functions
void drawSquare(float x, float y, float width, float height, float r, float g, float b)
{
  glColor3f(r, g, b);
  glBegin(GL_QUADS);
  glVertex2f(x, y);
  glVertex2f(x + width, y);
  glVertex2f(x + width, y + height);
  glVertex2f(x, y + height);
  glEnd();
}

void drawSnake()
{
  for (const auto &segment : snakeBody)
  {
    drawSquare(segment.x, segment.y, snakeWidth, snakeHeight, 1.0f, 1.0f, 1.0f);
  }
}

void drawFruit()
{
  drawSquare(fruitX, fruitY, snakeWidth, snakeHeight, 1.0f, 1.0f, 0.0f);
}

void renderSpacedBitmapString(float x, float y, void *font, const string &text)
{
  float x1 = x;
  for (char c : text)
  {
    glRasterPos2f(x1, y);
    glutBitmapCharacter(font, c);
    float widthOfChar = (float)(widthPxVal * glutBitmapWidth(font, c));
    x1 = x1 + widthOfChar;
  }
}

float getWidthOfSpacedBitmapString(float x, float y, void *font, const string &text)
{
  float x1 = x;
  float widthOfText = 0.0f;
  for (char c : text)
  {
    float widthOfChar = (float)(widthPxVal * glutBitmapWidth(font, c)) / 2;
    widthOfText = widthOfText + widthOfChar;
    x1 = x1 + widthOfChar;
  }
  return widthOfText;
}

void drawText(float x, float y, bool centerText, void *font, const string &text)
{
  const float x1 = centerText ? (getWidthOfSpacedBitmapString(x, y, font, text)) * -1.0f : x;
  renderSpacedBitmapString(x1, y, font, text);
}

void render()
{
  glClear(GL_COLOR_BUFFER_BIT);

  if (isGameOver)
  {
    glColor3f(0.5f, 1.0f, 0.0f);
    drawText(0.0f, 0.2f, true, GLUT_BITMAP_HELVETICA_18, "Game Over!");
    drawText(0.0f, 0.1f, true, GLUT_BITMAP_HELVETICA_18, "Score: " + to_string(playerScore));
    drawText(0.0f, 0.0f, true, GLUT_BITMAP_HELVETICA_18, "Press 'Space' to restart");
  }
  else
  {
    drawFruit();
    drawSnake();
    drawText(-0.9f, 0.9f, false, GLUT_BITMAP_HELVETICA_18, "Score: " + to_string(playerScore));
  }

  glutSwapBuffers();
}

std::chrono::steady_clock::time_point lastMoveTime = std::chrono::steady_clock::now();
const std::chrono::milliseconds moveInterval((int)(100 / (0.1f * snakeSpeed)));

void timer(int)
{
  auto currentTime = std::chrono::steady_clock::now();
  bool movementIntervalMet = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastMoveTime) >= moveInterval;
  if (!isGameOver && movementIntervalMet)
  {
    moveSnake();
    checkCollisions();
    lastMoveTime = currentTime;
  }
  glutPostRedisplay();
  glutTimerFunc(1000 / frame_rate, timer, 0);
}

void handleKeypress(unsigned char key, int, int)
{
  switch (key)
  {
  case 'a':
    if (snakeDirection != RIGHT)
      snakeDirection = LEFT;
    break;
  case 'd':
    if (snakeDirection != LEFT)
      snakeDirection = RIGHT;
    break;
  case 'w':
    if (snakeDirection != DOWN)
      snakeDirection = UP;
    break;
  case 's':
    if (snakeDirection != UP)
      snakeDirection = DOWN;
    break;
  case ' ':
    if (isGameOver)
    {
      snakeBody = {{0.0f, 0.0f}};
      snakeDirection = NONE;
      playerScore = 0;
      isGameOver = false;
      placeFruit();
      break;
    }
    snakeDirection = NONE;
    break;
  }
}
void handleSpecialKeypress(int key, int, int)
{
  switch (key)
  {
  case GLUT_KEY_LEFT:
    if (snakeDirection != RIGHT)
      snakeDirection = LEFT;
    break;
  case GLUT_KEY_RIGHT:
    if (snakeDirection != LEFT)
      snakeDirection = RIGHT;
    break;
  case GLUT_KEY_UP:
    if (snakeDirection != DOWN)
      snakeDirection = UP;
    break;
  case GLUT_KEY_DOWN:
    if (snakeDirection != UP)
      snakeDirection = DOWN;
    break;
  }
}

int main(int argc, char *argv[])
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize(width, height);
  glutCreateWindow("Snake Game");

  glewInit();

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

  placeFruit();

  glutDisplayFunc(render);
  glutKeyboardFunc(handleKeypress);
  glutSpecialFunc(handleSpecialKeypress);
  glutTimerFunc(0, timer, 0);
  glutMainLoop();

  return 0;
}
