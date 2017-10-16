#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <stdio.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <assert.h>
#include "ShaderProgram.h"
#include "Matrix.h"
#include <vector>

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

//Creating necessary classes
class SheetSprite {
public:
	SheetSprite() : textureID(NULL), u(NULL), v(NULL), width(NULL), height(NULL), size(NULL) {}
	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float
		size) : textureID(textureID), u(u), v(v), width(width), height(height), size(size) {}

	void Draw(ShaderProgram *program);

	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};

class Entity
{
public:
	float x;
	float y;
	float directionX;
	SheetSprite sprite;
	bool alive;

	Entity() : x(NULL), y(NULL), directionX(NULL), sprite(), alive(NULL) {}
	Entity(float posX, float posY, float dX, SheetSprite spr, bool live)
		: x(posX), y(posY), directionX(dX), sprite(spr), alive(live) {}

	void Draw(Matrix& model, ShaderProgram& program);
};

//Initialize Variables
SDL_Window* displayWindow;
Matrix projectionMatrix;
Matrix modelviewMatrix;
float lastFrameTicks = 0.0f;
Entity player;
std::vector<Entity> enemies;
Entity playerBullets;
std::vector<Entity> enemiesBullets;
int enemyBulletIndex = 0;
int enemiesAlive = 30;
//5 game states: Main Menu, Help Screen, Main Game, Game Over Screen, Win screen
enum GameMode { STATE_MAIN_MENU, STATE_HELP, STATE_GAME_LEVEL, STATE_GAME_OVER, STATE_WIN };
GameMode state;
const Uint8 *keys = SDL_GetKeyboardState(NULL);

//Function to draw text, load texture and draw texture
void DrawText(ShaderProgram *program, int fontTexture, std::string text, float size, float spacing) {
	float texture_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for (int i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];
		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;

		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
		});
	}

	glBindTexture(GL_TEXTURE_2D, fontTexture);

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);

	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6 * text.size());

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

