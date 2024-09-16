#include "utils/Raylib/raylib.h"
#include "utils/Raylib/raymath.h"
#include <stdio.h>
#include <stdlib.h>

#define NODE_RADIUS 5 

#define MONKEY_RADIUS 30
#define MONKEY_SPEED 10.0 
#define MONKEY_SHOOT_TIME 0.5

#define BULLET_WIDTH 5
#define BULLET_HEIGHT 3
#define BULLET_SPEED 20.0 

#define BALLON_RADIUS 8 
#define BALLOON_SPEED 1.0f 
#define BALLOON_SPAWN_TIME 5.0f 

#define SCREEN_WIDTH 1200 
#define SCREEN_HEIGHT 800 

typedef struct Node Node;
typedef struct Monkey Monkey;
typedef struct Bullet Bullet;
typedef struct Ballon Ballon;
typedef struct Game Game;

typedef enum {
  TITLE_SCREEN,
  STAGE_CREATION,
  IN_PLAY,
  GAME_OVER,
  WIN_GAME,
} GameState;

typedef enum {
  SELECTED,
  DESELECTED,
} MonkeyState;

typedef enum {
  VISIBLE,
  INVISIBLE,
} NodeState; 

typedef enum {
  ALIVE,
  DEAD,
} BallonState;

const Color balloonColors[] = {
  RED, ORANGE, YELLOW, GREEN, BLUE, PURPLE, PINK
};

typedef struct Node {
  unsigned int id; 
  Vector2 pos;
  NodeState state;
  void (*draw_node)(Game *game, Node *node);
} Node;

typedef struct Monkey {
  unsigned int id;
  Vector2 pos;
  unsigned int border_radius;
  unsigned int radius;
  MonkeyState state; 
  float shoot_timer;
  void (*draw_monkey)(Game *game, Monkey *monkey);
} Monkey;

typedef struct Bullet {
  Vector2 pos;
  Vector2 direction;
  unsigned int damage;
  unsigned int pierce;
  float speed;
  void (*draw_bullet)(struct Bullet *bullet);
} Bullet;

typedef struct Ballon {
  unsigned int id;
  Vector2 pos;
  int target_node;
  BallonState state;
  unsigned int layers;         
  Color color;        
  void (*update_ballon)(Game *game, Ballon *ballon);
} Ballon;

typedef struct Game {
  GameState curr_state;
  int score;

  unsigned int lives;
  unsigned int monkey_count;

  Monkey *monkeys;
  Monkey *new_monkey; 

  unsigned int bullet_count;
  Bullet *bullets;

  unsigned int node_count;
  Node *nodes;  

  unsigned int ballon_count;
  Ballon *ballons;

  float ballon_spawn_timer;
} Game;

Monkey *init_monkey(unsigned int id, unsigned int radius, unsigned int x, unsigned int y, void (*draw)(Game *game, Monkey *monkey)) {
  Monkey *monkey = malloc(sizeof(Monkey));
  if (!monkey) {
    fprintf(stderr, "Failed to allocate memory for Monkey.\n");
    exit(EXIT_FAILURE);
  }
  monkey->id = id;
  monkey->radius = radius;
  monkey->pos.x = x;
  monkey->pos.y = y;
  monkey->border_radius = radius * 2; 
  monkey->state = DESELECTED;
  monkey->draw_monkey = draw;

  return monkey;
}

void draw_monkey(Game *game, Monkey *monkey) {
  if (monkey->state == SELECTED) {
    DrawCircleV(monkey->pos, monkey->radius, BLACK);
  } else if (monkey->state == DESELECTED) {
    DrawCircleV(monkey->pos, monkey->border_radius, LIGHTGRAY);
    DrawCircleV(monkey->pos, monkey->radius, BLACK);
    DrawCircleLines(monkey->pos.x, monkey->pos.y, monkey->radius, RED); 
  }
}

void draw_bullet(Bullet *bullet) {
  DrawRectangleV(bullet->pos, (Vector2){BULLET_WIDTH, BULLET_HEIGHT}, BLACK); 
}

void update_bullet(Bullet *bullet) {
  if (bullet->pierce > 0) {
    bullet->pos = Vector2Add(bullet->pos, Vector2Scale(bullet->direction, bullet->speed));

    if (bullet->pos.x < 0 || bullet->pos.x > SCREEN_WIDTH || bullet->pos.y < 0 || bullet->pos.y > SCREEN_HEIGHT) {
      bullet->pierce--;
    }
  }
}

