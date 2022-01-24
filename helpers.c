#include "../include/helpers.h"
#include "protocol.h"
#include <stdlib.h>




void display_help_menu(){
	printf("we need to display a help menu\n");
	//just prints something
	return;
}
void display_num_job(int N){
	printf("we need to display the number of jobs\n");
	//prints the number of jobs default to 2
	return;
}
void tick_space(int M){
	printf("tick tok\n");
	//M times between ticks
	return;
}

int op_in(char* file){
	int fd=0;
	if(file!=NULL){
		if ((fd = open(file,O_RDONLY)) < 0) {
			
			return -1;
		}
	}
	return fd;
}

void pars(char* file,List_t* auctions){
	char* line =NULL;
	size_t len =0;
	
	FILE *stream = fopen(file, "r");
	
	if(stream==NULL){
		printf("unable to open file\n");
		return;
	}
	
	//get line also reads the number of characte read inludeing the delimiter character but no including the terminating null byte
	//i have to add this to auction list
	
	int i =0;


    // initialize users watching list
   
	Auction* a = (Auction*)malloc(sizeof(Auction));
	a->users_watching = create_list();
	//while the initial file still has stuff to add
	while(getline(&line,&len,stream)!=-1){
		
        if(line[0]=='\n'){//when the first charcter is \n we know that the
			i=0;
			
            a = (Auction*)malloc(sizeof(Auction));
            a->users_watching = create_list();
            
			continue;
		}else if(i==0){//first line is item name
            
			char * name = (char *)malloc(len*sizeof(char));
			strcpy(name,line);
			a->item_name = name;
			
		}else if(i==1){//then the remaining ticks
			a->remaining_ticks = atoi(line);
			//printf("%s",line);
		}else if(i==2){//finally the bin_price
			a->bin_price = atoi(line);
			//printf("%s",line);
            a->auc_id = next_auc_id;
            next_auc_id++;
            a->active =1;
            insertRear(auctions,a);
		}
        // set auction ID and increment next available ID
		i++;
	}
  
	return;
}


void remove_trailing_zeros(char * change){
    char *p = strrchr(change, '\0')-1;
            
    if (p != NULL) {
        *p = '\0';
    }
    p = strrchr(change, '\0')-1;
    if (p != NULL) {
        *p = '\0';
    }
}

// removes newline and crriage return from string
void formatString(char* buffer) {
    char* p = strchr(buffer, '\n');
    if (p != NULL) {
        *p = '\0';
    }

    p = strchr(buffer, '\r');
    if (p != NULL) {
        *p = '\0';
    }
}

// returns auction associated with id or NULL
Auction* getAuction(int id) {
    node_t* curr = auction_buffer.auctions_list->head;
    while (curr != NULL) {
        if (id == ((Auction*)curr->data)->auc_id) {
            Auction* auc = ((Auction*)curr->data);
            return auc;
        }
        curr = curr->next;
    }
    return NULL;
}

// changes user to inactive. Returns -1 if not successful else OK
int logout(int cl) {
    
    // if user is not active, return failure
    node_t* curr = user_buffer.users_list->head;
    while (curr != NULL) {
        User* usr = ((User*)curr->data);
        if(usr->client_fd==cl){
            
            if (usr->in_use == 0) {
                return -1;
            }
            usr->in_use = 0;
            return OK;

        }
       
        curr = curr->next;
    }
    return -1;
}

// setn by client to create auction - adds auction to list
// returns the new auction's ID on success
// returns 
int ancreate(char* name, char* duration, char* b_price, User* u) {
    if ((strlen(name) == 0) | (strlen(duration) == 0) | (strlen(b_price) == 0)) {
        return EINVALIDARG;
    }
    int d = atoi(duration);
    int p = atoi(b_price);
   
    if((p<-1)|(d<-1)){
        return EINVALIDARG;
    }
    
    // create auction, initialize users_watching linked list, and 
    // add the auction to auctions buffer
    Auction* new_auc = malloc(sizeof(Auction));
    new_auc->users_watching = create_list();
    new_auc->created_by = u;
    new_auc->auc_id = next_auc_id;
    next_auc_id++;
    new_auc->item_name = name;
    new_auc->active=1;
    new_auc->remaining_ticks = d;
    new_auc->bin_price = p;
    new_auc->highest_bid = 0;
    insertRear(auction_buffer.auctions_list, new_auc);
    insertRear(u->auctions_created, new_auc);
    return new_auc->auc_id;
}

