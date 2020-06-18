#ifndef GLOBALH
#define GLOBALH

// #define _GNU_SOURCE
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <vector>

/* boolean */
#define TRUE 1
#define FALSE 0

#define BODY 1
#define HEAD 3
#define TAIL 3
extern int DESKS;
extern int DRAGONS;

/* używane w wątku głównym, determinuje jak często i na jak długo zmieniają się stany */
#define STATE_CHANGE_PROB 50
#define SEC_IN_STATE 2

#define ROOT 0

/* stany procesu */
typedef enum {mission_wait, mission_have, desk_wait, desk_have, cooperator_wait, dragon_wait, dragon_have, prof_wait, mission_bored} state_t;
extern state_t state;
extern int rank;
extern int size;
extern std::vector <int> missions, cooperators;
extern std::vector <struct packet_t> coop_mis;
extern int deskCount, dragonCount;
extern int ackDesk, ackDragon;
extern int lamport;
extern int first, last;
extern int currentMission;
extern int timeRequest;
extern int ackMission;
extern pthread_mutex_t requestMut;


/* to może przeniesiemy do global... */
struct packet_t {
    int id;
    int mission;  /* id zlecenia */
    int timeLamport;       /* timestamp (zegar lamporta) */
    int timeRequest;
    int data;     /* przykładowe pole z danymi; można zmienić nazwę na bardziej pasującą */
    packet_t *next = 0;
};
extern packet_t recvPacket, myPacket;

struct list {
    packet_t *first;
    void addPacket(packet_t packet);
    void deletePacket(packet_t packet);
    void deleteFirstPacket();
    list();
};

extern list *allAck, *missionsReq, *desksReq, *dragonsReq;


/* Typy wiadomości */
#define MISSION_AD 1
#define MISSION_REQ 2
#define MISSION_ACK 3
#define MISSION_HAVE 4
#define DESK_REQ 5
#define DESK_ACK 6
#define DRAGON_REQ 7
#define DRAGON_ACK 8
#define DRAGON_KILL 9
#define DRAGON_READY 10
#define DRAGON_RELEASE 11
#define DESK_RELEASE 12

/* macro debug - działa jak printf, kiedy zdefiniowano
   DEBUG, kiedy DEBUG niezdefiniowane działa jak instrukcja pusta 
   
   używa się dokładnie jak printfa, tyle, że dodaje kolorków i automatycznie
   wyświetla rank

   w związku z tym, zmienna "rank" musi istnieć.

   w printfie: definicja znaku specjalnego "%c[%d;%dm [%d]" escape[styl bold/normal;kolor [RANK]
            FORMAT:argumenty doklejone z wywołania debug poprzez __VA_ARGS__
			"%c[%d;%dm"       wyczyszczenie atrybutów    27,0,37
        UWAGA:
            27 == kod ascii escape. 
            Pierwsze %c[%d;%dm np 27[1;10m definiuje styl i kolor literek
            Drugie   %c[%d;%dm czyli 27[0;37m przywraca domyślne kolory i brak pogrubienia (bolda)
            ...  w definicji makra oznacza, że ma zmienną liczbę parametrów
                                            
*/
#ifdef DEBUG
#define debug(FORMAT,...) printf("%c[%d;%dm [%d]: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, ##__VA_ARGS__, 27,0,37);
#else
#define debug(...) ;
#endif

void sendPacket(packet_t *pkt, int destination, int tag);
void changeState( state_t );
void lamport_time(int, int);
//void list::addPacket(packet_t);
//void list::deletePacket(packet_t);
//void list::deleteFirstPacket();

#endif
