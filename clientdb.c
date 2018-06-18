//https://tomatoma.wordpress.com/manual-de-php/insertar-cambiar-y-borrar-datos-de-una-tabla-mysql/
//http://zetcode.com/db/mysqlc/
//https://stackoverflow.com/questions/41904422/mysql-query-select-fields-where-time-value-in-table-is-equal-to-current-time
//https://stackoverflow.com/questions/38224219/how-do-i-insert-data-into-mysql-table
//gcc db.c -o db  `mysql_config --cflags --libs`

#include <mysql.h>
#include <math.h>
#include <stdio.h>
#include <my_global.h>
#include <string.h>    //strlen
#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr
#include <unistd.h> 
#include <stdlib.h>
#include <ctype.h>

#define N 19
#define BUF_SIZE 256
#define COMPLETE 0
#define MAX_STRING 128
#define TIMER 590000 //Tiempo de espera en el sleep para enviar datos
#define MAX_STRING 128


struct sockaddr_in server;


//message buffer related delcartions/macros
int buffer_message(char * message);
int find_network_newline(char * message, int inbuf);

static int inbuf; // how many bytes are currently in the buffer?
static int room; // how much room left in buffer?
static char *after; // pointer to position after the received characters

void finish_with_error(MYSQL *con)
{
    fprintf(stderr, "%s\n", mysql_error(con));
    mysql_close(con);
    exit(1);        
}

//===============================================================
//Menu
void menu();

//Obtencion de datos
char* parametro(char *mensaje, char* bandera, int dato);

//Cliente
void EnviarMensaje(int socket, char mensaje[]);
int buffer_message(char * message);
int find_network_newline(char * message, int bytes_inbuf);



//================================================================

