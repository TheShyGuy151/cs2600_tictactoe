#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <mosquitto.h>

#define BROKER_HOST "cppcs2600mqtt.duckdns.org"
#define BROKER_PORT 1883

#define TOPIC_P1_MOVE "ttt/move/player1"
#define TOPIC_P2_MOVE "ttt/move/player2"
#define TOPIC_STATE   "ttt/state"
#define TOPIC_RESET   "ttt/reset"
#define TOPIC_MODE    "ttt/mode"

static struct mosquitto *mosq = NULL;
static char board[10] = "123456789";
static char turn = 'X';
static char status[64] = "WAITING";
static int mode = 2;

void parse_state(char *msg) {
    char *board_ptr = strstr(msg, "BOARD:");
    char *turn_ptr = strstr(msg, "TURN:");
    char *status_ptr = strstr(msg, "STATUS:");
    char *mode_ptr = strstr(msg, "MODE:");

    if (board_ptr) {
        board_ptr += 6;
        for (int i = 0; i < 9; i++) {
            board[i] = board_ptr[i];
        }
        board[9] = '\0';
    }

    if (turn_ptr) {
        turn_ptr += 5;
        turn = turn_ptr[0];
    }

    if (status_ptr) {
        status_ptr += 7;
        int i = 0;
        while (status_ptr[i] != ';' && status_ptr[i] != '\0' && i < 63) {
            status[i] = status_ptr[i];
            i++;
        }
        status[i] = '\0';
    }

    if (mode_ptr) {
        mode_ptr += 5;
        mode = atoi(mode_ptr);
    }
}

void on_message(struct mosquitto *m, void *userdata, const struct mosquitto_message *msg) {
    char payload[256];

    if (msg->payloadlen >= (int)sizeof(payload)) {
        return;
    }

    memcpy(payload, msg->payload, msg->payloadlen);
    payload[msg->payloadlen] = '\0';

    if (strcmp(msg->topic, TOPIC_STATE) == 0) {
        parse_state(payload);
    }
}

void publish_move(int position) {
    char move[8];
    snprintf(move, sizeof(move), "%d", position);

    if (turn == 'X') {
        mosquitto_publish(mosq, NULL, TOPIC_P1_MOVE, strlen(move), move, 0, false);
    } else {
        mosquitto_publish(mosq, NULL, TOPIC_P2_MOVE, strlen(move), move, 0, false);
    }
}

void publish_simple(const char *topic, const char *msg) {
    mosquitto_publish(mosq, NULL, topic, strlen(msg), msg, 0, false);
}

int main(void) {
    mosquitto_lib_init();

    mosq = mosquitto_new("raylib_laptop_client", true, NULL);

    if (!mosq) {
        printf("Could not create MQTT client.\n");
        return 1;
    }

    mosquitto_message_callback_set(mosq, on_message);

    if (mosquitto_connect(mosq, BROKER_HOST, BROKER_PORT, 60) != MOSQ_ERR_SUCCESS) {
        printf("Could not connect to MQTT broker.\n");
        return 1;
    }

    mosquitto_subscribe(mosq, NULL, TOPIC_STATE, 0);
    mosquitto_loop_start(mosq);

    InitWindow(600, 700, "CS2600 MQTT Tic-Tac-Toe");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("MQTT Tic-Tac-Toe", 150, 30, 32, BLACK);

        char info[128];
        snprintf(info, sizeof(info), "Turn: %c   Status: %s   Mode: %d-player", turn, status, mode);
        DrawText(info, 80, 90, 20, DARKGRAY);

        int start_x = 90;
        int start_y = 150;
        int cell = 140;

        for (int i = 0; i < 9; i++) {
            int row = i / 3;
            int col = i % 3;
            int x = start_x + col * cell;
            int y = start_y + row * cell;

            Rectangle rect = {x, y, cell, cell};
            DrawRectangleLinesEx(rect, 4, BLACK);

            char text[2];
            text[0] = board[i];
            text[1] = '\0';

            Color color = DARKGRAY;

            if (board[i] == 'X') {
                color = BLUE;
            } else if (board[i] == 'O') {
                color = RED;
            }

            DrawText(text, x + 50, y + 40, 60, color);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mouse = GetMousePosition();

                if (CheckCollisionPointRec(mouse, rect)) {
                    if (board[i] >= '1' && board[i] <= '9') {
                        publish_move(i + 1);
                    }
                }
            }
        }

        Rectangle reset_btn = {90, 600, 120, 50};
        Rectangle one_btn = {240, 600, 120, 50};
        Rectangle two_btn = {390, 600, 120, 50};

        DrawRectangleRec(reset_btn, LIGHTGRAY);
        DrawRectangleRec(one_btn, LIGHTGRAY);
        DrawRectangleRec(two_btn, LIGHTGRAY);

        DrawRectangleLinesEx(reset_btn, 2, BLACK);
        DrawRectangleLinesEx(one_btn, 2, BLACK);
        DrawRectangleLinesEx(two_btn, 2, BLACK);

        DrawText("Reset", 120, 615, 22, BLACK);
        DrawText("1P", 285, 615, 22, BLACK);
        DrawText("2P", 435, 615, 22, BLACK);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetMousePosition();

            if (CheckCollisionPointRec(mouse, reset_btn)) {
                publish_simple(TOPIC_RESET, "reset");
            }

            if (CheckCollisionPointRec(mouse, one_btn)) {
                publish_simple(TOPIC_MODE, "1");
            }

            if (CheckCollisionPointRec(mouse, two_btn)) {
                publish_simple(TOPIC_MODE, "2");
            }
        }

        EndDrawing();
    }

    CloseWindow();

    mosquitto_loop_stop(mosq, true);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return 0;
}
