#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "message.h"

#define MAX_RADIUS 350.00
#define MIN_ALTITUDE_DIFFERENCE 750.0

void initializeList(LinkedList* list) {
    list->head = NULL;
    list->size = 0;
}

void appendToList(LinkedList* list, ADSBPacket data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }
    newNode->data = data;
    newNode->next = NULL;

    if (list->head == NULL) {
        list->head = newNode;
    } else {
        Node* current = list->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }
    list->size++;
}

void removeFromList(LinkedList* list, int id) {
    Node* current = list->head;
    Node* prev = NULL;

    while (current != NULL) {
        if (current->data.id == id) {
            if (prev == NULL) {
                list->head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            list->size--;
            return;
        }
        prev = current;
        current = current->next;
    }
}

// 释放列表内存
void freeList(LinkedList* list) {
    Node* current = list->head;
    while (current != NULL) {
        Node* next = current->next;
        free(current);
        current = next;
    }
    list->head = NULL;
    list->size = 0;
}

struct Node* search(LinkedList* list, int id) {
    Node* current = list->head;
    while (current != NULL) {
        if (current->data.id == id) {
            return current;
        } else {
            current = current->next;
        }
    }
    return NULL;
}

void estimatePosition(LinkedList* list, int aircraftID, EstimationData* estimates) {
    Node* last = list->head;
    while (last != NULL) {
        if (last->data.id == aircraftID) {
            double speed_in_ms = last->data.speed * 3.6;
            double heading_radians = last->data.heading * M_PI / 180.0;
            double north_velocity = speed_in_ms * sin(heading_radians);
            double east_velocity = speed_in_ms * cos(heading_radians);
            int delta_time_seconds = estimates->time_seconds - last->data.time_seconds;
            estimates->north_est = last->data.north + north_velocity * delta_time_seconds / 3600.0;
            estimates->east_est = last->data.east + east_velocity * delta_time_seconds / 3600.0;
            return;
        }
        last = last->next;
    }
    estimates->north_est = 0.0;
    estimates->east_est = 0.0;
}

int main() {
    LinkedList aircraftList;
    initializeList(&aircraftList);

    char line[256];

    while (fgets(line, sizeof(line), stdin) != NULL) {
        if (strncmp(line, "#ADS-B:", 7) == 0) {
            ADSBPacket packet;
            int hours, minutes;
            if (sscanf(line, "#ADS-B:%d,time:%d:%d,N:%lf,E:%lf,alt:%d,head:%lf,speed:%lf",
                       &packet.id, &hours, &minutes, &packet.north, &packet.east,
                       &packet.altitude, &packet.heading, &packet.speed) == 8) {
                packet.time_seconds = hours * 3600 + minutes * 60;
                removeFromList(&aircraftList, packet.id);
                appendToList(&aircraftList, packet);
            }
        } else if (line[0] == '*') {
            int hour, minute, aircraftID;
            char requestID[10];
            sscanf(line, "*time:%d:%d,%[^,],%d", &hour, &minute, requestID, &aircraftID);
            if (strncmp(requestID, "close", 5) == 0) {
                printf("closing\n");
                freeList(&aircraftList);
                exit(0);
            } else if (strcmp(requestID, "est_pos") == 0) {
                EstimationData estimates;
                estimates.id = aircraftID;
                estimates.time_seconds = hour * 3600 + minute * 60;
                estimatePosition(&aircraftList, aircraftID, &estimates);

                if (estimates.north_est == 0.0 && estimates.east_est == 0.0) {
                    printf("Aircraft (ID:%d) not currently in area of operation\n", aircraftID);
                } else {
                    double distance_est = sqrt(pow(estimates.east_est, 2) + pow(estimates.north_est, 2));
                    if (distance_est > MAX_RADIUS) {
                        printf("Aircraft (ID:%d) not currently in area of operation\n", aircraftID);
                    } else {
                        printf("Aircraft (ID:%d): Estimated Position: N:%.1f,E:%.1f\n", aircraftID, estimates.north_est, estimates.east_est);
                    }
                }
            }else if (strncmp(requestID, "num_contacts", 11) == 0) {
                int tracking = 0;
                int current_time_seconds = hour * 3600 + minute * 60;

                Node* current = aircraftList.head;
                while (current != NULL) {
                    double speed_in_ms = current->data.speed * 3.6; // Convert speed to m/s
                    double heading_radians = current->data.heading * M_PI / 180.0;
                    double north_velocity = speed_in_ms * sin(heading_radians);
                    double east_velocity = speed_in_ms * cos(heading_radians);
                    int delta_time_seconds = current_time_seconds - current->data.time_seconds;

                    double north_est = current->data.north + north_velocity * delta_time_seconds / 3600.0;
                    double east_est = current->data.east + east_velocity * delta_time_seconds / 3600.0;
                    double distance_est = sqrt(pow(east_est, 2) + pow(north_est, 2));

                    if (distance_est <= MAX_RADIUS) {
                        tracking++;
                    }
                    current = current->next;
                }

                printf("Currently tracking %d aircraft\n", tracking);
            }

        }
    }
    return 0;
}