int main(int argc, char **argv)
{
	int sock;
	char nombre[]="4";	// representa el cliente db
	int salir=0;
	char *opc; //char para comparar lo ingresado de teclado
	char *delimiter="|"; //Delimitador para el mensaje
	
	char *token=(char*)malloc(sizeof(char)*4);

	char *message=(char*)malloc((sizeof(char)*BUF_SIZE)+1); //Buffer de salida
	char *server_reply=(char*)malloc((sizeof(char)*BUF_SIZE)+1); //Buffer de entrada
	char *header= (char*)malloc(sizeof(char)*4); //Tiene los 3 primeros datos que siempre se envian
	char *header2= (char*)malloc(sizeof(char)*4); //Tiene los 3 primeros datos que siempre se envian
	char *flagC= (char*)malloc(sizeof(char)*4); //Se usa para copiar los datos de la imagen a char* y concatenar
	char query[MAX_STRING] = {0};
	int sactual;
	char ID[3];
	char *resp="1";
	
	//CREACIÓN DEL SOCKET
	//AF_INEt: es el dominio
	//SOCK_STREAM: conexión tipo TCP
	sock = socket(AF_INET , SOCK_STREAM , 0);
	if (sock == -1){
        	printf("No se logró crear el socket");
    	}
    	puts("Creación del socket exitoso");
    	server.sin_addr.s_addr = inet_addr("54.184.87.135"); //IP del servidor
    	server.sin_family = AF_INET;
    	server.sin_port = htons( 9034 ); //Puerto de comunicación (definido en server.c)

	//Connect to remote server
	if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0){
        	perror("Conexión Fallida. Error");
        	return 1;
   	}
	//set up variables for select()
	fd_set all_set, r_set;
	int maxfd = sock + 1;
	FD_ZERO(&all_set);	
	FD_SET(STDIN_FILENO, &all_set); FD_SET(sock, &all_set);
	r_set = all_set;
	struct timeval tv; tv.tv_sec = 2; tv.tv_usec = 0;
	//set the initial position of after
	after = message;

	//Envia el numero de socket para asignarlo a una variable en el server
	if(send(sock, nombre, strlen(nombre) + 1, 0) < 0){
		puts("Conexión Fallida por nombre. Error");
		return 1;
	};
	//insert the code below into main, after you've connected to the server
	puts("Conexión Exitosa al Servidor\n");    


	//keep communicating with server
	menu();

	while(!salir){
    		r_set = all_set;
    		select(maxfd, &r_set, NULL, NULL, &tv); //check to see if we can read from STDIN or sock

//------------------------------------------------------------------------------------------------------------------------------------
//---------	Apartado Enviar Información
//------------------------------------------------------------------------------------------------------------------------------------
  		if(FD_ISSET(STDIN_FILENO, &r_set)){
 
			if(buffer_message(message) == COMPLETE){ 	
	

//--------------------------------------------------------------------- Salida del sistema
				opc="2";       	  	  
				if(!strncmp(message,opc,1)){
					salir=1;

					puts("Saliendo del sistema");					
				}
//----------------------------------------------------------------------------------------

//--------------------------------------------------------------------- Enviar al celular
				opc="1";       	  	  
				if(!strncmp(message,opc,1)){
					puts("Enviar al celular");					
				}
//----------------------------------------------------------------------------------------


    		}}//Llave del envio de informacion


//------------------------------------------------------------------------------------------------------------------------------------
//---------	Apartado recibir Información
//------------------------------------------------------------------------------------------------------------------------------------
    		if(FD_ISSET(sock, &r_set)){puts("recibiendo\n");
        		if( recv(sock , server_reply , 256 , 0) < 0){
            			puts("recv failed");
            			break;
        		}

			flagC="8";printf("\nServer Reply: %s\n", server_reply); // bandera 8 pregunta saldo
			if(!strncmp(server_reply,flagC,1)){

				token=strtok(server_reply,delimiter); //Header
				strcpy(header,token);
				printf("token header: %s\n",token);

				strcpy(ID,&token[2]);//obtener solamente el id desde el token

				token=strtok(NULL,delimiter);			
				int id = atoi(ID);
				int recarga = atoi(token);

				//entro a la base de datos para realizar la recarga
				int status = 0;    
				MYSQL *con = mysql_init(NULL);
				if (con == NULL){
					fprintf(stderr, "mysql_init() failed\n");
					exit(1);
				}  

				if (mysql_real_connect(con, "localhost", "root", "5005", "prueba", 0, NULL, 0) == NULL){
					finish_with_error(con);
				}
	    
				snprintf(query,MAX_STRING,"SELECT Saldo FROM Usuarios WHERE ID=%d",id); // se obtiene el saldo actual 

				if (mysql_query(con,query)!=0){
					finish_with_error(con);
				}

				do {  
					MYSQL_RES *result = mysql_store_result(con);

					if (result == NULL) 
					{
					finish_with_error(con);
					}

					MYSQL_ROW row = mysql_fetch_row(result);

					int val = atoi(row[0]);
					sactual=val+recarga; //sumo el saldo mas la recarga
					printf("El saldo de MEMO es: %s\n", row[0]);
					printf("El saldo de MEMO luego de recargar es: %d \n", sactual);
					int Saldo = sactual;
					snprintf(query,MAX_STRING, "UPDATE Usuarios SET Saldo=%d WHERE id=%d", Saldo, id);

					if (mysql_query(con,query)!=0){
						finish_with_error(con);
					}

					mysql_free_result(result);                 
					status = mysql_next_result(con); 

					if (status > 0){
						finish_with_error(con);
					}

				} while(status == 0);

				mysql_close(con);   menu();
		}//Llave de la bandera de recarga

//--------------------------------------------------------------------------------------------
			flagC="5"; printf("\nServer Reply: %s\n", server_reply);
			if(!strncmp(server_reply,flagC,1)){

			token=strtok(server_reply,delimiter); //Header
			strcpy(header,token);
			printf("token header: %s\n",token);
			strcpy(ID,&token[2]);//obtener solamente el id desde el token
			int id = atoi(ID);


			char query[MAX_STRING] = {0};
			int status = 0;    
			MYSQL *con = mysql_init(NULL);  

			if (con == NULL){
				fprintf(stderr, "mysql_init() failed\n");
				exit(1);
			}  

			if (mysql_real_connect(con, "localhost", "root", "5005", "prueba", 0, NULL, CLIENT_MULTI_STATEMENTS) == NULL){
				finish_with_error(con);
			}    

			snprintf(query,MAX_STRING,"SELECT Saldo FROM Usuarios WHERE ID=%d",id); // se obtiene el saldo actual 

			if (mysql_query(con,query)!=0){
				finish_with_error(con);
			}

			do{  
				MYSQL_RES *result = mysql_store_result(con);

				if (result == NULL) 
				{
				finish_with_error(con);
				}

				MYSQL_ROW row = mysql_fetch_row(result);

				int val = atoi(row[0]);
				printf("El saldo de MEMO es: %d\n", val);

				printf("header: %s\n", header);
				header2="514"; 

				strcpy(message,header2);	
				strcat(message,delimiter);

//compara si el saldo es mayor o menor que 0 para responder 
	
				if(val>0){
					puts("Se puede enviar la imagen");
					resp="1";
					strcat(message,resp);
					printf("mensaje a enviar: %s\n",message); // Borrar luego
					puts("Enviando mensaje ...\n\n");
				}
				if(val<=0){
					puts("No se puede enviar la imagen");
					resp="0";
					strcat(message,resp);	
					printf("mensaje a enviar: %s\n",message); // Borrar luego			
				}

				if(send(sock, message, strlen(message) + 1, 0) < 0){
       	   	     				puts("Send failed");
       	  	     	 			return 1;
		 	   	}	
				memset(server_reply,'\0',1);	

			
				mysql_free_result(result);                 
				status = mysql_next_result(con); 
				
				if (status > 0)
				{
				finish_with_error(con);
				}

			} while(status == 0);

			mysql_close(con);
				  menu();
		}//Llave consulta de saldo
//---------------------------------------------------------------------------------------------------------
        		

    		}//Lave de la recepción de informacion

	}//Llave del while