GLuint LoadTexture(const char *filePath) {
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

void SheetSprite::Draw(ShaderProgram *program) {
	glBindTexture(GL_TEXTURE_2D, textureID);
	GLfloat texCoords[] = {
		u, v + height,
		u + width, v,
		u, v,
		u + width, v,
		u, v + height,
		u + width, v + height
	};
	float aspect = width / height;
	float vertices[] = {
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, 0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, -0.5f * size };

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);

	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

void Entity::Draw(Matrix& model, ShaderProgram& program){
	model.Identity();
	model.Translate(x, y, 0);
	program.SetModelviewMatrix(model);
	sprite.Draw(&program);
}

//The render function for each states
void renderMainMenu(Matrix& model, ShaderProgram& program, GLuint& texture)
{
	model.Identity();
	model.Translate(-2.1, 1.0, 0);
	program.SetModelviewMatrix(model);
	DrawText(&program, texture, "Space Invaders", 0.6, -0.25);

	model.Identity();
	model.Translate(-1.4, 0.0, 0);
	program.SetModelviewMatrix(model);
	DrawText(&program, texture, "Press ENTER to start the game", 0.2, -0.1);

	model.Identity();
	model.Translate(-1.2, -1.0, 0);
	program.SetModelviewMatrix(model);
	DrawText(&program, texture, "Press H for control help", 0.2, -0.1);
}

void renderHelp(Matrix& model, ShaderProgram& program, GLuint& texture)
{
	model.Identity();
	model.Translate(-2.5, 0.5, 0);
	program.SetModelviewMatrix(model);
	DrawText(&program, texture, "Use left or right arrow to move", 0.4, -0.22);

	model.Identity();
	model.Translate(-1.6, 0.0, 0);
	program.SetModelviewMatrix(model);
	DrawText(&program, texture, "Press Space to shoot", 0.4, -0.22);

	model.Identity();
	model.Translate(-1.4, -1.5, 0);
	program.SetModelviewMatrix(model);
	DrawText(&program, texture, "Press ESC to return to main menu", 0.2, -0.1);
}

void renderGameLevel(Matrix& model, ShaderProgram& program, Entity& player, std::vector<Entity>& enemies, Entity& playerBullet, std::vector<Entity>& enemiesBullets)
{
	player.Draw(model, program);

	for (Entity enemy : enemies)
	{
		if (enemy.alive)
			enemy.Draw(model, program);
	}

	if (playerBullet.alive)
		playerBullet.Draw(model, program);

	for (Entity enemiesBullet : enemiesBullets)
	{
		if (enemiesBullet.alive)
			enemiesBullet.Draw(model, program);
	}
}

void renderGameOver(Matrix& model, ShaderProgram& program, GLuint& texture)
{
	model.Identity();
	model.Translate(-1.5, 0.0, 0);
	program.SetModelviewMatrix(model);
	DrawText(&program, texture, "GAME OVER!", 0.6, -.25);
}

void renderWin(Matrix& model, ShaderProgram& program, GLuint& texture)
{
	model.Identity();
	model.Translate(-1.2, 0.0, 0);
	program.SetModelviewMatrix(model);
	DrawText(&program, texture, "YOU WON!", 0.6, -.25);
}

//Collision detection function
bool collisionDetection(Entity& entity1, Entity& entity2)
{
	float entity1Left = entity1.x - entity1.sprite.size * (entity1.sprite.width / entity1.sprite.height) / 2;
	float entity1Right = entity1.x + entity1.sprite.size * (entity1.sprite.width / entity1.sprite.height) / 2;
	float entity2Left = entity2.x - entity2.sprite.size * (entity2.sprite.width / entity2.sprite.height) / 2;
	float entity2Right = entity2.x + entity2.sprite.size * (entity2.sprite.width / entity2.sprite.height) / 2;

	if ((entity1.y + entity1.sprite.size / 2) >= (entity2.y - entity2.sprite.size / 2)
		&& (entity1.y - entity1.sprite.size / 2) <= (entity2.y + entity2.sprite.size / 2) && entity1Left < entity2Right && entity1Right >= entity2Left)
	{
		return true;
	}

	return false;
}

//Process user input
void processInput(Entity& player, Entity& playerBullet, float elapsed)
{
	if (keys[SDL_SCANCODE_LEFT] && player.x >= -3.3f)
	{
		player.x -= 1.2f * elapsed;
	}

	if (keys[SDL_SCANCODE_RIGHT] && player.x <= 3.3f)
	{
		player.x += 1.2f * elapsed;
	}

	if (keys[SDL_SCANCODE_SPACE])
	{
		//If bullet is not activated then set it to activated and put it on top of the player
		if (!playerBullet.alive)
		{
			playerBullet.alive = true;
			playerBullet.x = player.x;
			playerBullet.y = player.y + player.sprite.height / 2;
		}
	}
}

//Setup stuffs
void setup() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Space Invader", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	glViewport(0, 0, 640, 360);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	projectionMatrix.SetOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);

	//Sprite sheet for the game sprites and for the text
	GLuint spriteSheetTexture = LoadTexture(RESOURCE_FOLDER"sheet.png");

	//Load the texture from sprite sheet using coordinate from the XML file
	SheetSprite playerShip(spriteSheetTexture, 237.0f / 1024.0f, 377.0f / 1024.0f, 99.0 / 1024.0f, 75.0f / 1024.0f, 0.3f);
	SheetSprite enemyShip(spriteSheetTexture, 143.0f / 1024.0f, 293.0f / 1024.0f, 104.0 / 1024.0f, 84.0f / 1024.0f, 0.3f);
	SheetSprite playerBullet(spriteSheetTexture, 843.0f / 1024.0f, 116.0f / 1024.0f, 13.0 / 1024.0f, 57.0f / 1024.0f, 0.2f);
	SheetSprite enemyBullet(spriteSheetTexture, 841.0f / 1024.0f, 647.0f / 1024.0f, 13.0 / 1024.0f, 37.0f / 1024.0f, 0.2f);

	player = Entity(0.0f, -1.8f, 0.0f, playerShip, true);

	//Generate 6 per row * 5 rows of enemies
	for (float i = -1.85f; i <= 2.0f; i += 0.75f)
	{
		for (float j = 1.8f; j >= 0.0f; j -= 0.4f)
			enemies.push_back(Entity(i, j, 1.0f, enemyShip, true));
	}
	
	//One bullet at a time for player
	playerBullets = Entity(100.0f, 100.0f, 0.0f, playerBullet, false);

	//Generate bullet for enemies (max 3)
	for (int i = 0; i < 3; i++)
	{
		enemiesBullets.push_back(Entity(-100.0f - i, -100.0f - i, 0.0f, enemyBullet, false));
	}

	state = STATE_MAIN_MENU;
}

