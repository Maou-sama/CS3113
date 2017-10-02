/*
Name: Minh Cong Ho
netID: mch505
*/

#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <iostream>
#include <sstream>
#include <Windows.h>
#include <stdio.h>
#include "stb_image.h"
#include "ShaderProgram.h"
#include "Matrix.h"
#include <assert.h>

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

using namespace std;

//Constant variable
float PI = 3.14159265359;

//Initiate variables
SDL_Window* displayWindow;
float lastFrameTicks;
float ticks;
float elapsed;
Matrix projectionMatrix;
Matrix modelviewMatrix;
float x1_position;
float y1_position;
float x2_position;
float y2_position;
float ball_x_position;
float ball_y_position;
float ball_movement_angleX;
float ball_movement_angleY;
bool player_1_win;
bool player_2_win;
GLuint player1;
GLuint player2;
GLuint player_1_win_flag;
GLuint player_2_win_flag;
GLuint ball;

GLuint LoadTexture(const char* filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}
	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(image);
	return retTexture;
}

void setup() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	glViewport(0, 0, 640, 360);
	
	projectionMatrix.SetOrthoProjection(-14.2f, 14.2f, -8.0f, 8.0f, -1.0f, 1.0f);

	//Setting up variables
	lastFrameTicks = 0.0f;
	x1_position = -13.0f;
	y1_position = 0.0f;
	x2_position = 13.0f;
	y2_position = 0.0f;
	ball_x_position = 0.0f;
	ball_y_position = 0.0f;
	ball_movement_angleX = PI / 4;
	ball_movement_angleY = PI / 4;
	player_1_win = false;
	player_2_win = false;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

//Function to detect collision
bool collisionDetection(float top1, float bottom1, float left1, float right1, float top2, float bottom2, float left2, float right2) {
	if ((bottom1 > top2) || (top1 < bottom2) || (left1 > right2) || (right1 < left2)) {
		return false;
	}
	else {
		return true;
	}
}

void processEvents() {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	//If W is pressed move player 1 up
	if (keys[SDL_SCANCODE_W]) {
		if (y1_position + 2.0f < 8.0f) {
			y1_position += elapsed * 8.0f;
		}
	}

	//If S is pressed move player 1 down
	if (keys[SDL_SCANCODE_S]) {
		if (y1_position - 2.0f > -8.0f) {
			y1_position -= elapsed * 8.0f;
		}
	}

	//If Up is pressed move player 2 up
	if (keys[SDL_SCANCODE_UP]) {
		if (y2_position + 2.0f < 8.0f) {
			y2_position += elapsed * 8.0f;
		}
	}

	//If Down is pressed move player 2 down
	if (keys[SDL_SCANCODE_DOWN]) {
		if (y2_position - 2.0f > -8.0f) {
			y2_position -= elapsed * 8.0f;
		}
	}
}

void update() {
	//Check if ball is touching the top and change the movement angle
	if (ball_y_position + 0.5f > 8.0f) {
		ball_movement_angleY = -ball_movement_angleY;
	}

	//Check if ball is touching the bottom and change the movement angle
	if (ball_y_position - 0.5f < -8.0f) {
		ball_movement_angleY = -ball_movement_angleY;
	}

	//Check if ball and player 2 is colliding and change the movement angle
	if (collisionDetection(ball_y_position + 0.5f, ball_y_position - 0.5f, ball_x_position - 0.5f, ball_x_position + 0.5f, y2_position + 2.0f, y2_position - 2.0f, x2_position - 0.5f, x2_position + 0.5f)) {
		ball_movement_angleX = ball_movement_angleX + PI;
	}

	//Check if ball and player 1 is colliding and change the movement angle
	if (collisionDetection(y1_position + 2.0f, y1_position - 2.0f, x1_position - 0.5f, x1_position + 0.5f, ball_y_position + 0.5f, ball_y_position - 0.5f, ball_x_position - 0.5f, ball_x_position + 0.5f)) {
		ball_movement_angleX = ball_movement_angleX + PI;
	}

	//Check if player 1 wins
	if (ball_x_position - 0.5f > 14.2f) {
		player_1_win = true;
	}

	//Check if player 2 wins
	if (ball_x_position + 0.5f < -14.2f) {
		player_2_win = true;
	}

	//Update ball movement
	ball_x_position += cos(ball_movement_angleX) * elapsed * 8.0f;
	ball_y_position += sin(ball_movement_angleY) * elapsed * 8.0f;
}