close(sock);

return 0;
}


//Función pintar menu
void menu(){
	//system("clear");
	printf("\n\n\n\nMenú de Mensajería Multimedia\n");
	printf("Ingrese la opción que desea: \n");
	printf("\t1. Enviar al celular\n");
	printf("\t2. Salir del sistema\n");
}

//Función de archivo

void EnviarMensaje(int socket, char mensaje[]){
	if(send(socket, mensaje, strlen(mensaje) + 1, 0) < 0){//NOTE: we have to do strlen(message) + 1 because we MUST include '\0'	
		printf("fallo al enviar el mensaje");
	}
}

int buffer_message(char * message){

    int bytes_read = read(STDIN_FILENO, after, 256 - inbuf);
    short flag = -1; // indicates if returned_data has been set 
    inbuf += bytes_read;
    int where; // location of network newline

    // Step 1: call findeol, store result in where
    where = find_network_newline(message, inbuf);
    if (where >= 0) { // OK. we have a full line

        // Step 2: place a null terminator at the end of the string
        char * null_c = {'\0'};
        memcpy(message + where, &null_c, 1); 

        // Step 3: update inbuf and remove the full line from the clients's buffer
        memmove(message, message + where + 1, inbuf - (where + 1)); 
        inbuf -= (where+1);
        flag = 0;
    }

    // Step 4: update room and after, in preparation for the next read
    room = sizeof(message) - inbuf;
    after = message + inbuf;

    return flag;
}

int find_network_newline(char * message, int bytes_inbuf){
    int i;
    for(i = 0; i<inbuf; i++){
        if( *(message + i) == '\n')
        return i;
    }
    return -1;
}

