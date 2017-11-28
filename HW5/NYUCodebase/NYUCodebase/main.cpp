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
#include <SDL_mixer.h>

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#define FIXED_TIMESTEP 0.0166666f

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

class Vector3 {
public:
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

	float x;
	float y;
	float z;
};

enum EntityType { ENTITY_PLAYER, ENTITY_ENEMY, ENTITY_COIN, ENTITY_GROUND };

class Entity {
public:
	Entity() : position(Vector3(0.0f, 0.0f, 0.0f)), velocity(Vector3(0.0f, 0.0f, 0.0f)), acceleration(Vector3(0.0f, 0.0f, 0.0f)), isStatic(true), active(false) {}

	Entity(Vector3 pos, SheetSprite spr, EntityType et, bool isStatic, bool active)
		: position(pos), velocity(Vector3(0.0f, 0.0f, 0.0f)), acceleration(Vector3(0.0f, 0.0f, 0.0f)), 
		sprite(spr), entityType(et), isStatic(isStatic), active(active) {}

	void Update(float elapsed);	
	void Render(Matrix& model, Matrix& view, ShaderProgram& program);
	bool CollidesWith(Entity &entity);

	SheetSprite sprite;

	Vector3 position;
	//Vector3 size;
	Vector3 velocity;
	Vector3 acceleration;
	//Getting the top bottom left and right coordinates
	float top()
	{
		return position.y + sprite.size / 2;
	}

	float bottom()
	{
		return position.y - sprite.size / 2;
	}

	float left()
	{
		return position.x - sprite.size * (sprite.width / sprite.height) / 2;
	}

	float right()
	{
		return position.x + sprite.size * (sprite.width / sprite.height) / 2;
	}

	bool isStatic;

	bool active;

	EntityType entityType;

	bool collidedTop;
	bool collidedBottom;
	bool collidedLeft;
	bool collidedRight;
};

//Initializing Variables
SDL_Window* displayWindow;
//New music variables
Mix_Music* backgroundMusic;
Mix_Chunk* jumpSound;
Mix_Chunk* coinSound;

Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;
Entity player;
float rotation = 0;
std::vector<Entity> staticEntities;
std::vector<Entity> coins;
float lastFrameTicks = 0.0f;
int score = 0;
Vector3 friction = Vector3(1.0f, 0.5f, 0.0f);
Vector3 gravity = Vector3(0.0f, -2.5f, 0.0f);
const Uint8 *keys = SDL_GetKeyboardState(NULL);

//Functions
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

float lerp(float v0, float v1, float t) {
	return (1.0 - t) * v0 + t * v1;
}

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

//A Function to check collision
bool checkCollision(Entity& entity1, Entity& entity2)
{
	if (entity1.top() >= entity2.bottom() && entity1.bottom() <= entity2.top() && entity1.left() < entity2.right() && entity1.right() >= entity2.left())
		return true;

	return false;
}

//Add acceleration to player depends on input
void processInput(Entity& player, float elapsed)
{
	if (keys[SDL_SCANCODE_LEFT])
	{
		player.acceleration.x = -2.5f;
	}
	else if (keys[SDL_SCANCODE_RIGHT])
	{
		player.acceleration.x = 2.5f;
	}
	else {
		player.acceleration.x = 0.0f;
	}

	//jump if the player is on the ground
	if (keys[SDL_SCANCODE_SPACE])
	{
		player.acceleration.y = 7.0f;
		Mix_PlayChannel(-1, jumpSound, 0); //Newly added jump sound for HW5
	}

	else {
		player.acceleration.y = 0.0f;
	}
}

