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

#define RCVSIZE 1500



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


void wait_client(fd_set *readset , struct timeval *tv , int *pid_process , int *port_client_utilises , int *result_select , int *return_child ){

    do{
      FD_SET(3 , readset);
      *result_select = select(4, readset , NULL , NULL , tv); 
      *return_child = waitpid(-1 ,NULL,WNOHANG) ;
      if(*return_child != 0 && *return_child != -1 ) {
        for ( int j = 0 ; j<10*sizeof(int)/sizeof(int) ; j++) {
          if(pid_process[j] == *return_child){
            port_client_utilises[j] = 0 ;
            pid_process[j] = 0 ;
            printf("pid_process : %i\n", pid_process[j]);
            break ; 

          }
        }

      }

    }while(*result_select == 0 );
  }



void reconnaissance_DATA(int *sequence,int *sequence_ACK , unsigned char *DATA_char , char *buffer_message){
  char *ret_SEQ ;
  char *ret_ACK ;  
  ret_SEQ = strstr(buffer_message , "SEQ");
  ret_ACK = strstr(buffer_message , "ACK");
  char result[6];

  if(ret_SEQ != NULL){
    sscanf(ret_SEQ , "%*c %*c %*c %6s" , result);
    //printf("resultats : %s \n", result);
    *sequence = atoi(result);
    DATA_char = (unsigned char *)buffer_message+9 ;

  }
  if(ret_ACK != NULL){
    sscanf(ret_ACK , " %*c %*c %*c %6s" , result);
    //printf("resultats : %s \n", result);
    *sequence_ACK = atoi(result);
    DATA_char = (unsigned char *)buffer_message+9;


  }
  //sleep(1);

}

/*void Check_Ack(int *server_desc_message , char *buffer_message , struct sockaddr_storage *serverStorage , socklen_t addr_size){




}*/

void gestion_client_fork(int *server_desc_control , int *server_desc_message , struct sockaddr_in *adresse_message , int port , struct sockaddr_storage *serverStorage , socklen_t addr_size ) {


  //struct sockaddr_in addr = {0} ;
  //socklen_t addr_size2 = sizeof(addr);
  clock_t start_t,end_t ; 
  start_t = clock();
  long double file_size ; 
  struct timeval tv;
  tv.tv_sec = 0; 
  tv.tv_usec = 0 ; 
  int result_select ;
  fd_set readset ; 
  FILE *fp ; 
  //char sequence_ACK_char[RCVSIZE];
  unsigned char DATA_char[RCVSIZE-6];
  char *sequence_char = (char *)malloc(6*sizeof(char));
  //strcpy(sequence_char,"000000");
  char *buffer_message = (char *)malloc(RCVSIZE*sizeof(char));
  int sequence = 0 ;
  int sequence_ACK = -1;
  int message ; 
  int NO_STOP =1 ;
  int error ; 
  int size ; 
  close(*server_desc_control);
  sprintf(sequence_char,"%06i",sequence);
  printf("socket :   %i \n",*server_desc_message);


  memset(buffer_message,0,RCVSIZE);
  message = recvfrom(*server_desc_message,buffer_message,1024,0,(struct sockaddr *)serverStorage,&addr_size);
  if(message == -1){
    printf("Error \n");
  }
  fp = fopen(buffer_message,"rb");
  
  do{
    size = fread(DATA_char,1,RCVSIZE-6,fp);
    if( size != RCVSIZE-6 ){
        NO_STOP =0 ;
        //  printf("la sequence %s \n",sequence_char);

        memset(buffer_message, 0 , RCVSIZE);
        strcpy(buffer_message,sequence_char);
        strcat(buffer_message,(char *)DATA_char);
        //printf("sendto ok \n");
        if(error == -1){
          printf("Error \n");
        }
        do{

          FD_SET(4 , &readset);
          FD_SET(3 , &readset);
          tv.tv_sec = 0 ;
          tv.tv_usec = 11000 ;
          error = sendto(*server_desc_message,buffer_message,size+6,0,(struct sockaddr * )serverStorage , addr_size);
          result_select = select(6, &readset , NULL , NULL , &tv); 
          if(error == -1){
            printf("Error \n");
          }
        }while(result_select == 0);
        memset(buffer_message , 0 , RCVSIZE);
        message = recvfrom(*server_desc_message,buffer_message,1024,0,(struct sockaddr *)serverStorage,&addr_size);
        //printf("Reconnaissance_DATA 1 message : %s \n",buffer_message);
        reconnaissance_DATA(&sequence , &sequence_ACK , DATA_char , buffer_message);
        //printf("la sequence : %06i\n" , sequence_ACK);



      }
      else {
        memset(buffer_message, 0 , RCVSIZE);
        strcpy(buffer_message,sequence_char);
        strcat(buffer_message,(char *)DATA_char);
        do{

          FD_SET(4 , &readset);
          tv.tv_sec = 0 ;
          tv.tv_usec = 11000 ; 
          error = sendto(*server_desc_message,buffer_message,RCVSIZE,0,(struct sockaddr * )serverStorage , addr_size);
          result_select = select(6, &readset , NULL , NULL , &tv); 
        }while(result_select == 0);
        memset(buffer_message , 0 , RCVSIZE);
        message = recvfrom(*server_desc_message,buffer_message,1024,0,(struct sockaddr *)serverStorage,&addr_size);
        //printf("Reconnaissance_DATA 2 \n");
        reconnaissance_DATA(&sequence , &sequence_ACK , DATA_char , buffer_message);
        //printf("la sequence : %i\n" , sequence_ACK);
        sequence = atoi(sequence_char);
        sequence = sequence+1;
        sprintf(sequence_char,"%06i",sequence);
      }
  }while(NO_STOP == 1);
    memset(buffer_message, 0 , RCVSIZE);
    strcpy(buffer_message,"FIN");
    error = sendto(*server_desc_message,buffer_message,4,0,(struct sockaddr * )serverStorage , addr_size);
    if(error == -1){
      printf("Error \n");
    }
    /*do{

      FD_SET(4 , &readset);
      tv.tv_sec = 1; 
      error = sendto(*server_desc_message,buffer_message,4,0,(struct sockaddr * )serverStorage , addr_size);
      printf("Ce que j'ai envoyé : %s \n",buffer_message);
      result_select = select(4, &readset , NULL , NULL , &tv); 
    }while(result_select == 0);
    memset(buffer_message , 0 , RCVSIZE);
    message = recvfrom(*server_desc_message,buffer_message,1024,0,(struct sockaddr *)serverStorage,&addr_size);
    printf("Reconnaissance_DATA 1 \n");
    reconnaissance_DATA(&sequence , &sequence_ACK , DATA_char , buffer_message);
    printf("la sequence : %i\n" , sequence_ACK);
    */
    close(*server_desc_message);
    end_t = clock();
    fseek(fp , 0 , SEEK_END);
    file_size = ftell(fp);
    printf("start_t : %li    end_t : %li  CLOCKS_PER_SEC : %li \n",start_t , end_t , CLOCKS_PER_SEC);
    long double t = (long double)(end_t - start_t)/CLOCKS_PER_SEC;
    printf("Débit = %Lf \n",(long double)(file_size*0.000001/t));
    printf("BYE \n");
    exit(0);

}

