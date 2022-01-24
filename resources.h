#ifndef RESOURCES_H
#define RESOURCES_H
#include <errno.h>
#include <string.h>
#include <semaphore.h>
#include "../include/protocol.h"
#include "linkedList.h"


// User struct stores data for login info, list of won auctions, and 
typedef struct user {
    char* username;
    char* password;
    int client_fd;
    char* file_descriptor;
    List_t* auctions_created;
    List_t* won_auctions;
    int in_use;           // 0 if inactive 1 if active
} User;

// auction struct - contains auction id, pointer to associated user, remaining ticks,
// and array of watching users
typedef struct auction {
    int auc_id;
    char* item_name;
    struct user* created_by;
    int remaining_ticks;
    int bin_price;
    int highest_bid;
    int active;
    User* winning_user;
    List_t* users_watching;
} Auction;

typedef struct job {
    int job_header;
    char* str;
    int client_fd;
} Job;

// user buffer
typedef struct {
    List_t* users_list;
    sem_t mutex;
} user_buf;

// auction buffer
typedef struct {
    List_t* auctions_list;
    sem_t mutex;
} auction_buf;

// job buffer
// TODO add jobs
typedef struct {
    List_t* jobs_list;
    sem_t mutex;
    sem_t items;// available number of jobs
} job_buf;

extern user_buf user_buffer;        // buffer for users
extern auction_buf auction_buffer;   // buffer for auctions
extern job_buf job_buffer;           // buffer for jobs
extern int next_auc_id;             // next available auction ID starting with 1, requires Mutex

/*
* Checks for unique username/password in list of users
* return -1 if username exists but password does not match (EWRNGPWD)
* return 0 if username does not exist or the user exists but is not active (OK)
* return 1 if username exists and is logged in (EUSRLGDIN)
*
* @param username and passwords for user to campare to recorded users 
*/
int verifyUser(char* user, char* pass,int client_fd);


User* getUser(int socket);

int compareUsers(User* u1, User* u2);


void auction_buf_init();
void user_buf_init();

// prints current auctions in list for debugging
void printAuctions();

//prints content of users in buffer for debugging
void printUsers();

// prints job headers for debugging
void printJobs();

// all the job buffer functions
void job_buf_init();
void job_buf_deinit();
void job_buf_insert(int header, char* buf,int cl_fd);
void* job_buf_remove();







#endif