int main(int argc, char *argv[])
{
	setup();

	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	player1 = LoadTexture(RESOURCE_FOLDER"glass1.png");
	player2 = LoadTexture(RESOURCE_FOLDER"glass2.png");
	player_1_win_flag = LoadTexture(RESOURCE_FOLDER"flagBlue.png");
	player_2_win_flag = LoadTexture(RESOURCE_FOLDER"flagGreen.png");
	ball = LoadTexture(RESOURCE_FOLDER"glass.png");

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}

		//Getting elapsed time to put in movement calculation
		ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(program.programID);
		program.SetModelviewMatrix(modelviewMatrix);
		program.SetProjectionMatrix(projectionMatrix);

		processEvents();
		update();

		//If player 1 wins then displays the appropriate flag
		if (player_1_win) {
			modelviewMatrix.Identity();
			program.SetModelviewMatrix(modelviewMatrix);

			float player1WinVertices[] = { -1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, -1.0, 1.0, 1.0, -1.0, 1.0 };
			glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, player1WinVertices);
			glEnableVertexAttribArray(program.positionAttribute);

			float player1WinCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
			glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, player1WinCoords);
			glEnableVertexAttribArray(program.texCoordAttribute);

			glBindTexture(GL_TEXTURE_2D, player_1_win_flag);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			glDisableVertexAttribArray(program.positionAttribute);
			glDisableVertexAttribArray(program.texCoordAttribute);
		}

		//If player 2 wins then displays the appropriate flag
		if (player_2_win) {
			modelviewMatrix.Identity();
			program.SetModelviewMatrix(modelviewMatrix);

			float player2WinVertices[] = { -1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, -1.0, 1.0, 1.0, -1.0, 1.0 };
			glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, player2WinVertices);
			glEnableVertexAttribArray(program.positionAttribute);

			float player2WinCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
			glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, player2WinCoords);
			glEnableVertexAttribArray(program.texCoordAttribute);

			glBindTexture(GL_TEXTURE_2D, player_2_win_flag);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			glDisableVertexAttribArray(program.positionAttribute);
			glDisableVertexAttribArray(program.texCoordAttribute);
		}

		//Display the player 1
		modelviewMatrix.Identity();
		modelviewMatrix.Translate(x1_position, y1_position, 0.0f);
		program.SetModelviewMatrix(modelviewMatrix);

		float player1Vertices[] = { -0.5, -2.0, 0.5, -2.0, 0.5, 2.0, -0.5, -2.0, 0.5, 2.0, -0.5, 2.0 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, player1Vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float player1Coords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, player1Coords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glBindTexture(GL_TEXTURE_2D, player1);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		//Display the player 2
		modelviewMatrix.Identity();
		modelviewMatrix.Translate(x2_position, y2_position, 0.0f);
		program.SetModelviewMatrix(modelviewMatrix);

		float player2Vertices[] = { -0.5, -2.0, 0.5, -2.0, 0.5, 2.0, -0.5, -2.0, 0.5, 2.0, -0.5, 2.0 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, player2Vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float player2Coords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, player2Coords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glBindTexture(GL_TEXTURE_2D, player2);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		//Display the ball
		modelviewMatrix.Identity();
		modelviewMatrix.Translate(ball_x_position, ball_y_position, 0.0f);
		program.SetModelviewMatrix(modelviewMatrix);

		float ballVertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, ballVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float ballCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, ballCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glBindTexture(GL_TEXTURE_2D, ball);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
