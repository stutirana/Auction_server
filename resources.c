#include <stdio.h>
#include <string.h>
#include "resources.h"
#include "../include/protocol.h"
//user_buf user_b ;
//auction_buf auction_b;

// verifies username and password
// if user does not exist, add to list
// on success return pointer to user struct
// if user already exists and is active, return null
int verifyUser(char* user, char* pass,int cl_fd) {

  
    //we are reading from buffer
    //sem_wait(&user_buffer.mutex);
    // search through users_list to verify
    node_t* curr = user_buffer.users_list->head;   // pointer to head of list
    //printf("%p\n",curr);
    while (curr != NULL) {
       
        // compare user and pass data to input
        //printf("the passed in username %s\n",user);
        //printf("the user name %s\n",((User*)curr->data)->username);
        int u = strcmp(user, ((User*)curr->data)->username);
        int p = strcmp(pass, ((User*)curr->data)->password);
        if (u == 0) {        // usernames match (existing)
            if (p == 0) {    // username + password match
                if (((User*)curr->data)->in_use == 0) {    // user is inactive
                    ((User*)curr->data)->in_use = 1;
                    //sem_post(&job_buffer.mutex);
                    return OK;
                } else {
                    //sem_post(&job_buffer.mutex);
                    return EUSRLGDIN;                // user exists but is active
                }
            }
           // sem_post(&user_buffer.mutex);
            return EWRNGPWD; // incorrect password
        }
        curr = curr->next;
    }

    // we are writing to buffer
    // no matches - add user to list
    User* new_user = malloc(sizeof(User));
    new_user->auctions_created = create_list();
    new_user->won_auctions = create_list();

    char * u = (char*)malloc(sizeof(char*)*strlen(user));
    strcpy(u,user);

    new_user->username = u;
    char * p = (char*)malloc(sizeof(char*)*strlen(pass));
    strcpy(p,pass);
    new_user->password = p;
    
    new_user->client_fd = cl_fd;
    new_user->in_use = 1;
    insertRear(user_buffer.users_list, new_user);

    // return pointer to new user
  
    return OK;
    
}



void auction_buf_init()
{
    sem_init(&auction_buffer.mutex, 0, 1); /* Binary semaphore for locking */
    auction_buffer.auctions_list = create_list();
   
}

void user_buf_init()
{
    sem_init(&user_buffer.mutex, 0, 1); /* Binary semaphore for locking */
    user_buffer.users_list = create_list();
}



// returns user based on socket and NULL if user does not exist
User* getUser(int socket) {
    node_t* curr = user_buffer.users_list->head;
    while (curr != NULL) {
        User* u = ((User*)curr->data);
        if (socket == u->client_fd) {
            return u;
        }
        curr = curr->next;
    }

    return NULL;
}


// returns OK if users are same else -1
int compareUsers(User* u1, User* u2) {
    if(u1==NULL || u2==NULL){//if no username assigned,
        return -1;
    }
    if (strcmp(u1->username, u2->username) == 0) {
        printf("\nequal!\n");
        return 0;
    }
    return -1;
}

// prints a list of auctions in auction list
void printAuctions() {

    if (auction_buffer.auctions_list->length == 0) {
        fprintf(stderr, "Auctions list empty\n");
        return;
    }

    // pointer to head of list
    node_t* curr = auction_buffer.auctions_list->head;
    while (curr != NULL) {
        Auction* auc = ((Auction*)curr->data);
        fprintf(stderr, "%d;%s;%d;%d\n", auc->auc_id, auc->item_name, auc->bin_price,
            auc->highest_bid); 
        curr = curr->next;
    }
}

// prints list of users from buffer
void printUsers() {

    if (user_buffer.users_list->length == 0) {
        fprintf(stderr, "User list empty\n");
        return;
    }

    node_t* curr = user_buffer.users_list->head;
    while (curr != NULL) {
        User* usr = ((User*)curr->data);
        fprintf(stderr, "%d\n", usr->client_fd);
        curr = curr->next;
    }
}


//JOB BUFFER FUNCTIONS

// prints job headers
void printJobs() {
    if (job_buffer.jobs_list->length == 0) {
        fprintf(stderr, "job queue empty");
        return;
    }

    node_t* curr = job_buffer.jobs_list->head;
    while (curr != NULL) {
        Job* job = ((Job*)curr->data);
        fprintf(stderr, "%d\n", job->job_header);
        curr = curr->next;
    }
}

//initialize the job buffer
void job_buf_init()
{
    sem_init(&job_buffer.mutex, 0, 1); /* Binary semaphore for locking */
    job_buffer.jobs_list = create_list();
    sem_init(&job_buffer.items, 0, 0); /* Initially, buf has 0 items */
}

//deinitialize this buffer
void job_buf_deinit(){
    deleteList(job_buffer.jobs_list);
}

//insert into the buffer
void job_buf_insert(int header, char* buf,int cl_fd) {
    sem_wait(&job_buffer.mutex); /* Lock the buffer */
    Job* new_job = malloc(sizeof(Job));
    new_job->job_header = header;
    new_job->str = buf;
    new_job->client_fd = cl_fd;
    // insert into job list
    //printf("inserting at head\n");
    
    insertFront(job_buffer.jobs_list, new_job); 
    
    sem_post(&job_buffer.mutex); /* Unlock the buffer */
    sem_post(&job_buffer.items); /* Announce available item */
}

//remove from buffer
void* job_buf_remove(){
    sem_wait(&job_buffer.items); /* Wait for available item */
    sem_wait(&job_buffer.mutex); /* Lock the buffer */
    Job* job = removeRear(job_buffer.jobs_list);
    sem_post(&job_buffer.mutex); /* Unlock the buffer */
    return job;
}






