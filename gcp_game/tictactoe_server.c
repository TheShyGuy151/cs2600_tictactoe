#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <mosquitto.h>

#define BROKER_HOST "localhost"
#define BROKER_PORT 1883

#define TOPIC_P1_MOVE "ttt/move/player1"
#define TOPIC_P2_MOVE "ttt/move/player2"
#define TOPIC_STATE   "ttt/state"
#define TOPIC_RESET   "ttt/reset"
#define TOPIC_MODE    "ttt/mode"

static char board[9];
static char current_player = 'X';
static int game_over = 0;
static int one_player_mode = 0;
static struct mosquitto *mosq = NULL;

void init_board(void) {
    for (int i = 0; i < 9; i++) {
        board[i] = '1' + i;
    }
    current_player = 'X';
    game_over = 0;
}

char check_winner(void) {
    int wins[8][3] = {
        {0,1,2}, {3,4,5}, {6,7,8},
        {0,3,6}, {1,4,7}, {2,5,8},
        {0,4,8}, {2,4,6}
    };

    for (int i = 0; i < 8; i++) {
        int a = wins[i][0];
        int b = wins[i][1];
        int c = wins[i][2];

        if (board[a] == board[b] && board[b] == board[c]) {
            return board[a];
        }
    }

    int full = 1;
    for (int i = 0; i < 9; i++) {
        if (board[i] >= '1' && board[i] <= '9') {
            full = 0;
            break;
        }
    }

    if (full) {
        return 'D';
    }

    return '\0';
}

void publish_state(void) {
    char winner = check_winner();
    char msg[256];

    if (winner == 'X' || winner == 'O') {
        game_over = 1;
        snprintf(msg, sizeof(msg),
                 "BOARD:%c%c%c%c%c%c%c%c%c;TURN:%c;STATUS:WINNER_%c;MODE:%d",
                 board[0], board[1], board[2],
                 board[3], board[4], board[5],
                 board[6], board[7], board[8],
                 current_player, winner, one_player_mode);
    } else if (winner == 'D') {
        game_over = 1;
        snprintf(msg, sizeof(msg),
                 "BOARD:%c%c%c%c%c%c%c%c%c;TURN:%c;STATUS:DRAW;MODE:%d",
                 board[0], board[1], board[2],
                 board[3], board[4], board[5],
                 board[6], board[7], board[8],
                 current_player, one_player_mode);
    } else {
        snprintf(msg, sizeof(msg),
                 "BOARD:%c%c%c%c%c%c%c%c%c;TURN:%c;STATUS:PLAYING;MODE:%d",
                 board[0], board[1], board[2],
                 board[3], board[4], board[5],
                 board[6], board[7], board[8],
                 current_player, one_player_mode);
    }

    mosquitto_publish(mosq, NULL, TOPIC_STATE, strlen(msg), msg, 0, false);
    printf("%s\n", msg);
    fflush(stdout);
}

void print_board(void) {
    printf("\n");
    printf(" %c | %c | %c\n", board[0], board[1], board[2]);
    printf("---+---+---\n");
    printf(" %c | %c | %c\n", board[3], board[4], board[5]);
    printf("---+---+---\n");
    printf(" %c | %c | %c\n", board[6], board[7], board[8]);
    printf("\n");
}

void handle_move(char player, const char *payload) {
    if (game_over) {
        return;
    }

    if (player != current_player) {
        return;
    }

    int pos = atoi(payload);

    if (pos < 1 || pos > 9) {
        return;
    }

    int index = pos - 1;

    if (board[index] == 'X' || board[index] == 'O') {
        return;
    }

    board[index] = player;

    if (check_winner() == '\0') {
        current_player = current_player == 'X' ? 'O' : 'X';
    }

    print_board();
    publish_state();
}

void on_connect(struct mosquitto *m, void *userdata, int rc) {
    if (rc == 0) {
        printf("Connected to MQTT broker.\n");

        mosquitto_subscribe(m, NULL, TOPIC_P1_MOVE, 0);
        mosquitto_subscribe(m, NULL, TOPIC_P2_MOVE, 0);
        mosquitto_subscribe(m, NULL, TOPIC_RESET, 0);
        mosquitto_subscribe(m, NULL, TOPIC_MODE, 0);

        publish_state();
    } else {
        printf("Connection failed: %d\n", rc);
    }
}

void on_message(struct mosquitto *m, void *userdata, const struct mosquitto_message *msg) {
    char payload[128];

    memset(payload, 0, sizeof(payload));

    if (msg->payloadlen >= (int)sizeof(payload)) {
        return;
    }

    memcpy(payload, msg->payload, msg->payloadlen);
    payload[msg->payloadlen] = '\0';

    if (strcmp(msg->topic, TOPIC_P1_MOVE) == 0) {
        handle_move('X', payload);
    } else if (strcmp(msg->topic, TOPIC_P2_MOVE) == 0) {
        handle_move('O', payload);
    } else if (strcmp(msg->topic, TOPIC_RESET) == 0) {
        init_board();
        publish_state();
    } else if (strcmp(msg->topic, TOPIC_MODE) == 0) {
        if (strcmp(payload, "1") == 0) {
            one_player_mode = 1;
        } else {
            one_player_mode = 0;
        }
        init_board();
        publish_state();
    }
}

int main(void) {
    mosquitto_lib_init();

    mosq = mosquitto_new("gcp_tictactoe_server", true, NULL);

    if (!mosq) {
        fprintf(stderr, "Failed to create mosquitto client.\n");
        return 1;
    }

    init_board();

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);

    if (mosquitto_connect(mosq, BROKER_HOST, BROKER_PORT, 60) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Could not connect to MQTT broker.\n");
        return 1;
    }

    printf("Tic-Tac-Toe server running on GCP.\n");
    mosquitto_loop_forever(mosq, -1, 1);

    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return 0;
}
