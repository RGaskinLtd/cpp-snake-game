#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GL/glut.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <string>
#include <emscripten.h>
#include <emscripten/html5.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const char foodSound[18] = "/web/res/food.ogg";
const char moveSound[18] = "/web/res/move.ogg";
const char gameOverSound[22] = "/web/res/gameover.ogg";
const char music[19] = "/web/res/music.ogg";

using namespace std;
using namespace chrono;

const char fontTexturePath[25] = "web/res/font_texture.png";
const char snakeTexturePath[27] = "web/res/snake-graphics.png";
// Constants
const int columns = 60;
const int rows = 60;
const int frame_rate = 30;
int snakeSpeed = 10;
float snakeWidth, snakeHeight;
bool isGameOver = false;
int playerScore = 0;
bool musicPlayed = false;
bool gameOverSoundPlayed = false;

bool enableMusic = false;
bool enableMoveSound = false;
bool enableFoodSound = true;

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

void playAudio(const char *audioFile, bool loop = false, float volume = 1.0f)
{
  EM_ASM(
      {
        var audio = new Audio(UTF8ToString($0)); // Load the audio file
        audio.loop = $1;                         // Loop the audio
        audio.volume = $2;                       // Loop the audio
        audio.play();                            // Play the audio
      },
      audioFile, loop, volume);
}
// Random Generator for Fruit
float getRandomCoord()
{
  static random_device rd;
  static mt19937 gen(rd());
  uniform_real_distribution<float> dis(-1.0f + snakeWidth, 1.0f - snakeWidth);
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
    if (enableFoodSound)
      playAudio(foodSound);
    playerScore += 10;
    placeFruit();
    snakeBody.push_back({snakeBody.back().x, snakeBody.back().y});
  }
}

// Shader Source
const char *vertexShaderSource = R"(
    attribute vec2 aPosition;
    attribute vec2 aTexCoord;
    varying vec2 vTexCoord;

    void main() {
        vTexCoord = aTexCoord;
        gl_Position = vec4(aPosition, 0.0, 1.0);
    }
)";

