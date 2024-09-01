#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#define NUM_ROOMS 3

typedef struct {
    int room_id;
    int capacity;
    int is_available;
    int is_projector_available;
    int is_whiteboard_available;
    int priority;
    pthread_mutex_t mutex;
} Room;

Room conference_rooms[NUM_ROOMS];
pthread_mutex_t room_mutex;
sem_t room_available;


typedef struct {
    int room_id;
    int priority;
} Reservation;

Reservation booking_system[NUM_ROOMS];
pthread_mutex_t booking_mutex;

void initialize_rooms() {
    for (int i = 0; i < NUM_ROOMS; i++) {
        conference_rooms[i].room_id = i + 1;
        conference_rooms[i].capacity = 10;
        conference_rooms[i].is_available = 1;
        conference_rooms[i].is_projector_available = 1;
        conference_rooms[i].is_whiteboard_available = 1;
        conference_rooms[i].priority = 1;
        pthread_mutex_init(&(conference_rooms[i].mutex), NULL);
    }
    pthread_mutex_init(&room_mutex, NULL);
    sem_init(&room_available, 0, NUM_ROOMS);
    pthread_mutex_init(&booking_mutex, NULL);
}

void reserve_room(int room_id, int priority) {
    Room *room = &conference_rooms[room_id - 1];


    sem_wait(&room_available);

    pthread_mutex_lock(&(room->mutex));

    if (room->is_available && room->is_projector_available && room->is_whiteboard_available) {
        room->is_available = 0;
        room->is_projector_available = 0;
        room->is_whiteboard_available = 0;
        room->priority = priority;
        printf("Room %d reserved successfully (Priority: %d).\n", room->room_id, priority);

        pthread_mutex_lock(&booking_mutex);
        booking_system[room_id - 1].room_id = room_id;
        booking_system[room_id - 1].priority = priority;
        pthread_mutex_unlock(&booking_mutex);

    } else {
        printf("Room %d is not available.\n", room->room_id);
    }

    pthread_mutex_unlock(&(room->mutex));
}

void release_room(int room_id) {
    Room *room = &conference_rooms[room_id - 1];

    pthread_mutex_lock(&(room->mutex));

    room->is_available = 1;
    room->is_projector_available = 1;
    room->is_whiteboard_available = 1;
    printf("Room %d released.\n", room->room_id);

    pthread_mutex_unlock(&(room->mutex));

    sem_post(&room_available);


    pthread_mutex_lock(&booking_mutex);
    booking_system[room_id - 1].room_id = 0;
    booking_system[room_id - 1].priority = 0;
    pthread_mutex_unlock(&booking_mutex);
}

void *simulate_conference(void *arg) {
    int room_id = *(int *)arg;
    int priority = rand() % 3 + 1;

    reserve_room(room_id, priority);


    printf("Conference in Room %d (Priority: %d) is ongoing...\n", room_id, priority);
    sleep(rand() % 5 + 1);
    release_room(room_id);
    pthread_exit(NULL);
}

void *urgent_meeting(void *arg) {
    int room_id = *(int *)arg;

    reserve_room(room_id, 3);
    printf("Urgent meeting in Room %d (Priority: 3) is ongoing...\n", room_id);
    sleep(2);

    release_room(room_id);

    pthread_exit(NULL);
}

int main() {
    initialize_rooms();

    pthread_t threads[NUM_ROOMS];
    int room_ids[NUM_ROOMS];


    for (int i = 0; i < NUM_ROOMS; i++) {
        room_ids[i] = i + 1;
        if (pthread_create(&threads[i], NULL, simulate_conference, &room_ids[i]) != 0) {
            perror("Thread creation failed");
            exit(EXIT_FAILURE);
        }
    }

    pthread_t urgent_meeting_thread;
    int urgent_room_id = rand() % NUM_ROOMS + 1;

    if (pthread_create(&urgent_meeting_thread, NULL, urgent_meeting, &urgent_room_id) != 0) {
        perror("Thread creation for urgent meeting failed");
        exit(EXIT_FAILURE);
    }


    for (int i = 0; i < NUM_ROOMS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Thread join failed");
            exit(EXIT_FAILURE);
        }
    }

    if (pthread_join(urgent_meeting_thread, NULL) != 0) {
        perror("Thread join for urgent meeting failed");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_destroy(&room_mutex);
    sem_destroy(&room_available);
    pthread_mutex_destroy(&booking_mutex);

    return 0;
}