Bullet *init_bullet(Vector2 start_pos, Vector2 direction, unsigned int damage, unsigned int pierce, float speed) {
  Bullet *bullet = malloc(sizeof(Bullet));
  if (!bullet) {
    fprintf(stderr, "Failed to allocate memory for Bullet.\n");
    exit(EXIT_FAILURE);
  }
  bullet->pos = start_pos;
  bullet->direction = Vector2Normalize(direction);
  bullet->damage = damage;
  bullet->pierce = pierce;
  bullet->speed = speed;
  bullet->draw_bullet = draw_bullet;

  return bullet;
}

void fire_bullet(Game *game, Monkey *monkey, Vector2 target_pos) {
  Vector2 direction = Vector2Subtract(target_pos, monkey->pos);
  Bullet *bullet = init_bullet(monkey->pos, direction, 10, 1, BULLET_SPEED); 
  game->bullet_count++;
  game->bullets = realloc(game->bullets, sizeof(Bullet) * game->bullet_count);
  game->bullets[game->bullet_count - 1] = *bullet;
  free(bullet);
}

void draw_node(Game *game, Node *node) {
  if (node->state == VISIBLE) {
    DrawCircleV(node->pos, NODE_RADIUS, BLACK);
  }
}

Node *init_node(unsigned int id, Vector2 position) {
  Node *node = malloc(sizeof(Node));
  if (!node) {
    fprintf(stderr, "Failed to allocate memory for Node.\n");
    exit(EXIT_FAILURE);
  }
  node->id = id;
  node->pos = position;
  node->state = VISIBLE;
  node->draw_node = draw_node;
  return node;
}

void draw_ballon(Game *game, Ballon *ballon) {
  if (ballon->state == ALIVE) {
    DrawCircleV(ballon->pos, BALLON_RADIUS, ballon->color);
  }
}

Ballon *init_ballon(unsigned int id, Vector2 position, int target_node, int layers, Color color) {
  Ballon *ballon = malloc(sizeof(Ballon));
  if (!ballon) {
    fprintf(stderr, "Failed to allocate memory for Ballon.\n");
    exit(EXIT_FAILURE);
  }

  ballon->id = id;
  ballon->pos = position;
  ballon->target_node = target_node;
  ballon->layers = layers;  
  ballon->color = color; 
  ballon->state = ALIVE;

  return ballon;
}

void update_ballon(Game *game, Ballon *ballon) {
  if (ballon->state == ALIVE) {
    if (ballon->target_node < game->node_count) {
      Vector2 target_pos = game->nodes[ballon->target_node].pos;

      if (Vector2Distance(ballon->pos, target_pos) < 1.0f) {
        ballon->pos = target_pos; 
        ballon->target_node++; 
        if (ballon->target_node >= game->node_count) {
          ballon->state = DEAD; 
        }
      } else {
        Vector2 direction = Vector2Normalize(Vector2Subtract(target_pos, ballon->pos));
        ballon->pos = Vector2Add(ballon->pos, Vector2Scale(direction, BALLOON_SPEED));
      }
    }
  }
}

Game *init_game(void) {
  Game *game = malloc(sizeof(Game));
  if (!game) {
    fprintf(stderr, "Failed to allocate memory for Game.\n");
    exit(EXIT_FAILURE);
  }

  game->curr_state = TITLE_SCREEN;
  game->lives = 3;
  game->score = 0;

  game->monkeys = NULL;
  game->monkey_count = 0;
  game->new_monkey = NULL;

  game->bullet_count = 0;
  game->bullets = NULL;

  game->nodes = NULL;  
  game->node_count = 0;

  game->ballon_count = 0;
  game->ballons = NULL;
  game->ballon_spawn_timer = BALLOON_SPAWN_TIME;

  return game;
}

void draw_title_screen(void) {
  DrawText("Click to Start", SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2 - 10, 20, DARKGRAY);
}