int main (int argc, char *argv[]) {
if(argc == 2)
  {
    int *port_client_utilises = (int *)malloc(10*sizeof(int)) ;
    memset(port_client_utilises, 0 ,10*sizeof(int));
    int *pid_process = (int *)malloc(10*sizeof(int));
    memset(pid_process,0,10*sizeof(int)); 
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
    //char sequence_ACK_char[RCVSIZE];
    //char sequence_SYN_char[RCVSIZE];
    char PORT[RCVSIZE];
    //int sequence_ACK ; 
    char result[6];
    int NO_STOP = 1 ; 



///////////////////////////////////////////////////////////////////////



    printf("%i \n", server_desc_control);
    while(NO_STOP){
      //NO_STOP = 0 ; 



////////////////////////////// Arrivée Client //////////////////////////////////////////////////////////////
      
      printf("recv \n");
      wait_client(&readset , &tv , pid_process ,port_client_utilises, &result_select ,&return_child);
      message = recvfrom(server_desc_control,buffer_control,1024,0,(struct sockaddr *)&adresse_control,&addr_size);
      printf("Message_error : %i \n",message);
      printf("%s \n",buffer_control);

      if(strstr(buffer_control , "SYN") != NULL){

          sscanf(buffer_control , "%*c %*c %*c %6s", result);
          //sequence_ACK = atoi(result);
        
      }
      if(strstr(buffer_control , "stop serveur") != NULL)
      {
        NO_STOP = 0 ;
        continue;
      }
      else
      {
            //printf("%i \n",sequence_SYN);
            printf("%s \n",buffer_control);
            memset(buffer_control,0,RCVSIZE);

            strcpy(buffer_control,"SYN-ACK");
            for(int i = 0 ; i< 10*sizeof(int)/sizeof(int) ; i ++){
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
                tv_SYN_ACK.tv_sec = 4 ;
                printf("la socket : %i \n", server_desc_control);
                FD_SET(3 , &readset); 
                sendto(server_desc_control,buffer_control,12,0,(struct sockaddr *)&adresse_control,addr_size);
                result_select = select(4, &readset , NULL , NULL , &tv_SYN_ACK);
                printf("Send SYN_ACK until recieve ACK \n");
              }while(result_select == 0); 
              memset(buffer_control,0,RCVSIZE);
              message = recvfrom(server_desc_control,buffer_control,1024,0,(struct sockaddr *)&serverStorage,&addr_size);
              printf("%s \n",buffer_control);
              for ( int i = 0 ; i <10*sizeof(int)/sizeof(int) ; i++){
                if(port_client_utilises[i] == 0 ){
                   port_client_utilises[i] = port+1+i; 
                   pid_process[i]= gestion_client ;
                   break ;               

                }
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