// sent by client to server. server responds with list of running auctions
// if none, server responds with ANLIST
// auctions ordered lexographically ascending - id;name;bin;highest bid;num watchers;cycles remaining
char* anlist(char* str_buffer) {
    if (auction_buffer.auctions_list->length == 0) {
        return NULL;    // return null if there are no running auctions
    }
    char* colon = ";";
    char* newline = "\n";
    char* terminate = "\0";
    node_t* curr_auc = auction_buffer.auctions_list->head;    // start at head
    
    while (curr_auc != NULL) {       
                                 // for each auction
        Auction* auc = ((Auction*)curr_auc->data);
        if(auc->active ==1){
            // do string stuff
            char new_str[700];
            char buffer[700];
            sprintf(buffer, "%s", auc->item_name);
            formatString(buffer);

            sprintf(new_str, "%d;%s;%d;%d;%d;%d\n", auc->auc_id, buffer, auc->bin_price, 
                auc->highest_bid, auc->users_watching->length, auc->remaining_ticks);

            strcat(str_buffer, new_str);
        }
        curr_auc=curr_auc->next;
    }
    strcat(str_buffer, terminate);
    return str_buffer;
}

// sent by client to server to watch a running auction - adds user to auction watch list
// returns NULL if <auction_id> does not exist (EANNOTFOUND)
// returns the name of auction on success (ANWATCH)
int anwatch(int id, User* u,char * new_str) {
    Auction* auc = getAuction(id);
    if (auc == NULL) {
        return EANNOTFOUND;
    }
    // add user to acution watch list
    insertRear(auc->users_watching, u);
    
    char buffer[30];
    char * nwlin = "\n";
    strcpy(new_str, auc->item_name);
    strcat(new_str,nwlin);

    sprintf(buffer, "%d", auc->bin_price);
    strcat(new_str,buffer);
    
    return ANWATCH;
}

// sent by client to server to stop watchhing auction - removes user from auction watch list
// returns NULL if auc id does not exist (EANNOTFOUND)
// returns the auction on success (OK)
enum msg_types anleave(int id, User* u) {
    Auction* auc = getAuction(id);
    if (auc == NULL) {
        return EANNOTFOUND;
    }
    int i = 0;
    node_t* curr_u = auc->users_watching->head;
    while (curr_u != NULL) {
        if (compareUsers(u, ((User*)curr_u->data)) == 0) {
            removeByIndex(auc->users_watching, i);
        }
        curr_u = curr_u->next;
        i++;
    }
    return OK;
}

void anupdate(petr_header write_back, char * buf_to_write){
    char id = buf_to_write[0];
    int x = id - '0';
    int ret;
    node_t* curr_user = getAuction(x)->users_watching->head;
    User * user;
    while(curr_user!=NULL){
        user = (User*)curr_user->data;
         
        ret = wr_msg(user->client_fd, &write_back, buf_to_write);
        curr_user=curr_user->next; 
   }
}


void anclosed(petr_header write_back, char * buf_to_write){
    char id = buf_to_write[0];
    int x = id - '0';
    int ret;
    node_t* curr_user = getAuction(x)->users_watching->head;
    User * user;
    while(curr_user!=NULL){
        user = (User*)curr_user->data;
         
        ret = wr_msg(user->client_fd, &write_back, buf_to_write);
        curr_user=curr_user->next; 
    }
    free(buf_to_write); 
}

void get_auc_bid(char* item,char* username,char *bid,char *new_str){
    Auction* auc = getAuction(atoi(item));
    formatString(item);
    formatString(auc->item_name);
    formatString(username);
    char* newline = "\r\n";
    strcpy(new_str, item);
    strcat(new_str, newline);
    strcat(new_str, auc->item_name);
    strcat(new_str, newline);
    strcat(new_str, username);
    strcat(new_str, newline);
    strcat(new_str, bid);
}
// sent by client to make a bid on an auction
// if auction id does not exist return (EANNOTFOUND)
// if auction exists but user is not watching or created the auction return (EANDENIED)
// if bid is lower than current highest bid in auction return (EBIDLOW)
// if everything good return OK and update auction
int anbid(int id, int bid, User* u) {
    Auction* auc = getAuction(id);
    if (auc == NULL) {
        return EANNOTFOUND;
    }
    
    // check if user created the auction
    if (compareUsers(u, auc->created_by) == 0) {
        return EANDENIED;
    }
    // if bid is below highest bid for auction
   
    if (bid < auc->highest_bid) {
        return EBIDLOW;
    }
    // if bid is higher than buy it now  price
    if (bid > auc->bin_price) {
        // TODO close auction - remove from list and add to won_auctions for user
        auc->highest_bid = bid;
        auc->active = 0;
        insertRear(u->won_auctions, auc);
        // TODO send anclose to all users watching aucton
    }
    // update bid
    auc->highest_bid = bid;
    auc->winning_user = u;
    return OK;
}

