#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GL/glut.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;
using namespace chrono;

// Constants
const int columns = 40;
const int rows = 40;
const int frame_rate = 30;
int snakeSpeed = 10;
float snakeWidth, snakeHeight;
bool isGameOver = false;
int playerScore = 0;

// Snake Properties
struct SnakeSegment
{
  float x, y;
};
vector<SnakeSegment> snakeBody = {{0.0f, 0.0f}};
enum Direction
{
  NONE,
  LEFT,
  RIGHT,
  UP,
  DOWN
};
Direction snakeDirection = NONE;

// Fruit Position
float fruitX, fruitY;

// Random Generator for Fruit
float getRandomCoord()
{
  static random_device rd;
  static mt19937 gen(rd());
  uniform_real_distribution<float> dis(-1.0f + snakeWidth / 2, 1.0f - snakeWidth / 2);
  return dis(gen);
}

void placeFruit()
{
  fruitX = getRandomCoord();
  fruitY = getRandomCoord();
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
}

// Check Collisions
void checkCollisions()
{
  // Check self-collision
  for (size_t i = 1; i < snakeBody.size(); ++i)
  {
    if (snakeBody[0].x == snakeBody[i].x && snakeBody[0].y == snakeBody[i].y)
    {
      isGameOver = true;
      return;
    }
  }

  // Check fruit collision
  if (abs(snakeBody[0].x - fruitX) < snakeWidth && abs(snakeBody[0].y - fruitY) < snakeHeight)
  {
    playerScore += 10;
    placeFruit();
    snakeBody.push_back({snakeBody.back().x, snakeBody.back().y});
  }
}

// Shader Source
const char *vertexShaderSource = R"(
    attribute vec2 aPosition;
    void main() {
        gl_Position = vec4(aPosition, 0.0, 1.0);
    }
)";

const char *fragmentShaderSource = R"(
    uniform vec3 uColor;
    void main() {
        gl_FragColor = vec4(uColor, 1.0);
    }
)";

GLuint compileShader(GLenum type, const char *source)
{
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);

  GLint compiled;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled)
  {
    char log[512];
    glGetShaderInfoLog(shader, 512, nullptr, log);
    cerr << "Shader compilation failed: " << log << endl;
    exit(EXIT_FAILURE);
  }

  return shader;
}

GLuint createProgram()
{
  GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
  GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

  GLuint program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  GLint linked;
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  if (!linked)
  {
    char log[512];
    glGetProgramInfoLog(program, 512, nullptr, log);
    cerr << "Program linking failed: " << log << endl;
    exit(EXIT_FAILURE);
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  return program;
}

void drawSquare(GLuint program, float x, float y, float size, float r, float g, float b)
{
  float vertices[] = {
      x - size / 2, y - size / 2,
      x + size / 2, y - size / 2,
      x + size / 2, y + size / 2,
      x - size / 2, y + size / 2};

  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glUseProgram(program);

  GLint posLoc = glGetAttribLocation(program, "aPosition");
  glEnableVertexAttribArray(posLoc);
  glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);

  GLint colorLoc = glGetUniformLocation(program, "uColor");
  glUniform3f(colorLoc, r, g, b);

  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  glDisableVertexAttribArray(posLoc);
  glDeleteBuffers(1, &vbo);
}

GLuint loadTexture(const char *filePath)
{
  // Load texture using an image loading library (e.g., stb_image)
  int width, height, channels;
  unsigned char *image = stbi_load(filePath, &width, &height, &channels, 4);
  if (!image)
  {
    cerr << "Failed to load texture: " << filePath << endl;
    exit(EXIT_FAILURE);
  }

  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  stbi_image_free(image);
  return texture;
}

void renderText(GLuint program, GLuint texture, const string &text, float x, float y, float scale, float r, float g, float b)
{
  glUseProgram(program);
  glBindTexture(GL_TEXTURE_2D, texture);

  float charWidth = 1.0f / 16.0f; // Assuming 16x16 grid of characters
  float charHeight = 1.0f / 16.0f;

  for (char c : text)
  {
    int charIndex = c - 32; // Assuming the texture starts with ASCII 32 (space)
    int col = charIndex % 16;
    int row = charIndex / 16;

    float tx1 = col * charWidth;
    float ty1 = row * charHeight;
    float tx2 = tx1 + charWidth;
    float ty2 = ty1 + charHeight;

    float vertices[] = {
        x, y, tx1, ty1,
        x + scale, y, tx2, ty1,
        x + scale, y + scale, tx2, ty2,
        x, y + scale, tx1, ty2};

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLint posLoc = glGetAttribLocation(program, "aPosition");
    GLint texLoc = glGetAttribLocation(program, "aTexCoord");

    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(texLoc);
    glVertexAttribPointer(texLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));

    GLint colorLoc = glGetUniformLocation(program, "uColor");
    glUniform3f(colorLoc, r, g, b);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableVertexAttribArray(posLoc);
    glDisableVertexAttribArray(texLoc);
    glDeleteBuffers(1, &vbo);

    x += scale; // Move to next character position
  }
}

// GLUT callbacks
GLuint program;

void display()
{
  glClear(GL_COLOR_BUFFER_BIT);

  drawSquare(program, fruitX, fruitY, snakeWidth, 1.0f, 1.0f, 0.0f);
  for (const auto &segment : snakeBody)
  {
    drawSquare(program, segment.x, segment.y, snakeWidth, 1.0f, 1.0f, 1.0f);
  }

  glutSwapBuffers();
}

void keyboard(int key, int, int)
{
  if (key == GLUT_KEY_LEFT && snakeDirection != RIGHT)
    snakeDirection = LEFT;
  if (key == GLUT_KEY_RIGHT && snakeDirection != LEFT)
    snakeDirection = RIGHT;
  if (key == GLUT_KEY_UP && snakeDirection != DOWN)
    snakeDirection = UP;
  if (key == GLUT_KEY_DOWN && snakeDirection != UP)
    snakeDirection = DOWN;
}

std::chrono::steady_clock::time_point lastMoveTime = std::chrono::steady_clock::now();
const std::chrono::milliseconds moveInterval((int)(100 / (0.1f * snakeSpeed)));

void update(int)
{
  auto currentTime = std::chrono::steady_clock::now();
  bool movementIntervalMet = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastMoveTime) >= moveInterval;
  if (movementIntervalMet)
  {
    moveSnake();
    checkCollisions();
    lastMoveTime = currentTime;
  }
  glutPostRedisplay();
  glutTimerFunc(1000 / frame_rate, update, 0);
}

int main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize(800, 800);
  glutCreateWindow("Snake Game");

  program = createProgram();
  placeFruit();
  snakeWidth = 2.0f / columns;
  snakeHeight = 2.0f / rows;

  glutDisplayFunc(display);
  glutSpecialFunc(keyboard);
  glutTimerFunc(1000 / frame_rate, update, 0);

  glutMainLoop();
  return 0;
}