const char *fragmentShaderSource = R"(
  precision mediump float;
  uniform vec3 uColor;
  uniform sampler2D uTexture;
  uniform bool uUseTexture;
  uniform bool uUseFontTexture;
  varying vec2 vTexCoord;

  void main() {
    if (uUseFontTexture) {
      vec4 texColor = texture2D(uTexture, vTexCoord);
      gl_FragColor = vec4(uColor, 1.0) * texColor;
    } else if(uUseTexture) {
      vec4 texColor = texture2D(uTexture, vTexCoord);
      gl_FragColor = texColor * texture2D(uTexture, vTexCoord).aaaa;
    } else {
      gl_FragColor = vec4(uColor, 1.0);
    }
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

// // Loop over every snake segment
// for (var i = 0; i < snake.segments.length; i++)
// {
//   var segment = snake.segments[i];
//   var segx = segment.x;
//   var segy = segment.y;
//   var tilex = segx * level.tilewidth;
//   var tiley = segy * level.tileheight;

//   // Sprite column and row that gets calculated
//   var tx = 0;
//   var ty = 0;

//   if (i == 0)
//   {
//     // Head; Determine the correct image
//     var nseg = snake.segments[i + 1]; // Next segment
//     if (segy < nseg.y)
//     {
//       // Up
//       tx = 3;
//       ty = 0;
//     }
//     else if (segx > nseg.x)
//     {
//       // Right
//       tx = 4;
//       ty = 0;
//     }
//     else if (segy > nseg.y)
//     {
//       // Down
//       tx = 4;
//       ty = 1;
//     }
//     else if (segx < nseg.x)
//     {
//       // Left
//       tx = 3;
//       ty = 1;
//     }
//   }
//   else if (i == snake.segments.length - 1)
//   {
//     // Tail; Determine the correct image
//     var pseg = snake.segments[i - 1]; // Prev segment
//     if (pseg.y < segy)
//     {
//       // Up
//       tx = 3;
//       ty = 2;
//     }
//     else if (pseg.x > segx)
//     {
//       // Right
//       tx = 4;
//       ty = 2;
//     }
//     else if (pseg.y > segy)
//     {
//       // Down
//       tx = 4;
//       ty = 3;
//     }
//     else if (pseg.x < segx)
//     {
//       // Left
//       tx = 3;
//       ty = 3;
//     }
//   }
//   else
//   {
//     // Body; Determine the correct image
//     var pseg = snake.segments[i - 1]; // Previous segment
//     var nseg = snake.segments[i + 1]; // Next segment
//     if (pseg.x < segx && nseg.x > segx || nseg.x < segx && pseg.x > segx)
//     {
//       // Horizontal Left-Right
//       tx = 1;
//       ty = 0;
//     }
//     else if (pseg.x < segx && nseg.y > segy || nseg.x < segx && pseg.y > segy)
//     {
//       // Angle Left-Down
//       tx = 2;
//       ty = 0;
//     }
//     else if (pseg.y < segy && nseg.y > segy || nseg.y < segy && pseg.y > segy)
//     {
//       // Vertical Up-Down
//       tx = 2;
//       ty = 1;
//     }
//     else if (pseg.y < segy && nseg.x < segx || nseg.y < segy && pseg.x < segx)
//     {
//       // Angle Top-Left
//       tx = 2;
//       ty = 2;
//     }
//     else if (pseg.x > segx && nseg.y < segy || nseg.x > segx && pseg.y < segy)
//     {
//       // Angle Right-Up
//       tx = 0;
//       ty = 1;
//     }
//     else if (pseg.y > segy && nseg.x > segx || nseg.y > segy && pseg.x > segx)
//     {
//       // Angle Down-Right
//       tx = 0;
//       ty = 0;
//     }
//   }

//   // Draw the image of the snake part
//   context.drawImage(tileimage, tx * 64, ty * 64, 64, 64, tilex, tiley,
//                     level.tilewidth, level.tileheight);
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

GLuint loadTexture(const char *filename)
{
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  // Load the image using STB or similar and generate mipmaps
  int width, height, channels;
  unsigned char *image = stbi_load(filename, &width, &height, &channels, 4);
  if (!image)
  {
    cerr << "Failed to load texture: " << filename << endl;
    return 0;
  }

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
  glGenerateMipmap(GL_TEXTURE_2D);

  stbi_image_free(image);
  return texture;
}

void renderText(GLuint program, GLuint texture, const string &text, float x, float y, float scale, float r, float g, float b, bool center = false)
{
  glUseProgram(program);
  glBindTexture(GL_TEXTURE_2D, texture);

  float charWidth = 1.0f / 16.0f; // Assuming 16x16 grid of characters
  float charHeight = 1.0f / 16.0f;

  // Calculate the total width of the text if centering is enabled
  if (center)
  {
    float textWidth = text.size() * scale;
    x -= textWidth / 2.0f; // Adjust the x position to center the text
  }

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
        x, y, tx1, ty2,
        x + scale, y, tx2, ty2,
        x + scale, y + scale, tx2, ty1,
        x, y + scale, tx1, ty1};

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

    x += scale; // Move to the next character position
  }
}

