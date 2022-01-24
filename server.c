#include "../include/server.h"
#include "../include/helpers.h"
#include "../include/resources.h"

#include <ctype.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

user_buf user_buffer;
auction_buf auction_buffer;
job_buf job_buffer;
int next_auc_id = 1;
int listen_fd;
int tick_length;
int job_count;

pthread_mutex_t buffer_lock; 
char* buffer;
char *msgbuf="";

void sigint_handler(int sig)
{
    printf("shutting down server\n");
    close(listen_fd);
    exit(0);
}

void process_job(Job *j){
    char* buf_to_write = (char*)malloc(sizeof(char)*900);
    int ret;
    petr_header write_back;
    char *saveptr;
    
    switch(j->job_header){
        case ANLIST:
            sem_wait(&auction_buffer.mutex);
            
            anlist(buf_to_write);
            
            sem_post(&auction_buffer.mutex);

            write_back.msg_len = strlen(buf_to_write)+1;
            write_back.msg_type =ANLIST;
            break;
        case ANCREATE:
            
            sem_wait(&auction_buffer.mutex);
            char *name, *d,*price;
            //remove_trailing_zeros(j->str);
            name= strtok_r(j->str, "\n", &saveptr);
            d = strtok_r(NULL, "\n", &saveptr);
            price = strtok_r(NULL, "\n", &saveptr);
            int type = ancreate(name, d, price, getUser(j->client_fd));
            sem_post(&auction_buffer.mutex);
            if(type == EINVALIDARG){
               write_back.msg_type= EINVALIDARG;
               write_back.msg_len =0;
               buf_to_write ="";
            }else{
                sprintf(buf_to_write, "%d",type);
                printf("%s\n",buf_to_write);
                write_back.msg_len = strlen(buf_to_write)+1;
                write_back.msg_type =ANCREATE;
            }
            break;
        case ANWATCH:
            sem_wait(&auction_buffer.mutex);
            
            char *id;
            id= strtok_r(j->str, "\n", &saveptr);
            
            write_back.msg_type = anwatch(atoi(id), getUser(j->client_fd),buf_to_write);
            sem_post(&auction_buffer.mutex);
            write_back.msg_len = strlen(buf_to_write)+1; 
            break;
        case ANLEAVE:
            
            sem_wait(&auction_buffer.mutex);
            char * i;
            i = strtok_r(j->str, "\n", &saveptr);
             
            
            //write_back.msg_type=anleave(atoi(i), getUser(j->client_fd));

            Auction* auc = getAuction(atoi(i));
            
            write_back.msg_type = OK;
            
            if (auc == NULL) {
                write_back.msg_type= EANNOTFOUND;
                write_back.msg_len = 0;
                buf_to_write="";
                break;
            }
            
            
            // remove user from watch list of the auction
            int index = 0;
            node_t* curr = auc->users_watching->head;
            while (curr != NULL) {
                User* u = ((User*)curr->data);
                if (u->username == getUser(j->client_fd)->username) {
                    removeByIndex(auc->users_watching, index);
                    break;
                }
                curr = curr->next;
                index++;
            }

            write_back.msg_len = 0;
           
            buf_to_write="";
            sem_post(&auction_buffer.mutex);
          
            break;
        case ANBID:
            sem_wait(&auction_buffer.mutex);
            char * item,*bid;
            
            item = strtok_r(j->str, "\n", &saveptr);
            bid = strtok_r(NULL, "\n", &saveptr);
            int bid_result = anbid(atoi(item), atoi(bid),getUser(j->client_fd));
            
            sem_post(&auction_buffer.mutex);
            write_back.msg_type=bid_result;
            write_back.msg_len = 0;
            buf_to_write="";
            char* message = (char*)malloc(sizeof(char)*700);

            if(j->job_header ==ANBID && bid_result == OK){//if we bid and the
                get_auc_bid( item,getUser(j->client_fd)->username,bid,message);
                job_buf_insert(ANUPDATE,message,j->client_fd);
                
            }
            break;
        case ANUPDATE:
            write_back.msg_type=j->job_header;
            write_back.msg_len = strlen(j->str)+1;
            buf_to_write=j->str;
            anupdate(write_back,buf_to_write);
            break;
        case ANCLOSED:
            write_back.msg_type=j->job_header;
            write_back.msg_len = strlen(j->str)+1;
            buf_to_write=j->str;
            anclosed(write_back,buf_to_write);
            break;
        case USRWINS:
            sem_wait(&user_buffer.mutex);
            char* allwins = usrwins(getUser(j->client_fd),j->str);
            sem_post(&user_buffer.mutex);
            write_back.msg_type=USRWINS;
            write_back.msg_len = strlen(allwins);
            
            buf_to_write=allwins;
            break;
        case USRLIST:
            sem_wait(&user_buffer.mutex);
            
            usrlist(getUser(j->client_fd),buf_to_write);
            
            sem_post(&user_buffer.mutex);
            write_back.msg_type=USRLIST;
            write_back.msg_len = strlen(buf_to_write)+1;
            break;
        case USRSALES:
            sem_wait(&user_buffer.mutex);
            
            usrsales(getUser(j->client_fd),buf_to_write);
            
            sem_post(&user_buffer.mutex);
            write_back.msg_type=USRSALES;
            write_back.msg_len = strlen(buf_to_write)+1;
            break;
        case USRBLNC:
            sem_wait(&user_buffer.mutex);
            
            int total =usrblnc(getUser(j->client_fd));
            sprintf(buf_to_write,"%d",total);
            sem_post(&user_buffer.mutex);
            write_back.msg_type=USRBLNC;
            write_back.msg_len = strlen(buf_to_write)+1;
            break;
            
        

    }
    
    if(j->job_header!=ANUPDATE && j->job_header!=ANCLOSED){
      
        pthread_mutex_lock(&buffer_lock);    
        ret = wr_msg(j->client_fd, &write_back, buf_to_write);
        pthread_mutex_unlock(&buffer_lock);
    }
    

    
    if (ret < 0){
        printf("Sending failed\n");
    }
    return;
}

