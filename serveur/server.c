#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>

#define RCVSIZE 1024



int create_socket(struct sockaddr_in *adresse_control , int port) {
  int valid = 1  ; 
  int server_desc_control = socket(AF_INET, SOCK_DGRAM, 0);
  if (server_desc_control < 0) {
    perror("Cannot create socket\n");
    return -1;
  }
  setsockopt(server_desc_control, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));

  adresse_control->sin_family= AF_INET;
  adresse_control->sin_port= htons(port);
  adresse_control->sin_addr.s_addr= htonl(INADDR_ANY);
  if (bind(server_desc_control, (struct sockaddr*) adresse_control, sizeof(*adresse_control)) == -1) {
    perror("Bind failed\n");
    close(server_desc_control);
    return -1;
  }
  return server_desc_control ;  

}


void wait_client(fd_set *readset , struct timeval *tv , int *pid_process , int *port_client_utilises , int *nombre_client , int *result_select , int *return_child , char *mode){
  if(strcmp(mode,"BEGIN") == 0 ) {
    do{
      FD_SET(3 , readset);
      *result_select = select(4, readset , NULL , NULL , tv); 
      *return_child = waitpid(-1 ,NULL,WNOHANG) ;
      if(*return_child != 0 && *return_child != -1 ) {
        for ( int j = 0 ; j<sizeof(pid_process)/sizeof(pid_process[0]) ; j++) {
          if(pid_process[j] == *return_child){
            port_client_utilises[j] = 0 ;
            pid_process[j] = 0 ;
            printf("pid_process : %i\n", pid_process[j]);
            *nombre_client--;
            break ; 

          }
        }

      }

    }while(*result_select == 0 );
  }
  else if (strcmp(mode,"SYN_ACK") == 0 ) {
      FD_SET(3 , readset);
      *result_select = select(4, readset , NULL , NULL , tv); 

  }

}


void reconnaissance_DATA(int *sequence,int *sequence_ACK , char *DATA_char , char *buffer_message){
  char *ret_SEQ ;
  char *ret_ACK ;  
    /*printf("\n \n \nDATA est : %s  \n \n \n", buffer_message);

    char *tok = strtok(buffer_message,";");

    while(tok != NULL)
    {
      if(strcmp(tok,"SEQ") == 0){
        tok = strtok(NULL , ";");
        if(atoi(tok) != 0){
          *sequence = atoi(tok);
        }
      }

      else if(strcmp(tok,"ACK") == 0)
      {
        tok = strtok(NULL , ";");
        if(atoi(tok) != 0){
          *sequence_ACK = atoi(tok);
        }

      }
      else if(strcmp(tok,"DATA") == 0 )
      {
        tok = strtok(NULL , ";");
        printf("\n \n \ntok est : %s  \n \n \n", tok);
        strcpy(DATA_char,tok);
      }
      tok = strtok(NULL , ";");
    }
    */

  ret_SEQ = strstr(buffer_message , "SEQ");
  ret_ACK = strstr(buffer_message , "ACK");
  char result[6];

  if(ret_SEQ != NULL){
    sscanf(ret_SEQ , "%*c %*c %*c %6s" , result);
    printf("resultats : %s \n", result);
    *sequence = atoi(result);

  }
  if(ret_ACK != NULL){
    sscanf(ret_ACK , " %*c %*c %*c %6s" , result);
    printf("resultats : %s \n", result);
    *sequence_ACK = atoi(result);


  }
  sleep(3);


}

void gestion_client_fork(int *server_desc_control , int *server_desc_message , struct sockaddr_in *adresse_message , int port , struct sockaddr_storage *serverStorage , socklen_t addr_size ) {


  struct sockaddr_in addr = {0} ;
  socklen_t addr_size2 = sizeof(addr);
  FILE *fp ; 
  char sequence_ACK_char[RCVSIZE];
  unsigned char DATA_char[RCVSIZE];
  char sequence_char[RCVSIZE];
  char buffer_message[RCVSIZE];
  int sequence = -1 ;
  int sequence_ACK = -1;
  int message ; 
  int NO_STOP =1 ;
  int error ; 
  close(*server_desc_control);
  fp = fopen("image.txt","ab");
  
  while(NO_STOP){
    //printf("coucou \n");
    memset(buffer_message, 0 , RCVSIZE);
    message = recvfrom(*server_desc_message,buffer_message,1024,0,(struct sockaddr *)&addr,&addr_size2);
    printf("le premier message reçu : %s \n", buffer_message);
    reconnaissance_DATA(&sequence,&sequence_ACK,DATA_char,buffer_message);
    sequence_ACK = sequence; 
    sequence = sequence +1  ; 
    sprintf(sequence_char,"%i",sequence);
    sprintf(sequence_ACK_char,"%i",sequence_ACK);
    printf("les données du clients sont : SEQ : %i , ACK %i , DATA %s \n", sequence , sequence_ACK , DATA_char);
    if(strcmp(DATA_char,"stop") == 0 ){
      NO_STOP = 0 ; 
      memset(buffer_message,0,RCVSIZE);
      strcpy(buffer_message,"DATA;FIN");
      sendto(*server_desc_message,buffer_message,1024,0,(struct sockaddr *)&addr,addr_size2);
    }
    else{
      fwrite(DATA_char , 1 , 900 , fp);
      memset(buffer_message,0,RCVSIZE);
      strcpy(buffer_message,"SEQ;");

      strcat(buffer_message,sequence_char);
      strcat(buffer_message,";ACK;");
      strcat(buffer_message,sequence_ACK_char);
      error = sendto(*server_desc_message,buffer_message,1024,0,(struct sockaddr *)&addr,addr_size2);


    }
}
close(*server_desc_message);
printf("BYE \n");
exit(0);

}

