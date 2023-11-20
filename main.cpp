/**
* Author: Safin Shihab
* Assignment: Rise of the AI
* Date due: 2023-11-18, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define ENEMY_COUNT 3
#define LEVEL1_WIDTH 20
#define LEVEL1_HEIGHT 8

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_mixer.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"

// ––––– STRUCTS AND ENUMS ––––– //
struct GameState
{
    Entity* player;
    Map* map;
    Entity* enemies;

    Mix_Music* bgm;
};

// ––––– CONSTANTS ––––– //
const int   WINDOW_WIDTH = 640,
            WINDOW_HEIGHT = 480;

const float BG_RED = 0.1922f,
            BG_BLUE = 0.549f,
            BG_GREEN = 0.9059f,
            BG_OPACITY = 1.0f;

const int   VIEWPORT_X = 0,
            VIEWPORT_Y = 0,
            VIEWPORT_WIDTH = WINDOW_WIDTH,
            VIEWPORT_HEIGHT = WINDOW_HEIGHT,
            FONTBANK_SIZE = 16;


const char  V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
            F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND  = 1000.0;
const char  SPRITESHEET_FILEPATH[]  = "assets/images/him.png",
            MAP_TILESET_FILEPATH[]  = "assets/images/tileset.png",
            ENEMY_FILEPATH[]        = "assets/images/Monster.png",
            FONT_FILEPAHT[]         = "assets/fonts/font1.png";



const char  BGM_FILEPATH[]          = "assets/audio/barriers.mp3";
const int   LOOP_FOREVER     = -1;  // -1 means loop forever in Mix_PlayMusic; 0 means play once and loop zero times

const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL  = 0;
const GLint TEXTURE_BORDER   = 0;

bool g_game_is_actually_running = true;

int g_enemies_alive = 0;

unsigned int LEVEL_DATA[] =
{
    5, 0, 5, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    5, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    5, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    5, 0, 5, 0, 0, 0, 0, 5, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 5, 5, 0, 0, 0, 0, 0, 0, 5, 0, 0, 5, 5, 5, 5, 5,
    0, 0, 5, 5, 5, 5, 0, 0, 0, 0, 0, 5, 5, 0, 0, 0, 0, 0, 0, 0,
    0, 5, 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 0, 0, 5, 5, 5, 5, 5,
};




// ––––– GLOBAL VARIABLES ––––– //
GameState g_game_state;
GLuint    g_font_texture_id;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_time_accumulator = 0.0f;

// ———— GENERAL FUNCTIONS ———— //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}

void sleep(float seconds){
//    To add delay ;
    clock_t startClock = clock();
    float secondsAhead = seconds * CLOCKS_PER_SEC;
    while(clock() < startClock+secondsAhead);
    return;
}

void draw_text(ShaderProgram *program, GLuint font_texture_id, std::string text, float screen_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for each character
    // Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their position
        //    relative to the whole sentence)
        int spritesheet_index = (int) text[i];  // ascii value of character
        float offset = (screen_size + spacing) * i;
        
        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float) (spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float) (spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
        });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
        });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);
    
    program->set_model_matrix(model_matrix);
    glUseProgram(program->get_program_id());
    
    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());
    
    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int) (text.size() * 6));
    
    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}
void initialise()
{
    // Initialising both the video AND audio subsystems
    // We did something similar when we talked about video game controllers
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("Rise of AI",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif
    // ––––– VIDEO SETUP ––––– //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-0.5f, 19.35f, -7.5f, 0.5f, -3.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

  

    // ––––– PLATFORMS ––––– //
    GLuint map_texture_id = load_texture(MAP_TILESET_FILEPATH);
    g_game_state.map = new Map(LEVEL1_WIDTH, LEVEL1_HEIGHT, LEVEL_DATA, map_texture_id, 1.0f, 20, 8);

    // ––––– PLAYER (HIM) ––––– //
    // Existing
    g_game_state.player = new Entity();
    g_game_state.player->set_position(glm::vec3(0.0f, -5.0f, 0.0f));
    g_game_state.player->set_movement(glm::vec3(0.0f));
    g_game_state.player->set_speed(1.0f);
    g_game_state.player->set_acceleration(glm::vec3(0.0f, -4.905f, 0.0f));
    g_game_state.player->m_texture_id = load_texture(SPRITESHEET_FILEPATH);

    // Walking
    g_game_state.player->m_walking[g_game_state.player->DOWN]   = new int[4] { 0, 1, 2,  3 };
    g_game_state.player->m_walking[g_game_state.player->RIGHT]  = new int[4] { 4, 5, 6, 7 };
    g_game_state.player->m_walking[g_game_state.player->LEFT]   = new int[4] { 8, 9, 10, 11 };
    g_game_state.player->m_walking[g_game_state.player->UP]     = new int[4] { 12, 13, 14, 15 };

    g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->RIGHT];  // start George looking left
    g_game_state.player->m_animation_frames  = 4;
    g_game_state.player->m_animation_index   = 0;
    g_game_state.player->m_animation_time    = 0.0f;
    g_game_state.player->m_animation_cols    = 4;
    g_game_state.player->m_animation_rows    = 4;
    g_game_state.player->set_height(1.0f);
    g_game_state.player->set_width(.9f);

    // Jumping
    g_game_state.player->set_jumping_power(4.0f);

    // ––––– ENEMY (MONSTERS) ––––– //
    GLuint enemy_texture_id = load_texture(ENEMY_FILEPATH);

    g_game_state.enemies = new Entity[ENEMY_COUNT];
    
    for (int i=0; i<ENEMY_COUNT; ++i){
        g_game_state.enemies[i].set_entity_type(ENEMY);
        g_game_state.enemies[i].m_texture_id = enemy_texture_id;
        g_game_state.enemies[i].set_speed(.5f);
        g_game_state.enemies[i].set_acceleration(glm::vec3(0.0f, -10.0f, 0.0f));
        g_game_state.enemies[i].set_movement(glm::vec3(0.0f));
    }
    
    g_game_state.enemies[0].set_ai_type(FLOAT);
    g_game_state.enemies[0].set_width(.5f);
    g_game_state.enemies[0].set_acceleration(glm::vec3(0.0f, -10.0f, 0.0f));
    g_game_state.enemies[0].set_position(glm::vec3(.98f, -3.0f, 0.0f));
    
    
    g_game_state.enemies[1].set_ai_type(PACE);
    g_game_state.enemies[1].set_speed(1.5f);
    g_game_state.enemies[1].set_position(glm::vec3(9.0f, -5.0f, 0.0f));
    
    g_game_state.enemies[2].set_ai_type(GUARD);
    g_game_state.enemies[2].set_ai_state(IDLE);
    g_game_state.enemies[2].set_position(glm::vec3(16.8f, 4.0f, 0.0f));
    
    //font
    g_font_texture_id = load_texture(FONT_FILEPAHT);
    
    // ––––– AUDIO STUFF ––––– //
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

    g_game_state.bgm = Mix_LoadMUS(BGM_FILEPATH);
//    add jumping effect here
    Mix_PlayMusic(g_game_state.bgm, -1);
    Mix_VolumeMusic(MIX_MAX_VOLUME / 4.0f);


    // ––––– GENERAL ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    
}

void process_input()
{
    g_game_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_game_is_running = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q:
                // Quit the game with a keystroke
                g_game_is_running = false;
                break;

            case SDLK_SPACE:
                // Jump
                if (g_game_state.player->m_collided_bottom)
                {
                    g_game_state.player->m_is_jumping = true;
                }
                break;

            case SDLK_m:
                // Mute volume
                Mix_HaltMusic();
                break;

            default:
                break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT])
    {
        g_game_state.player->move_left();
        g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->LEFT];
    }
    else if (key_state[SDL_SCANCODE_RIGHT])
    {
        g_game_state.player->move_right();
        g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->RIGHT];
    }

    // This makes sure that the player can't move faster diagonally
    if (glm::length(g_game_state.player->get_movement()) > 1.0f)
    {
        g_game_state.player->set_movement(glm::normalize(g_game_state.player->get_movement()));
    }
}

void update()
{
    if (!g_game_is_actually_running){
        sleep(5);
        g_game_is_running = false;
    }
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_time_accumulator;

    if (delta_time < FIXED_TIMESTEP)
    {
        g_time_accumulator = delta_time;
        return;
    }

    while (delta_time >= FIXED_TIMESTEP)
    {
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.player, g_game_state.enemies, 3, g_game_state.map);
        
        for (int i = 0; i < ENEMY_COUNT; i++) {
            g_game_state.enemies[i].update(FIXED_TIMESTEP, g_game_state.player, g_game_state.player, 1, g_game_state.map);
        }

        delta_time -= FIXED_TIMESTEP;
    }
    g_time_accumulator = delta_time;
//    std::cout<<g_game_state.player->get_position().x<<g_game_state.player->get_position().y<<std::endl;
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    g_game_state.map->render(&g_shader_program);
    if (g_game_state.enemies[0].get_is_active()){
        draw_text(&g_shader_program, g_font_texture_id, std::string("Go a bit left to kill floater"), .4f, 0.0f, glm::vec3(4.0f, -1.0f, 0.0f));
    }
    else{
        draw_text(&g_shader_program, g_font_texture_id, std::string("kill the rest by jumping on top"), .4f, 0.0f, glm::vec3(4.0f, 0.0f, 0.0f));
        draw_text(&g_shader_program, g_font_texture_id, std::string("or lure them to fall into void"), .4f, 0.0f, glm::vec3(4.0f, -1.0f, 0.0f));
    }
    if (g_game_state.enemies[2].get_position().y <= -7.0f){
        g_game_state.enemies[2].deactivate();
    }
    if (g_game_state.player->get_position().y <= -7.0f){
        g_game_state.player->deactivate();
    }
    if (g_game_state.player->get_position().x <-0.4 ){
        g_game_state.enemies[0].deactivate();
    }
    int g_enemies_dead = 0;
    for (int i = 0; i < ENEMY_COUNT; i++){
        if (!g_game_state.enemies[i].get_is_active()){
            g_enemies_dead += 1;
        }
    }
    if (g_enemies_dead == ENEMY_COUNT){
        draw_text(&g_shader_program, g_font_texture_id, std::string("You Win"), .4f, 0.0f, glm::vec3(9.0f, -3.0f, 0.0f));
        g_game_is_actually_running = false;
    }
    if (g_game_state.player->get_is_active()){
        g_game_state.player->render(&g_shader_program);
    }
    else{
        draw_text(&g_shader_program, g_font_texture_id, std::string("You Lose"), .4f, 0.0f, glm::vec3(9.0f, -3.0f, 0.0f));
        g_game_is_actually_running = false;
    }
    
    for (int i = 0; i < ENEMY_COUNT; i++){
        if (g_game_state.enemies[i].get_is_active()){
            g_game_state.enemies[i].render(&g_shader_program);
        }
    }
    
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();

    delete[] g_game_state.enemies;
    delete    g_game_state.player;
    delete    g_game_state.map;
    Mix_FreeMusic(g_game_state.bgm);
    
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