void drawTexturedSquare(GLuint program, GLuint texture)
{
  glUseProgram(program);
  glBindTexture(GL_TEXTURE_2D, texture);
  glDepthMask(GL_TRUE);
  glBlendFunc(GL_TEXTURE_2D, GL_TEXTURE_ALPHA_TYPE);

  float charWidth = 1.0f / 5.0f; // Assuming 16x16 grid of characters
  float charHeight = 1.0f / 4.0f;

  for (size_t i = 0; i < snakeBody.size(); ++i)
  {
    const auto segment = snakeBody[i];
    // drawSquare(program, segment.x, segment.y, snakeWidth, 1.0f, 1.0f, 1.0f);

    float segx = segment.x;
    float segy = segment.y;

    // Sprite column and row that gets calculated
    int tx = 0;
    int ty = 0;

    if (i == 0)
    {
      // Head; Determine the correct image
      auto nseg = snakeBody[i + 1]; // Next segment
      if (segy > nseg.y)
      {
        // Up
        tx = 3;
        ty = 0;
      }
      else if (segx > nseg.x)
      {
        // Right
        tx = 4;
        ty = 0;
      }
      else if (segy < nseg.y)
      {
        // Down
        tx = 4;
        ty = 1;
      }
      else if (segx < nseg.x)
      {
        // Left
        tx = 3;
        ty = 1;
      }
    }
    else if (i == snakeBody.size() - 1)
    {
      // Tail; Determine the correct image
      auto pseg = snakeBody[i - 1]; // Prev segment
      if (pseg.y > segy)
      {
        // Up
        tx = 3;
        ty = 2;
      }
      else if (pseg.x > segx)
      {
        // Right
        tx = 4;
        ty = 2;
      }
      else if (pseg.y < segy)
      {
        // Down
        tx = 4;
        ty = 3;
      }
      else if (pseg.x < segx)
      {
        // Left
        tx = 3;
        ty = 3;
      }
    }
    else
    {
      // Body; Determine the correct image
      auto pseg = snakeBody[i - 1]; // Previous segment
      auto nseg = snakeBody[i + 1]; // Next segment
      if (pseg.x < segx && nseg.x > segx || nseg.x < segx && pseg.x > segx)
      {
        // Horizontal Left-Right
        tx = 1;
        ty = 0;
      }
      else if (pseg.x < segx && nseg.y > segy || nseg.x < segx && pseg.y > segy)
      {
        // Angle Left-Down
        tx = 2;
        ty = 2;
      }
      else if (pseg.y < segy && nseg.y > segy || nseg.y < segy && pseg.y > segy)
      {
        // Vertical Up-Down
        tx = 2;
        ty = 1;
      }
      else if (pseg.y < segy && nseg.x < segx || nseg.y < segy && pseg.x < segx)
      {
        // Angle Top-Left
        tx = 2;
        ty = 0;
      }
      else if (pseg.x > segx && nseg.y < segy || nseg.x > segx && pseg.y < segy)
      {
        // Angle Right-Up
        tx = 0;
        ty = 0;
      }
      else if (pseg.y > segy && nseg.x > segx || nseg.y > segy && pseg.x > segx)
      {
        // Angle Down-Right
        tx = 0;
        ty = 1;
      }
    }

    float tx1 = tx * charWidth;
    float ty1 = ty * charHeight;
    float tx2 = tx1 + charWidth;
    float ty2 = ty1 + charHeight;

    float vertices[] = {
        segment.x, segment.y, tx1, ty2,
        segment.x + snakeWidth, segment.y, tx2, ty2,
        segment.x + snakeWidth, segment.y + snakeHeight, tx2, ty1,
        segment.x, segment.y + snakeHeight, tx1, ty1};

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
    glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableVertexAttribArray(posLoc);
    glDisableVertexAttribArray(texLoc);
    glDeleteBuffers(1, &vbo);
  }
}
void drawTexturedFruit(GLuint program, GLfloat x, GLfloat y)
{
  GLuint texture = loadTexture(snakeTexturePath);
  glUseProgram(program);
  glBindTexture(GL_TEXTURE_2D, texture);
  glDepthMask(GL_TRUE);
  glBlendFunc(GL_TEXTURE_2D, GL_TEXTURE_ALPHA_TYPE);

  float charWidth = 1.0f / 5.0f; // Assuming 16x16 grid of characters
  float charHeight = 1.0f / 4.0f;

  int tx = 0;
  int ty = 3;
  float tx1 = tx * charWidth;
  float ty1 = ty * charHeight;
  float tx2 = tx1 + charWidth;
  float ty2 = ty1 + charHeight;

  float vertices[] = {
      x, y, tx1, ty2,
      x + snakeWidth, y, tx2, ty2,
      x + snakeWidth, y + snakeHeight, tx2, ty1,
      x, y + snakeHeight, tx1, ty1};

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
  glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);

  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(texLoc);
  glDeleteBuffers(1, &vbo);
}
// GLUT callbacks
GLuint program;

void drawFruit(float x, float y)
{
  drawSquare(program, x, y, snakeWidth, 1.0f, 1.0f, 0.0f);
}
void drawSnakeSegment(float x, float y)
{
  drawSquare(program, x, y, snakeWidth, 1.0f, 1.0f, 1.0f);
}
void drawSnake()
{
  GLuint snakeTexture = loadTexture(snakeTexturePath);
  drawTexturedSquare(program, snakeTexture);
  // for (const auto &segment : snakeBody)
  // {
  //   drawSquare(program, segment.x, segment.y, snakeWidth, 1.0f, 1.0f, 1.0f);
  // }
}