void update_game(Game *game) {
  Vector2 mouse_pos = GetMousePosition();

  switch (game->curr_state) {
    case TITLE_SCREEN:
      draw_title_screen();

      if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        game->curr_state = STAGE_CREATION;
      }
      break;

    case STAGE_CREATION:
      if (IsKeyPressed(KEY_N)) {
        game->node_count++;
        Node *node = init_node(game->node_count, mouse_pos);
        game->nodes = realloc(game->nodes, sizeof(Node) * game->node_count);
        game->nodes[game->node_count - 1] = *node;
        free(node);
      }

      if (IsKeyPressed(KEY_SPACE)) {
        if (game->node_count > 1) {
          game->curr_state = IN_PLAY;
        } else {
          free(game->nodes);
          game->nodes = NULL;
          game->node_count = 0;
        }
        break;
      }

      for (unsigned int i = 0; i < game->node_count; i++) {
        game->nodes[i].draw_node(game, &game->nodes[i]);
      }

      for (unsigned int i = 0; i < game->node_count; i++) {
        DrawLineV(game->nodes[i].pos, game->nodes[(i + 1) % game->node_count].pos, BLACK);
      }

      DrawText("SPACE TO START", SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2 - 10, 20, DARKGRAY);
      break;

    case IN_PLAY:
      game->ballon_spawn_timer += GetFrameTime();
      if (game->ballon_spawn_timer >= BALLOON_SPAWN_TIME) {
        game->ballon_spawn_timer = 0;
        game->ballon_count++;

        unsigned int layers = 1;
        Ballon *ballon = init_ballon(game->ballon_count, game->nodes[0].pos, 0, layers, balloonColors[layers - 1]);
        game->ballons = realloc(game->ballons, sizeof(Ballon) * game->ballon_count);
        game->ballons[game->ballon_count - 1] = *ballon;
        free(ballon);
      }

      for (unsigned int i = 0; i < game->ballon_count; i++) {
        update_ballon(game, &game->ballons[i]);
        draw_ballon(game, &game->ballons[i]);
      }

      for (unsigned int i = 0; i < game->bullet_count; i++) {
        update_bullet(&game->bullets[i]);
        game->bullets[i].draw_bullet(&game->bullets[i]);
      }

      for (unsigned int i = 0; i < game->bullet_count; i++) {
        Bullet *bullet = &game->bullets[i];

        for (unsigned int j = 0; j < game->ballon_count; j++) {
          Ballon *ballon = &game->ballons[j];

          if (ballon->state == ALIVE && CheckCollisionRecs(
            (Rectangle){bullet->pos.x, bullet->pos.y, BULLET_WIDTH, BULLET_HEIGHT},
            (Rectangle){ballon->pos.x - BALLON_RADIUS, ballon->pos.y - BALLON_RADIUS, BALLON_RADIUS * 2, BALLON_RADIUS * 2})) {

            if (ballon->layers > 0) {
              ballon->layers--;
              if (ballon->layers == 0) {
                ballon->state = DEAD;  
              } else {
                ballon->color = balloonColors[ballon->layers - 1];  
              }
            }

            bullet->pierce--;
            if (bullet->pierce == 0) {
              for (unsigned int k = i; k < game->bullet_count - 1; k++) {
                game->bullets[k] = game->bullets[k + 1];
              }
              game->bullet_count--;
              break;
            }
          }
        }
      }

      if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        for (unsigned int i = 0; i < game->monkey_count; i++) {
          Monkey *monkey = &game->monkeys[i];
          float distance = Vector2Distance(mouse_pos, monkey->pos);
          if (distance <= monkey->radius) {
            monkey->state = DESELECTED;
          } else {
            monkey->state = SELECTED;
          }
        }
      }

      if (IsKeyPressed(KEY_M)) {
        game->monkey_count++;
        Monkey *monkey = init_monkey(game->monkey_count, MONKEY_RADIUS, mouse_pos.x, mouse_pos.y, draw_monkey);
        game->monkeys = realloc(game->monkeys, sizeof(Monkey) * game->monkey_count);
        game->monkeys[game->monkey_count - 1] = *monkey;
        free(monkey);
      }

      for (unsigned int i = 0; i < game->monkey_count; i++) {
        Monkey *monkey = &game->monkeys[i];
        monkey->shoot_timer += GetFrameTime();

        for (unsigned int j = 0; j < game->ballon_count; j++) {
          Ballon *ballon = &game->ballons[j];
          if (ballon->state == ALIVE) {
            float distance = Vector2Distance(monkey->pos, ballon->pos);
            if (distance <= monkey->border_radius && monkey->shoot_timer >= MONKEY_SHOOT_TIME) {  
              fire_bullet(game, monkey, ballon->pos);

              monkey->shoot_timer = 0; 
              break;  
            }
          }
        }
        game->monkeys[i].draw_monkey(game, monkey);
      }
      break;

    case GAME_OVER:
      // Handle game over logic
      break;

    case WIN_GAME:
      // Handle win game logic
      break;

    default:
      break;
  }
}

int main(void) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "BTD Style Game");

  Game *game = init_game();

  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    update_game(game);

    EndDrawing();
  }

  free(game->monkeys);
  free(game->nodes);
  free(game->ballons);
  free(game);

  CloseWindow();

  return 0;
}