int main (int argc, char *argv[]) {
if(argc == 2)
  {
    int port_client_utilises[10] = {0} ;
    int pid_process[10] = {0}; 
    int nombre_client = 0;
    struct sockaddr_in adresse_control , adresse_message ;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size = sizeof(serverStorage);
    int port = 5001;
    if(atoi(argv[1]) != 0){
      port = atoi(argv[1]);
    }
    else{
      perror("Invalid Input \n");
      return -1;
    }
    char buffer_control[RCVSIZE];

    //create socket
    int server_desc_control = create_socket(&adresse_control , port);
    printf("server_desc_control : %i \n", server_desc_control);

//////Initialisation ///////////////////////////////////////////////////



    fd_set readset ; 
    struct timeval tv;
    tv.tv_sec = 0; 
    tv.tv_usec = 0 ; 
    struct timeval tv_SYN_ACK;
    tv_SYN_ACK.tv_sec = 1; 
    tv_SYN_ACK.tv_usec = 0 ; 
    srand(time(NULL)); 
    int result_select ;   
    int new_Port ; 
    int return_child ; 
    int message ; 
    char sequence_ACK_char[RCVSIZE];
    char sequence_SYN_char[RCVSIZE];
    char PORT[RCVSIZE];
    int sequence_ACK ; 
    int sequence_SYN ; 
    char result[6];
    int NO_STOP = 1 ; 



///////////////////////////////////////////////////////////////////////



    printf("%i \n", server_desc_control);
    while(NO_STOP){
      //NO_STOP = 0 ; 



////////////////////////////// Arrivée Client //////////////////////////////////////////////////////////////
      
      printf("recv \n");
      wait_client(&readset , &tv , pid_process ,port_client_utilises ,&nombre_client , &result_select ,&return_child,"BEGIN");
      message = recvfrom(server_desc_control,buffer_control,1024,0,(struct sockaddr *)&serverStorage,&addr_size);
      printf("%s \n",buffer_control);

      if(strstr(buffer_control , "SYN") != NULL){

          sscanf(buffer_control , "%*c %*c %*c %6s", result);
          sequence_ACK = atoi(result);
        
      }
      if(strstr(buffer_control , "stop serveur") != NULL)
      {
        NO_STOP = 0 ;
        continue;
      }
      else
      {
            sequence_SYN = 111111;
            //printf("%i \n",sequence_SYN);
            printf("%s \n",buffer_control);
            memset(buffer_control,0,RCVSIZE);

            strcpy(buffer_control,"SYN_ACK");
            for(int i = 0 ; i< sizeof(pid_process)/sizeof(pid_process[0]) ; i ++){
              printf(" port client : %i \n", port_client_utilises[i]);
              if(port_client_utilises[i] == 0){
                new_Port = port+1+i ; 
                break;
              }
            }
            int server_desc_message = create_socket(&adresse_message , new_Port);
            printf("server_desc_message : %i\n",server_desc_message );
            int gestion_client = fork();
            if (gestion_client == 0 ){
                gestion_client_fork( &server_desc_control , &server_desc_message, &adresse_message , new_Port , &serverStorage , addr_size);
            }
            else{
              close(server_desc_message);
              sprintf(PORT,"%i",new_Port);
              strcat(buffer_control,PORT);
              printf("%s\n",buffer_control);
              do{
                sendto(server_desc_control,buffer_control,12,0,(struct sockaddr *)&serverStorage,addr_size);
                FD_SET(3 , &readset);
                tv_SYN_ACK.tv_sec = 1 ; 
                result_select = select(4, &readset , NULL , NULL , &tv_SYN_ACK);
                printf("Send SYN_ACK until recieve ACK \n");
              }while(result_select == 0); 
              memset(buffer_control,0,RCVSIZE);
              message = recvfrom(server_desc_control,buffer_control,1024,0,(struct sockaddr *)&serverStorage,&addr_size);
              printf("%s \n",buffer_control);
              for ( int i = 0 ; i <sizeof(port_client_utilises)/sizeof(port_client_utilises[0]) ; i++){
                if(port_client_utilises[i] == 0 ){
                   port_client_utilises[i] = port+1+i; 
                   pid_process[i]= gestion_client ;
                   break ;               

                }
              nombre_client++;
            }
          }
        }
     // }




    }
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


  close(server_desc_control);
  return 0;
  }
}
