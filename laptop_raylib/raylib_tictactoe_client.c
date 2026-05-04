#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <mosquitto.h>

#define BROKER_PORT 1883

#define TOPIC_P1_MOVE "ttt/move/player1"
#define TOPIC_P2_MOVE "ttt/move/player2"
#define TOPIC_STATE   "ttt/state"
#define TOPIC_RESET   "ttt/reset"
#define TOPIC_MODE    "ttt/mode"

static struct mosquitto *mosq = NULL;

static char broker_host[128] = "";
static bool connected = false;
static bool typing_url = true;

static char board[10] = "123456789";
static char turn = 'X';
static char status[64] = "NOT_CONNECTED";
static int mode = 2;

void parse_state(char *msg) {
    char *board_ptr = strstr(msg, "BOARD:");
    char *turn_ptr = strstr(msg, "TURN:");
    char *status_ptr = strstr(msg, "STATUS:");
    char *mode_ptr = strstr(msg, "MODE:");

    if (board_ptr) {
        board_ptr += 6;
        for (int i = 0; i < 9; i++) board[i] = board_ptr[i];
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

    if (msg->payloadlen >= (int)sizeof(payload)) return;

    memcpy(payload, msg->payload, msg->payloadlen);
    payload[msg->payloadlen] = '\0';

    if (strcmp(msg->topic, TOPIC_STATE) == 0) {
        parse_state(payload);
    }
}

bool connect_to_broker(void) {
    mosq = mosquitto_new("raylib_laptop_client", true, NULL);

    if (!mosq) {
        strcpy(status, "MQTT_CLIENT_ERROR");
        return false;
    }

    mosquitto_message_callback_set(mosq, on_message);

    if (mosquitto_connect(mosq, broker_host, BROKER_PORT, 60) != MOSQ_ERR_SUCCESS) {
        strcpy(status, "CONNECT_FAILED");
        mosquitto_destroy(mosq);
        mosq = NULL;
        return false;
    }

    mosquitto_subscribe(mosq, NULL, TOPIC_STATE, 0);
    mosquitto_loop_start(mosq);

    connected = true;
    strcpy(status, "CONNECTED");

    return true;
}

void publish_move(int position) {
    if (!connected || mosq == NULL) return;

    char move[8];
    snprintf(move, sizeof(move), "%d", position);

    if (turn == 'X') {
        mosquitto_publish(mosq, NULL, TOPIC_P1_MOVE, strlen(move), move, 0, false);
    } else {
        mosquitto_publish(mosq, NULL, TOPIC_P2_MOVE, strlen(move), move, 0, false);
    }
}

void publish_simple(const char *topic, const char *msg) {
    if (!connected || mosq == NULL) return;
    mosquitto_publish(mosq, NULL, topic, strlen(msg), msg, 0, false);
}

void handle_url_input(void) {
    int key = GetCharPressed();

    while (key > 0) {
        int len = strlen(broker_host);

        if (len < 127 && key >= 32 && key <= 126) {
            broker_host[len] = (char)key;
            broker_host[len + 1] = '\0';
        }

        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        int len = strlen(broker_host);
        if (len > 0) {
            broker_host[len - 1] = '\0';
        }
    }

    if (IsKeyPressed(KEY_ENTER)) {
        if (strlen(broker_host) > 0) {
            connect_to_broker();
        }
    }
}

int main(void) {
    mosquitto_lib_init();

    InitWindow(700, 760, "CS2600 MQTT Tic-Tac-Toe");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("MQTT Tic-Tac-Toe", 190, 25, 32, BLACK);

        DrawText("Enter GCP IP or DuckDNS URL:", 70, 80, 22, BLACK);

        Rectangle input_box = {70, 115, 560, 45};
        DrawRectangleLinesEx(input_box, 2, BLACK);
        DrawText(broker_host, 80, 127, 22, DARKBLUE);

        if (!connected) {
            DrawText("Press ENTER to connect", 70, 170, 20, DARKGRAY);
            DrawText(status, 70, 200, 20, RED);
            handle_url_input();
        } else {
            DrawText("Connected to MQTT broker", 70, 170, 20, DARKGREEN);
        }

        char info[160];
        snprintf(info, sizeof(info), "Turn: %c   Status: %s   Mode: %d-player", turn, status, mode);
        DrawText(info, 70, 230, 20, DARKGRAY);

        int start_x = 140;
        int start_y = 280;
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
            if (board[i] == 'X') color = BLUE;
            if (board[i] == 'O') color = RED;

            DrawText(text, x + 50, y + 40, 60, color);

            if (connected && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mouse = GetMousePosition();

                if (CheckCollisionPointRec(mouse, rect)) {
                    if (board[i] >= '1' && board[i] <= '9') {
                        publish_move(i + 1);
                    }
                }
            }
        }

        Rectangle reset_btn = {110, 710, 120, 40};
        Rectangle one_btn = {290, 710, 120, 40};
        Rectangle two_btn = {470, 710, 120, 40};

        DrawRectangleRec(reset_btn, LIGHTGRAY);
        DrawRectangleRec(one_btn, LIGHTGRAY);
        DrawRectangleRec(two_btn, LIGHTGRAY);

        DrawRectangleLinesEx(reset_btn, 2, BLACK);
        DrawRectangleLinesEx(one_btn, 2, BLACK);
        DrawRectangleLinesEx(two_btn, 2, BLACK);

        DrawText("Reset", 140, 720, 20, BLACK);
        DrawText("1 Player", 310, 720, 20, BLACK);
        DrawText("2 Player", 490, 720, 20, BLACK);

        if (connected && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
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

    if (mosq != NULL) {
        mosquitto_loop_stop(mosq, true);
        mosquitto_destroy(mosq);
    }

    mosquitto_lib_cleanup();
    CloseWindow();

    return 0;
}