//THIS FUNCTION INITIALIZES A SOCKET AN ENDPOINT FOR COMMUNICATION FOR THE SERVER
int server_init(int server_port){

    int sockfd;//socket fd
    struct sockaddr_in servaddr;//socket address

    // socket create and verification

    /*int socket(int domain, int type, int protocol)
    creates an endpoint for communication and returns a file
    descriptor that refers to that endpoint.*/
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(EXIT_FAILURE);
    }
    //Clear the entire space of server address so I can place it in cleanly
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(server_port);

    int opt = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char *)&opt, sizeof(opt))<0)
    {
	    perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed\n");
        exit(EXIT_FAILURE);
    }
    

    // Now server is ready to listen and verification
    if ((listen(sockfd, 1)) != 0) {
        printf("Listen failed\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Server listening on port: %d.\n", server_port);
    return sockfd;
}

//WHERE WE PARSE THE INPUT FROM THE CLIENT AND ADD TO JOB QUEUE
void *process_client(void* clientfd_ptr){
    int client_fd = *(int *)clientfd_ptr;
    free(clientfd_ptr);
    int received_size;
    fd_set read_fds;
    int retval,ret;
    char* buf_cpy = "";
    //malloc space so that we can always use it
    petr_header* header = (petr_header*)malloc(sizeof(petr_header));
    
    while(1){
        
        //Initializes the file descriptor set READ_FDS to have zero bits for all file descriptors.
        FD_ZERO(&read_fds);
        //Sets the bit for the file descriptor CLIENT_fd in the file descriptor set READ_FDS.
        FD_SET(client_fd, &read_fds);

        //indicates which of the specified file descriptors is ready for 
        //reading, ready for writing, or has an error condition pending
        retval = select(client_fd + 1, &read_fds, NULL, NULL, NULL);
       
        //ERROR  handling for selecting file descriptor to read /write
        if (retval!=1 && !FD_ISSET(client_fd, &read_fds)){
            printf("Error with select() function\n");
            break;
        }
       
        //lock so that the buffer and the socket don't change and read something
        //while another thread is doing the same

        //BUFFER NOTES BELOW
        /*
            the server has to listen one at a time to a client
            there can be multiple clients connected to it,but 
            to put stuff in the job queue we have to listen to the request
            of one client at a time
            this is why we block the mutex for header reading as well
            since that is also part of buffer
        */
       
        pthread_mutex_lock(&buffer_lock);
        
        //we read the petr header to see message type and length
        int success = rd_msgheader(client_fd, header);
       
        if(success<0){
            header->msg_len =0;
            header->msg_type = EINVALIDARG;
            ret = wr_msg(client_fd, header, buf_cpy);
            continue;
            if (ret < 0){
                printf("Sending failed\n");
                break;
            }
        }
        char buffer[header->msg_len];
        bzero(buffer, header->msg_len);
        if( header->msg_len>0){
            //we read the stuff in buffer with the given msg_len
            received_size = read(client_fd, buffer, header->msg_len);
            
            //we are in someways always listening so if we don't receive anything we have to skip
            if(received_size < 0){
                printf("Receiving failed\n");
                break;
            }else if(received_size == 0){
                continue;
            }
        }
        
       
        //THESE jobs aren't meant to be carried out by a job thread, only the client
        //as specified in the prompt        
       
        
        if(header->msg_type == LOGOUT){ //if type is a logout
           
            sem_wait(&user_buffer.mutex);
            int type =logout(client_fd);
            sem_post(&user_buffer.mutex);
            printf("Client exit\n");
            header->msg_type =type;
            
            header->msg_len = 0;           
            ret = wr_msg(client_fd, header, buffer);
            pthread_mutex_unlock(&buffer_lock); //unlock the buffer
            break;
        }else if(header->msg_type == LOGIN){//if type is login
            
            
            char *saveptr;
            char *username, *password;
            
            username = strtok_r(buffer, "\n", &saveptr);
            password = strtok_r(NULL, "\n", &saveptr);
           
            sem_wait(&user_buffer.mutex);
            int type =  verifyUser(username,password,client_fd);
            sem_post(&user_buffer.mutex);


            header->msg_type =type;
            
            header->msg_len = 0;           
            ret = wr_msg(client_fd, header, buffer);

            if (ret < 0){
                printf("Sending failed\n");
                pthread_mutex_unlock(&buffer_lock); //unlock the buffer
                break;
            }else if(header->msg_type!=OK){
                pthread_mutex_unlock(&buffer_lock); //unlock the buffer
                break;
            }
            
        }else{
            if(header->msg_len>0){
                buf_cpy = (char*)malloc(sizeof(char*)*header->msg_len);
                strcpy(buf_cpy,buffer);
            }
           
            //insert into job queue
            job_buf_insert(header->msg_type,buf_cpy,client_fd);
           
        }
    
        pthread_mutex_unlock(&buffer_lock); //unlock the buffer
       
    }
    // Close the socket at the end
    printf("Close current client connection\n");
    close(client_fd);
    return NULL;
}