void Entity::Update(float elapsed) {

	//If this dynamic entity touch a coin deactivate it and increment score;
	for (Entity& coin : coins) {
		if (CollidesWith(coin)) {
			if (coin.active) {
				coin.active = false;
				Mix_PlayChannel(-1, coinSound, 0); //Newly added coin sound for HW5
				score++;
			}
		}
	}

	//Standard physics movement
	if (isStatic == false && active) {

		collidedTop = false;
		collidedBottom = false;
		collidedLeft = false;
		collidedRight = false;
		
		velocity.x = lerp(velocity.x, 0.0f, elapsed * friction.x);
		velocity.y = lerp(velocity.y, 0.0f, elapsed * friction.y);

		velocity.x += acceleration.x * elapsed;
		velocity.y += acceleration.y * elapsed;

		velocity.x += gravity.x * elapsed;
		velocity.y += gravity.y * elapsed;

		position.x += velocity.x * elapsed;
		//Checking collision (x axis)
		for (Entity& thing : staticEntities)
		{
			if (checkCollision(*this, thing))
			{
				//If they collided, calculate their penetration and move accordingly and set velocity in that direction to 0
				if (this->left() > thing.left())
				{
					this->position.x += fabs(thing.right() - this->left()) + 0.0001f;
					this->collidedLeft = true;
					this->velocity.x = 0.0f;
				}
				else if (this->right() < thing.right())
				{
					this->position.x -= fabs(this->right() - thing.left()) + 0.0001f;
					this->collidedRight = true;
					this->velocity.x = 0.0f;
				}
			}
		}
		position.y += velocity.y * elapsed;
		//Checking collision (y axis)
		for (Entity& thing : staticEntities)
		{
			if (checkCollision(*this, thing))
			{
				//If they collided, calculate their penetration and move accordingly and set velocity in that direction to 0
				if (this->top() > thing.top())
				{
					this->position.y += fabs(thing.top() - this->bottom()) + 0.0001f;
					this->collidedBottom = true;
					this->velocity.y = 0.0f;
				}
				else if (this->bottom() < thing.bottom())
				{
					this->position.y -= fabs(this->top() - thing.bottom()) + 0.0001f;
					this->collidedTop = true;
					this->velocity.y = 0.0f;
				}
			}
		}
	}
}

//Render an entity and rotation around y axis if its a coin
void Entity::Render(Matrix& model, Matrix& view, ShaderProgram& program) {
	if (active) {
		model.Identity();
		model.Translate(position.x, position.y, 0);
		if (entityType == ENTITY_COIN) {
			model.Yaw(5 * rotation);
		}
		program.SetModelviewMatrix(model * view);
		sprite.Draw(&program);
	}
}

//Check collision with an entity
bool Entity::CollidesWith(Entity &entity) {
	return checkCollision(*this, entity);
}