void usrlist(User* usr, char* str_buffer) {
    
    if (user_buffer.users_list->length == 0) {
          // return null if there are no running auctions
        return;
    }

    char* newline = "\n";
    char* terminate = "\0";
    node_t* curr = user_buffer.users_list->head;
    while (curr != NULL) {
        User* u = ((User*)curr->data);
        char buffer[50];
        if ((u->in_use == 1) && (u->username != usr->username)) {
            sprintf(buffer, "%s", u->username);
            formatString(buffer);
            strcat(str_buffer, buffer);
            strcat(str_buffer,newline);

        }
        curr = curr->next;
    }
    strcat(str_buffer, terminate);
    
}

// sent by server - string of text with each auction won by the user
char* usrwins(User* u, char* str_buffer) {
    if (u->won_auctions->length == 0) {
        return NULL;
    }
    
    char* semicolon = ";";
    char* newline = "\n";
    char* terminate = "\0";
    node_t* curr = u->won_auctions->head;
    while (curr != NULL) {
        Auction* auc = ((Auction*)curr->data);
        char new_str[100];
        char buffer[30];
        sprintf(buffer, "%s", auc->item_name);
        formatString(buffer);
        sprintf(new_str, "%d;%s;%d\n", auc->auc_id, buffer, auc->highest_bid);

        strcat(str_buffer, new_str);

    }
    strcat(str_buffer, terminate);
    return str_buffer;
}



// returs list of complete auctions created by the user
void usrsales(User* u, char* str_buffer) {
    node_t* curr_auc = u->auctions_created->head;
    char* terminate = "\0";
    while (curr_auc != NULL) {
        Auction* auc = ((Auction*)curr_auc->data);
        char buffer[50];
        char new_str[500];
        if (auc->active == 0) {
            sprintf(buffer, "%s", auc->item_name);
            formatString(buffer);
            sprintf(new_str, "%d;%s;%d\n", auc->auc_id, buffer, auc->highest_bid); 

            strcat(str_buffer, new_str);
        }
        curr_auc = curr_auc->next;
    }
    strcat(str_buffer, terminate);

}

// returns net difference between all winning bid prices the user created
// minus the sum of winning bid prices for auctions the user won
// balance does not include unfinished auctions
int usrblnc(User* u) {
    int money_won = 0;
    int money_lost = 0;
    Auction* auc;
    // loop through auctions the user created and made money from
    node_t* curr_created = u->auctions_created->head;
    while (curr_created != NULL) {
        auc = ((Auction*)curr_created->data);
        if (auc->active == 0) {
            money_won += auc->highest_bid;
        }
        curr_created = curr_created->next;
    }

    // loop through won auctions
    node_t* curr_won = u->won_auctions->head;
    while (curr_won != NULL) {
        auc = ((Auction*)curr_won->data);
        money_lost += auc->highest_bid;
        curr_won = curr_won->next;
    }

    return money_won - money_lost;
}

// loops through auctions list and decrements ticks_remaining
void * decrementTicks() {
    
    sem_wait(&auction_buffer.mutex);
    node_t* curr = auction_buffer.auctions_list->head;
    while (curr != NULL) {
        Auction* auc = ((Auction*)curr->data);
        if(auc->active ==1){
            auc->remaining_ticks--;
        }
       
        if (auc->remaining_ticks ==0 && auc->active ==1) {
            
            // if auction is completed set as inactive
            auc->active = 0;
            // add to auctions_won list of the user that won had the highest bid
            if (auc->highest_bid > 0) {
                User* winning_user = auc->winning_user;
                insertRear(winning_user->won_auctions, auc);
                
                char * buf_cpy = (char*)malloc(sizeof(char*)*700);
                sprintf(buf_cpy,"%d\r\n%s\r\n%d",auc->auc_id,winning_user->username,auc->highest_bid);
                
           
                //insert into job queue
                job_buf_insert(ANCLOSED,buf_cpy,winning_user->client_fd);
            }else{
                char * buf_cpy = (char*)malloc(sizeof(char*)*700);
                sprintf(buf_cpy,"%d\r\n\r\n",auc->auc_id);
                job_buf_insert(ANCLOSED,buf_cpy,0);
            }
        }
        curr = curr->next;
    }
    sem_post(&auction_buffer.mutex);
    return NULL;
}












