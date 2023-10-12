#ifndef MESSAGE_H
#define MESSAGE_H

typedef struct {
    int id;
    double time_seconds;
    double north;
    double east;
    int altitude;
    double heading;
    double speed;
} ADSBPacket;

typedef struct {
    int id;
    double east_est;
    double north_est;
    int time_seconds;
} EstimationData;

typedef struct Node {
    ADSBPacket data;
    struct Node* next;
} Node;

typedef struct {
    Node* head;
    int size;
} LinkedList;
#endif