void drawScore()
{
  GLuint fontTexture = loadTexture(fontTexturePath);
  renderText(program, fontTexture, "SCORE:" + to_string(playerScore), -0.9f, 0.85f, 0.07f, 0.0f, 0.0f, 0.0f, false);
}

void drawGameover()
{
  if (!gameOverSoundPlayed)
  {
    playAudio(gameOverSound);
    gameOverSoundPlayed = true;
  }
  GLuint fontTexture = loadTexture(fontTexturePath);
  renderText(program, fontTexture, "GAME OVER!", 0.0f, 0.7f, 0.13f, 0.0f, 0.0f, 0.0f, true);
  renderText(program, fontTexture, "SCORE:" + to_string(playerScore), 0.0f, 0.58f, 0.1f, 0.0f, 0.0f, 0.0f, true);
  renderText(program, fontTexture, "\"SPACE\" TO RESTART", 0.0f, 0.48f, 0.08f, 0.0f, 0.0f, 0.0f, true);
}

void display()
{
  glClearColor(0.01f, 0.75f, 0.01f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  GLint useFontTextureLoc = glGetUniformLocation(program, "uUseFontTexture");
  glUniform1i(useFontTextureLoc, 1);

  GLint useTextureLoc = glGetUniformLocation(program, "uUseTexture");
  glUniform1i(useTextureLoc, 0);

  if (isGameOver)
  {
    drawGameover();
  }
  else
  {
    drawScore();

    glUniform1i(useFontTextureLoc, 0);
    glUniform1i(useTextureLoc, 1);

    drawTexturedFruit(program, fruitX, fruitY);

    glUniform1i(useFontTextureLoc, 0);
    glUniform1i(useTextureLoc, 1);

    drawSnake();
  }

  glutSwapBuffers();
}

void resetGame()
{
  // Reset game over flag and player score
  isGameOver = false;
  playerScore = 0;
  gameOverSoundPlayed = false;

  // Reset snake direction
  snakeDirection = NONE;

  // Reset snake body to initial state (one segment)
  snakeBody.clear();
  snakeBody.push_back({0.0f, 0.0f});
  snakeBody.push_back({snakeBody.back().x, snakeBody.back().y - snakeHeight});

  // Reset fruit position
  placeFruit();
}

void keyboard(unsigned char key, int, int)
{
  if (key == 'a' && snakeDirection != RIGHT)
    snakeDirection = LEFT;
  if (key == 'd' && snakeDirection != LEFT)
    snakeDirection = RIGHT;
  if (key == 'w' && snakeDirection != DOWN)
    snakeDirection = UP;
  if (key == 's' && snakeDirection != UP)
    snakeDirection = DOWN;
  if (key == ' ' && snakeDirection != NONE && !isGameOver)
    snakeDirection = NONE;
  if (key == ' ' && isGameOver)
    resetGame();
}

void specialKeyboard(int key, int, int)
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

std::chrono::steady_clock::time_point lastMoveSoundTime = std::chrono::steady_clock::now();
const std::chrono::milliseconds moveSoundInterval(300);

void update(int)
{
  auto currentTime = std::chrono::steady_clock::now();
  bool movementIntervalMet = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastMoveTime) >= moveInterval;
  bool movementSoundIntervalMet = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastMoveSoundTime) >= moveSoundInterval;
  if (enableMoveSound && movementSoundIntervalMet && snakeDirection != NONE && !isGameOver)
  {
    playAudio(moveSound, false, 0.1f);
    lastMoveSoundTime = currentTime;
  }
  if (movementIntervalMet)
  {
    moveSnake();
    checkCollisions();
    if (enableMusic && !musicPlayed && snakeDirection != NONE)
    {
      playAudio(music, true, 0.1f);
      musicPlayed = true;
    }
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
  snakeWidth = 4.0f / columns;
  snakeHeight = 4.0f / rows;
  snakeBody.push_back({snakeBody.back().x, snakeBody.back().y - snakeHeight});

  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(specialKeyboard);
  glutTimerFunc(1000 / frame_rate, update, 0);

  glutMainLoop();
  return 0;
}
