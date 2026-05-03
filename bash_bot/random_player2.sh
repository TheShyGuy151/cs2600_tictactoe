#!/bin/bash

BROKER="cppcs2600mqtt.duckdns.org"
STATE_TOPIC="ttt/state"
MOVE_TOPIC="ttt/move/player2"

while true
do
    STATE=$(mosquitto_sub -h "$BROKER" -t "$STATE_TOPIC" -C 1)

    BOARD=$(echo "$STATE" | sed -n 's/.*BOARD:\([XO1-9]*\);.*/\1/p')
    TURN=$(echo "$STATE" | sed -n 's/.*TURN:\([XO]\);.*/\1/p')
    STATUS=$(echo "$STATE" | sed -n 's/.*STATUS:\([^;]*\).*/\1/p')
    MODE=$(echo "$STATE" | sed -n 's/.*MODE:\([0-9]\).*/\1/p')

    if [ "$STATUS" != "PLAYING" ]; then
        sleep 1
        continue
    fi

    if [ "$MODE" != "1" ]; then
        sleep 1
        continue
    fi

    if [ "$TURN" != "O" ]; then
        sleep 1
        continue
    fi

    AVAILABLE=""

    for i in 1 2 3 4 5 6 7 8 9
    do
        CHAR=$(echo "$BOARD" | cut -c "$i")
        if [ "$CHAR" = "$i" ]; then
            AVAILABLE="$AVAILABLE $i"
        fi
    done

    COUNT=$(echo "$AVAILABLE" | wc -w)

    if [ "$COUNT" -gt 0 ]; then
        PICK_INDEX=$((RANDOM % COUNT + 1))
        MOVE=$(echo "$AVAILABLE" | awk "{print \$$PICK_INDEX}")
        echo "Bot chooses $MOVE"
        mosquitto_pub -h "$BROKER" -t "$MOVE_TOPIC" -m "$MOVE"
    fi

    sleep 1
done