void update(Entity& player, std::vector<Entity>& enemies, Entity& playerBullet, std::vector<Entity>& enemiesBullets, float elapsed)
{

	//Check if the enemies hit the edges
	bool turnAround = false;

	//Shoot the player bullet
	if (playerBullet.alive)
		playerBullet.y += 3.0f * elapsed;

	//Bullet is deactivated if it goes off screen
	if (playerBullet.y > 2.0f)
		playerBullet.alive = false;

	//Move the enemy bullets
	for (Entity& enemiesBullet : enemiesBullets)
	{
		if (enemiesBullet.alive)
			enemiesBullet.y -= 1.5f * elapsed;
		if (enemiesBullet.y <= -2.0f)
			enemiesBullet.alive = false;
		//Check for collision between enemies' bullet and the player and end the game if one of them hit
		if (enemiesBullet.alive && collisionDetection(player, enemiesBullet))
			state = STATE_GAME_OVER;
	}

	for (Entity& enemy : enemies)
	{
		//Check if player bullet collided with any enemy and kill it if collide and move to win state if no enemy left
		if (enemy.alive && playerBullet.alive && collisionDetection(playerBullet, enemy))
		{
			enemy.alive = false;
			playerBullet.alive = false;
			enemiesAlive--;
			if (enemiesAlive == 0)
				state = STATE_WIN;
		}

		//If any enemy is alive generate a bullet at random interval
		else if (enemy.alive)
		{
			if ((std::rand() % 100 + 1) <= 1)
			{
				//Max 3 enemy bullet can be on the screen
				//Enemies can shoot multiple bullets to increase difficulty at end
				if (!enemiesBullets[enemyBulletIndex].alive)
				{
					enemiesBullets[enemyBulletIndex].alive = true;
					enemiesBullets[enemyBulletIndex].x = enemy.x;
					enemiesBullets[enemyBulletIndex].y = enemy.y - enemy.sprite.height / 2;
					enemyBulletIndex++;

					if (enemyBulletIndex > 2)
						enemyBulletIndex = 0;
				}
			}

			//If enemy collide with player move to game over state
			if (collisionDetection(enemy, player))
				state = STATE_GAME_OVER;
		}


		//Move the enemies
		enemy.x += 0.3f * enemy.directionX * elapsed;

		//If enemies reach either of the edges change their direction
		if (enemy.x >= 3.3f)
		{
			turnAround = true;
		}
		else if (enemy.x <= -3.3f)
		{
			turnAround = true;
		}
	}

	//Reverse directions and move down if an edge is reached
	if (turnAround)
	{
		for (Entity& enemy : enemies)
		{
			enemy.directionX *= -1;
			enemy.y -= 0.05f;
			if (enemy.y < -2.0f) {
				state = STATE_GAME_OVER;
			}
		}
	}
}

int main(int argc, char *argv[])
{
	setup();

	//Initialize stuffs that cannot be done in setup
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	GLuint text = LoadTexture(RESOURCE_FOLDER"font1.png");

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(program.programID);

		program.SetModelviewMatrix(modelviewMatrix);
		program.SetProjectionMatrix(projectionMatrix);

		//Handle each of the game states
		switch (state)
		{
		case(STATE_MAIN_MENU):
			renderMainMenu(modelviewMatrix, program, text);
			if (keys[SDL_SCANCODE_H]) { state = STATE_HELP; }
			if (keys[SDL_SCANCODE_RETURN]) { state = STATE_GAME_LEVEL; }
			break;
		case(STATE_HELP):
			renderHelp(modelviewMatrix, program, text);
			if (keys[SDL_SCANCODE_ESCAPE]) { state = STATE_MAIN_MENU; }
			break;
		case(STATE_GAME_LEVEL):
			renderGameLevel(modelviewMatrix, program, player, enemies, playerBullets, enemiesBullets);
			update(player, enemies, playerBullets, enemiesBullets, elapsed);
			processInput(player, playerBullets, elapsed);
			break;
		case(STATE_GAME_OVER):
			renderGameOver(modelviewMatrix, program, text);
			break;
		case(STATE_WIN):
			renderWin(modelviewMatrix, program, text);
			break;
		}

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