//Initial Setup
void Setup() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Platformer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 640, 360);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	projectionMatrix.SetOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);

	//Sound Initialized for HW5
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
	backgroundMusic = Mix_LoadMUS("background.flac");
	jumpSound = Mix_LoadWAV("jump_11.wav");
	coinSound = Mix_LoadWAV("coin.wav");
	//Background music played on loop forever
	Mix_PlayMusic(backgroundMusic, -1);

	//Load spritesheet
	GLuint spriteSheetTexture1 = LoadTexture(RESOURCE_FOLDER"sheet1.png");
	GLuint spriteSheetPlayer = LoadTexture(RESOURCE_FOLDER"p1_spritesheet.png");
	GLuint spriteSheetItem = LoadTexture(RESOURCE_FOLDER"items_spritesheet.png");

	//Load ground sprites
	SheetSprite grassLeft(spriteSheetTexture1, 280.0f / 512.0f, 280.0f / 512.0f, 70.0f / 512.0f, 70.0f / 512.0f, 0.5f);
	SheetSprite grassMid(spriteSheetTexture1, 280.0f / 512.0f, 210.0f / 512.0f, 70.0f / 512.0f, 70.0f / 512.0f, 0.5f);
	SheetSprite grassRight(spriteSheetTexture1, 280.0f / 512.0f, 140.0f / 512.0f, 70.0f / 512.0f, 70.0f / 512.0f, 0.5f);
	SheetSprite grassLeftWall(spriteSheetTexture1, 140.0f / 512.0f, 0.0f / 512.0f, 70.0f / 512.0f, 70.0f / 512.0f, 0.5f);
	SheetSprite grassRightWall(spriteSheetTexture1, 140.0f / 512.0f, 70.0f / 512.0f, 70.0f / 512.0f, 70.0f / 512.0f, 0.5f);

	//Adding ground
	staticEntities.push_back(Entity(Vector3(-3.3f, -1.7f, 0.0f), grassLeft, ENTITY_GROUND, true, true));

	for (int i = 1; i < 13; i++)
	{
		staticEntities.push_back(Entity(Vector3(-3.3f + 0.5f * i, -1.7f, 0.0f), grassMid, ENTITY_GROUND, true, true));
	}

	staticEntities.push_back(Entity(Vector3(-3.3f + 0.5f * 13, -1.7f, 0.0f), grassRight, ENTITY_GROUND, true, true));

	for (int i = 1; i < 8; i++)
	{
		staticEntities.push_back(Entity(Vector3(-3.3f, -1.7f + 0.5f * i, 0.0f), grassLeftWall, ENTITY_GROUND, true, true));
	}

	for (int i = 1; i < 8; i++)
	{
		staticEntities.push_back(Entity(Vector3(-3.3f + 0.5f * 13, -1.7f + 0.5f * i, 0.0f), grassRightWall, ENTITY_GROUND, true, true));
	}

	//Adding Player
	SheetSprite spr_player(spriteSheetPlayer, 0.0f / 512.0f, 196.0f / 512.0f, 66.0f / 512.0f, 92.0f / 512.0f, 0.5f);

	player = Entity(Vector3(0.0f, 0.0f, 0.0f), spr_player, ENTITY_PLAYER, false, true);

	//Adding Coin
	SheetSprite spr_coin(spriteSheetItem, 288.0f / 576.0f, 360.0f / 576.0f, 70.0f / 576.0f, 70.0f / 576.0f, 0.3f);

	for (int i = 1; i < 11; i++) {
		coins.push_back(Entity(Vector3(-3.3f + 0.5f, -1.7f + 0.5f * i, 0.0f), spr_coin, ENTITY_COIN, true, true));
	}

	for (int i = 1; i < 11; i++) {
		coins.push_back(Entity(Vector3(-3.3f + 0.5f * 12, -1.7f + 0.5f * i, 0.0f), spr_coin, ENTITY_COIN, true, true));
	}
}

//Calling update function of each dynamic entity
void Update(float elapsed) {
	processInput(player, elapsed);
	player.Update(elapsed);
}

//Call render function of each entity and set camera to follow players
void Render(Matrix& model, Matrix& view, ShaderProgram& program) {
	for (size_t i = 0; i < staticEntities.size(); i++) {
		staticEntities[i].Render(model, view, program);
	}
	for (size_t i = 0; i < coins.size(); i++) {
		coins[i].Render(model, view, program);
	}
	player.Render(model, view, program);

	view.Identity();
	view.Translate(-player.position.x, -player.position.y, -player.position.z);
	program.SetModelviewMatrix(model * view);
}

int main(int argc, char *argv[])
{
	Setup();

	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	GLuint text = LoadTexture(RESOURCE_FOLDER"font1.png");

	SDL_Event event;
	bool done = false;
	
	float accumulator = 0.0f;

	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		rotation += 0.1f;

		//Fixing the time step
		//get elapsed time
		elapsed += accumulator;
		if (elapsed < FIXED_TIMESTEP) {
			accumulator = elapsed;
			continue;
		}

		while (elapsed >= FIXED_TIMESTEP) {
			Update(FIXED_TIMESTEP);
			elapsed -= FIXED_TIMESTEP;
		}
		accumulator = elapsed;

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(program.programID);

		program.SetModelviewMatrix(modelMatrix * viewMatrix);
		program.SetProjectionMatrix(projectionMatrix);

		Render(modelMatrix, viewMatrix, program);

		//Draw Score
		modelMatrix.Identity();
		modelMatrix.Translate(-3.35f + player.position.x, 1.9f + player.position.y, 0.0f);
		program.SetModelviewMatrix(modelMatrix * viewMatrix);
		DrawText(&program, text, "Score: " + std::to_string(score), 0.3f, -.15f);

		SDL_GL_SwapWindow(displayWindow);
	}

	Mix_FreeChunk(jumpSound);
	Mix_FreeMusic(backgroundMusic);

	SDL_Quit();
	return 0;
}
