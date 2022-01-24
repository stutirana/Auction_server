#ifndef HELPERS_H
#define HELPERS_H

#include "../include/server.h"
#include "../include/protocol.h"
#include <string.h>

#include <errno.h>

#include <fcntl.h>

void display_help_menu();
void display_num_job(int N);
void tick_space(int M);
int op_in(char* file);
void pars(char* file,List_t* auctions);


void remove_trailing_zeros(char * change);
// formats string to remove carriage return and new line
void formatString(char* buffer);

// returns acution associated to auction id
Auction* getAuction(int id);

// logs use out by making them inactive
int logout(int cl);

// creates auction ands adds to auctions list
int ancreate(char* name, char* duration, char* b_price, User* u);

// returns string of running auctions
char* anlist();

// adds user to auction watch list
int anwatch(int id, User* u,char * new_str);

// removes user from auction watchlist
enum msg_types anleave(int id, User* u) ;

void get_auc_bid(char* item,char* username,char *bid,char *new_str);

// sent by client to make bid on auction
int anbid(int id, int bid, User* u);

void anclosed(petr_header write_back, char * buf_to_write);

void anupdate(petr_header write_back, char * buf_to_write);

// returns list of users as a string
void usrlist(User* u, char* str_buffer);

// returns list of auctions the user won
char* usrwins(User* u, char* str_buffer);

void usrsales(User* u, char* str_buffer);

int usrblnc(User* u);

void * decrementTicks();

#endif