//INITIATE THE SERVER SOCKET AND THE CLIENT STUFF AND LISTEN/ACCEPT CONNECTIONS
void run_server(int server_port){
    // Initiate server and start listening on specified port
	listen_fd = server_init(server_port); 

    //make the client of the socket, we specify the fid for client
    // the address and length of the address
	int client_fd;
    struct sockaddr_in client_addr;
    socklen_t clientlen;

    //this is the pthread id
    pthread_t tid;

    //we keep listening for the client
    while(1){
        // Wait and Accept the connection from client

        //this gets the length of the client address
        clientlen=sizeof(struct sockaddr_storage);

        //we dynamically allocate the fid for client end of socket
        int* client_fd = malloc(sizeof(int));

        //we accept the client
        *client_fd = accept(listen_fd, (SA*)&client_addr, &clientlen);
        
        if (*client_fd < 0) {
            printf("server acccept failed\n");
            exit(EXIT_FAILURE);
        }
        else{
            //we accep the client connection
            printf("Client connetion accepted\n");

            //we create a client thread
            pthread_create(&tid, NULL, process_client, (void *)client_fd); 
        }
    }

    //we clear the buffer connnection and we stop listening
    bzero(buffer, BUFFER_SIZE);
    close(listen_fd);
    return;
}

// this function parses the input passed to run the server
unsigned int establish_port(int input_size, char * requested_port[],List_t* auctions){
	
    int opt;
	int N=2;//initialize the number of jobs, 2 is default
	int M=0;//initialize number of ticks, default is to wait from stdin for ticks
    unsigned int port = 0;
   	int opts_indicated=1;//iterate through the commands i recieved in argv
    int x,y;
    while ((opt = getopt(input_size, requested_port, "hjt")) != -1) {
    	
        switch (opt) {
       
        case 'h'://this case displays help menu
        	opts_indicated++;
        	display_help_menu();
        	break; //haveing trouble here
        case 'j'://this case displays number of jobs
        	x = opts_indicated+1;
        	if(strlen(requested_port[x])==1 && *requested_port[x]>47 && *requested_port[x]<58){
        		opts_indicated = x;
        		N = atoi(requested_port[x]);
        	}
       		//display_num_job(N);
            job_count = N;
            opts_indicated++;// if num is specified opt++
       		break;
        case 't'://this case displays number of ticks
        	y = opts_indicated+1;
        	if(strlen(requested_port[y])==1 && *requested_port[y]>47 && *requested_port[y]<58){
        		opts_indicated = y;
        		M = atoi(requested_port[y]);
        	}
       		//tick_space(M);
            tick_length =M;
            opts_indicated++;//if M specified opt++
       		break;
        }
        
    }
    
    //set the port equal to specification
   	port = atoi(requested_port[opts_indicated++]);
           
    // before we start the server we need to parse the AUCTION FILE pars does so
    pars(requested_port[opts_indicated++],auctions);
   	
    //error check if port is 0 the port number was never reassigned
    if (port == 0){
        fprintf(stderr, "ERROR: Port number for server to listen is not given\n");
        fprintf(stderr, "Server Application Usage: %s -p <port_number>\n",
                    requested_port[0]);
        exit(EXIT_FAILURE);
    }
   

	return port;
}

void *thread(void *vargp)
{
    pthread_detach(pthread_self());//we can't do this
    while (1) {
        Job* job = job_buf_remove(); /* Remove job from buf */
        process_job(job);
    }
}
void* tik(void* t){
    pthread_detach(pthread_self());//we can't do this
    while (1) {
        sleep(tick_length); /* Remove job from buf */
        decrementTicks();
    }
}

int main(int argc, char* argv[]) {
     
    job_buf_init();
    user_buf_init();
    auction_buf_init();
    job_count =2;
    //MAKE THE WORKERS
    tick_length =2;
    pthread_t tid;
    pthread_t tik_tid;
    pthread_create(&tik_tid,NULL,tik,NULL);
    // initialize the queue
    //we specify NTHREADS
    for (int i = 0; i < job_count; i++){ //Create worker threads 
        pthread_create(&tid, NULL, thread, NULL);
    }

    unsigned int port = establish_port(argc,argv,auction_buffer.auctions_list);
    run_server(port);
    return 0;
}
