//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------
//---------	CLIENTE UART		------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------

#include <stdio.h> //printf
#include <string.h>    //strlen
#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr
#include <unistd.h> 
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>

//------------------------------------------------------------------------------------------------------------------------------------
//---------	VARIABLES Y DEFINICIONES
//------------------------------------------------------------------------------------------------------------------------------------


//message buffer related delcartions/macros
int buffer_message(char * message);
int find_network_newline(char * message, int inbuf);
#define COMPLETE 0
#define BUF_SIZE 256
#define TIMER 699000 //Tiempo de espera en el sleep para recibir datos
#define TIMER2 700000 //Tiempo de espera en el sleep para enviar datos

//Para el archivo
#define HI(num) (((num) & 0x0000FF00) << 8) 
#define LO(num) ((num) & 0x000000FF) 

static int inbuf; // how many bytes are currently in the buffer?
static int room; // how much room left in buffer?
static char *after; // pointer to position after the received characters


//------------------------------------------------------------------------------------------------------------------------------------
//---------	ESTRUCTURAS
//------------------------------------------------------------------------------------------------------------------------------------

typedef struct {
    int row;
    int col;
    int max_gray;
    int **matrix;
} PGMData;
struct sockaddr_in server;


//------------------------------------------------------------------------------------------------------------------------------------
//---------	HEADERS DE FUNCIONES
//------------------------------------------------------------------------------------------------------------------------------------

//Menu
void menu();

//Obtencion de datos
char* parametro(char *mensaje, char* bandera, int dato);

//Cliente
void EnviarMensaje(int socket, char mensaje[]);
int buffer_message(char * message);
int find_network_newline(char * message, int bytes_inbuf);

//Archivo
int **allocate_dynamic_matrix(int row, int col);
void deallocate_dynamic_matrix(int **matrix, int row);
void SkipComments(FILE *fp);
static PGMData* readPGM(const char *file_name, PGMData *data);
void writePGM(const char *filename, const PGMData *data);


int set_interface_attribs(int fd, int speed){
	struct termios tty;
	if (tcgetattr(fd, &tty) < 0) {
		printf("Error from tcgetattr: %s\n", strerror(errno));
		return -1;
	}

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

void set_mincount(int fd, int mcount)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error tcgetattr: %s\n", strerror(errno));
        return;
    }

    tty.c_cc[VMIN] = mcount ? 1 : 0;
    tty.c_cc[VTIME] = 5;        /* half second timer */

    if (tcsetattr(fd, TCSANOW, &tty) < 0)
        printf("Error tcsetattr: %s\n", strerror(errno));
}

//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------
//---------	INICIO DEL MAIN
//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------
int main(int argc , char *argv[]){

PGMData *image=malloc(sizeof(*image));
PGMData *Newimage=malloc(sizeof(*Newimage));
PGMData *g = malloc(sizeof(*g));
image = readPGM("8.pgm",g);

char *portname = "/dev/ttyUSB0";
int fd;
int wlen;
char memo[13]="12345678912";
char conexion[13]="estoyactivo";

fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
if (fd < 0) 
{
printf("Error opening %s: %s\n", portname, strerror(errno));
return -1;
}
set_interface_attribs(fd, B9600);

//declaracion de variables
char buf[13];
int rdlen;
int columna=0;
int fila=0;
int gris=0;
int limiteContador=0;
int dato=0;

char *header= (char*)malloc(sizeof(char)*4); //Tiene los 3 primeros datos que siempre se envian
int Row=0;
int Col=0;
	//Variables de socket
int sock;
char nombre[]="1";	// representa clienteuart

//Variables de control
int contador=0; //Contador para saber si ya se recibió toda la imagen
char *delimiter="|"; //Delimitador para el mensaje
char *token=(char*)malloc(sizeof(char)*4);

//Variables para el mensaje
int numero; //Para sacar los numeros de la imagen para convertirlos a char
char *message=(char*)malloc((sizeof(char)*BUF_SIZE)+1); //Buffer de salida
char *server_reply=(char*)malloc((sizeof(char)*BUF_SIZE)+1); //Buffer de entrada

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

 
//=================APARTADO PARA RECIBIR POR TRANSCEIVERS===============================================================
		for(;;){
		menu();

//para calentar los transceives=================================================================================================
				int k=0;
				for(k=0;k<2;k++){
				rdlen = read(fd, buf, sizeof(buf));
					if (rdlen > 0){
						printf("me llego la basura para calentar los transceivers\n");
						buf[rdlen] = 0;
						printf("Read %d %s", rdlen, buf);
						printf("\n");
					} 
					usleep(TIMER);
				}
//===========================================================================================================================

					rdlen = read(fd, buf, sizeof(buf)); // se recibe informacion para comparar 
					if (rdlen > 0){
						buf[rdlen] = 0;
						printf("Read buffer  %d %s \n", rdlen, buf);
						strcpy(header,buf);
						token=strtok(header,delimiter); //toma el header
						printf("\ntoken header: %s\n",token);			
						usleep(TIMER);
					}

					int bandera=atoi(token);
					printf("BANDERA %d \n",bandera);

					if(bandera==0){// se recibe el mensaje para prueba de conexion
						puts("realizando pruebas de conexion\n");
						
						token=strtok(buf,delimiter); //toma el header
						strcpy(header,token);
						printf("\ntoken header: %s\n",token);

						token=strtok(NULL,delimiter);  
						printf("mensaje de prueba de conexion: %s\n",token);
						
						wlen = write(fd, &memo, sizeof(memo)); //envio
						usleep(TIMER2);
						
			
		
						wlen = write(fd, &conexion, sizeof(conexion)); //envio
						printf("respuesta para el celular: %s\n",conexion);
						printf("\nwlen: %d\n",wlen);
						usleep(TIMER2);



				
					}

					if(bandera==841){//-----------------------------------------------------recibe la recarga de saldo
						printf("recibiendo la recarga de saldo\n");
						if(send(sock, buf, strlen(buf) + 1, 0) < 0){//envia el mensaje al server
							puts("Send failed");
							return 1;
						}	
						//memset(buf,0,13);
					}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------	
					if(bandera==541){//recibe la consulta de saldo
						if(send(sock, buf, strlen(buf) + 1, 0) < 0){//envia el mensaje al server
							puts("Send failed");
							return 1;
						}
						if(FD_ISSET(sock, &r_set)){//recibe la respuesta de la base de datos 
							puts("recibiendo respuesta de db\n");
							if( recv(sock , server_reply , 256 , 0) < 0){
								puts("recv failed");
								break;
							}
						}
						
						wlen = write(fd, &memo, sizeof(memo)); //envio
						printf("respuesta para el cliente: %s\n",memo);
						usleep(TIMER2);

						strcpy(memo,server_reply);
						wlen = write(fd, &memo, sizeof(memo)); //envio
						printf("respuesta para el cliente: %s\n",memo);
						printf("\nwlen: %d\n",wlen);
						usleep(TIMER2);

					}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
					if((bandera==721)||(bandera==621)||(bandera==731)||(bandera==631)){// recibe la imagen
						if(send(sock, buf, strlen(buf) + 1, 0) < 0){//envia el mensaje al server
	       	   	     				puts("Send failed");
	       	  	     	 			return 1;
			 	   		}

						token=strtok(buf,delimiter); //toma el header
						strcpy(header,token);
						printf("\ntoken header: %s\n",token);

						token=strtok(NULL,delimiter); //Columna 
						printf("token col: %s\n",token);
						columna=atoi(token);
						Newimage->col=columna; 

						token=strtok(NULL,delimiter); //Fila
						printf("token fila: %s\n",token);
						fila=atoi(token);
						Newimage->row=fila;

						token=strtok(NULL,delimiter); //max_gray
						printf("token gris: %s\n\n",token);

						gris=atoi(token);
						Newimage->max_gray=gris;
						Newimage->matrix = allocate_dynamic_matrix(Newimage->row, Newimage->col); // Algo de la matriz v:
						limiteContador++;

						memset(buf,0,13);
						usleep(TIMER);

						//recibiendo informacion de la imagen 
						int i,j;
						for (i = 0; i < fila; ++i) {
							for (j = 0; j < columna; ++j){
								printf("esperando a recibir\n");

								rdlen = read(fd, buf, sizeof(buf));
						
								if(send(sock, buf, strlen(buf) + 1, 0) < 0){//envia el mensaje al server
			       	   	     				puts("Send failed");
			       	  	     	 			return 1;
							 	}	
								if (rdlen > 0){
									printf("recibiendo matriz de imagen\n");
									buf[rdlen] = 0;
									printf("Read %d %s", rdlen, buf);

									token=strtok(buf,delimiter); //Header
									strcpy(header,token);

									token=strtok(NULL,delimiter); //Dato
									dato=atoi(token);
									printf("\nRow: %d\n\n",Row);
									printf("COL: %d\n",Col);
									printf("dato: %d\n\n",dato);
									Newimage->matrix[Row][Col]=dato; //Se guarda el dato en la matriz
									limiteContador++;
									printf("contador llegada: %d\n",limiteContador);
				
									if((Row==(fila-1))&&(Col==(columna-1))){
									Col=0;
									Row=0;
									puts("Creando la imagen\n");
									//limiteContador=0;
									writePGM("aprobado.pgm",Newimage); 
									}
									else if (Col<(columna-1)){
									Col++;
									}				

									else{ //Posición columna=al limite
									Col=0;
									Row++;
									}
									usleep(TIMER);
								} 
								else if (rdlen < 0) 
								{
									printf("Error from read: %d: %s\n", rdlen, strerror(errno));
								}
								memset(buf,0,13);
								printf("\n");
							}//for2
						}//for 1
					}//recepcion de imagen
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

					if((bandera==612)){// recibe la imagen
						if(FD_ISSET(sock, &r_set)){//recibe la respuesta de la base de datos 
							puts("recibiendo respuesta de db\n");
							if( recv(sock , server_reply , 256 , 0) < 0){
								puts("recv failed");
								break;
							}
						}
						char memo[13]="m|123456789";
						wlen = write(fd, &memo, sizeof(memo)); //envio
						printf("respuesta para el cliente: %s\n",memo);
						usleep(TIMER2);

						strcpy(memo,server_reply);
						wlen = write(fd, &memo, sizeof(memo)); //envio
						printf("respuesta para el cliente: %s\n",memo);
						printf("\nwlen: %d\n",wlen);
						usleep(TIMER2);

						token=strtok(buf,delimiter); //toma el header
						strcpy(header,token);
						printf("\ntoken header: %s\n",token);

						token=strtok(NULL,delimiter); //Columna 
						printf("token col: %s\n",token);
						columna=atoi(token);
						Newimage->col=columna; 

						token=strtok(NULL,delimiter); //Fila
						printf("token fila: %s\n",token);
						fila=atoi(token);
						Newimage->row=fila;

						token=strtok(NULL,delimiter); //max_gray
						printf("token gris: %s\n\n",token);

						gris=atoi(token);
						Newimage->max_gray=gris;
						Newimage->matrix = allocate_dynamic_matrix(Newimage->row, Newimage->col); // Algo de la matriz v:
						limiteContador++;

						memset(buf,0,13);
						usleep(TIMER);

						//recibiendo informacion de la imagen 
						int i,j;
						for (i = 0; i < fila; ++i) {
							for (j = 0; j < columna; ++j){
								printf("esperando a recibir\n");

								if(FD_ISSET(sock, &r_set)){//recibe la respuesta de la base de datos 
									puts("recibiendo respuesta de db\n");
									if( recv(sock , server_reply , 256 , 0) < 0){
										puts("recv failed");
										break;
									}
								}					
								

									token=strtok(buf,delimiter); //Header
									strcpy(header,token);

									token=strtok(NULL,delimiter); //Dato
									dato=atoi(token);
									printf("\nRow: %d\n\n",Row);
									printf("COL: %d\n",Col);
									printf("dato: %d\n\n",dato);
									Newimage->matrix[Row][Col]=dato; //Se guarda el dato en la matriz
									limiteContador++;
									printf("contador llegada: %d\n",limiteContador);
				
									if((Row==(fila-1))&&(Col==(columna-1))){
									Col=0;
									Row=0;
									puts("Creando la imagen\n");
									//limiteContador=0;
									writePGM("aprobado.pgm",Newimage); 
									}
									else if (Col<(columna-1)){
									Col++;
									}				

									else{ //Posición columna=al limite
									Col=0;
									Row++;
									}
									usleep(TIMER);
								 
								
								memset(buf,0,13);
								printf("\n");
							}//for2
						}//for 1
					}//recepcion de imagen
//////------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
				}// cierra el for infinito
close(sock);
return 0;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------
//---------	DEFINICION DE FUNCIONES     -----------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------

//Función pintar menu
void menu(){
	//system("clear");
	printf("\n\n\nINICIO CLIENTEUART\n\n\n\n");
}

//funcion obtencion parametros imagen (convierte de int a char)

char* parametro(char *mensaje, char* bandera, int dato){
	sprintf(bandera,"%d",dato);
	strcat(mensaje,bandera);
	return mensaje;
	
}

//Función de archivo

void EnviarMensaje(int socket, char mensaje[]){
	if(send(socket, mensaje, strlen(mensaje) + 1, 0) < 0){//NOTE: we have to do strlen(message) + 1 because we MUST include '\0'	
		printf("fallo al enviar el mensaje");
	}
}

int **allocate_dynamic_matrix(int row, int col)
{
    int **ret_val;
    int i;
 
    ret_val = (int **)malloc(sizeof(int *) * row);
    if (ret_val == NULL) {
        perror("memory allocation failure");
        exit(EXIT_FAILURE);
    }
 
    for (i = 0; i < row; ++i) {
        ret_val[i] = (int *)malloc(sizeof(int) * col);
        if (ret_val[i] == NULL) {
            perror("memory allocation failure");
            exit(EXIT_FAILURE);
        }
    }
 
    return ret_val;
}

void deallocate_dynamic_matrix(int **matrix, int row)
{
    int i;
 
    for (i = 0; i < row; ++i) {
        free(matrix[i]);
    }
    free(matrix);
}

// Elimina los comentarios de la imagen
void SkipComments(FILE *fp)
{
    int ch;
    char line[100];
    while ((ch = fgetc(fp)) != EOF && isspace(ch)) {
        ;
    }
 
    if (ch == '#') {
        fgets(line, sizeof(line), fp);
        SkipComments(fp);
    } else {
        fseek(fp, -1, SEEK_CUR);
    } //puts("etiqueta prueba final SkipComments\n");
} 

//for reading
static PGMData* readPGM(const char *file_name, PGMData *data)
{

    FILE *pgmFile;
    char version[3];
    int i, j;
    int lo, hi;
    pgmFile = fopen(file_name, "rb");

    if (pgmFile == NULL) {
        perror("cannot open file to read");
        exit(EXIT_FAILURE);
    }
    fgets(version, sizeof(version), pgmFile);
    if (strcmp(version, "P5")) {
        fprintf(stderr, "Wrong file type!\n");
        exit(EXIT_FAILURE);
    }

    SkipComments(pgmFile);
    fscanf(pgmFile, "%d", &data->col);
    SkipComments(pgmFile);
    fscanf(pgmFile, "%d", &data->row);
    SkipComments(pgmFile);
    fscanf(pgmFile, "%d", &data->max_gray);
    fgetc(pgmFile);
    data->matrix = allocate_dynamic_matrix(data->row, data->col);
    if (data->max_gray > 255) {
        for (i = 0; i < data->row; ++i) {
            for (j = 0; j < data->col; ++j) {
                hi = fgetc(pgmFile);
                lo = fgetc(pgmFile);
                data->matrix[i][j] = (hi << 8) + lo;
		//printf("%d ",data->matrix[i][j]);
            }
        }
    }
    else {
        for (i = 0; i < data->row; ++i) {
            for (j = 0; j < data->col; ++j) {
                lo = fgetc(pgmFile);
                data->matrix[i][j] = lo;
		//printf("%d ",data->matrix[i][j]);
            }
        }
    }
 
    fclose(pgmFile);
    return data;
 
}

//for writing
void writePGM(const char *filename, const PGMData *data)
{
    FILE *pgmFile;
    int i, j;
    int hi, lo;

 
    pgmFile = fopen(filename, "wb");
    if (pgmFile == NULL) {
        perror("cannot open file to write");
        exit(EXIT_FAILURE);
    }
 
    fprintf(pgmFile, "P5 ");
    fprintf(pgmFile, "%d %d ", data->col, data->row);
    fprintf(pgmFile, "%d ", data->max_gray);
 
    if (data->max_gray > 255) {
        for (i = 0; i < data->row; ++i) {
            for (j = 0; j < data->col; ++j) {
                hi = HI(data->matrix[i][j]);
                lo = LO(data->matrix[i][j]);
                fputc(hi, pgmFile);
                fputc(lo, pgmFile);
            }
 
        }
    }
    else {
        for (i = 0; i < data->row; ++i) {
            for (j = 0; j < data->col; ++j) {
                lo = LO(data->matrix[i][j]);
                fputc(lo, pgmFile);
            }
        }
    }
 
    fclose(pgmFile);
    deallocate_dynamic_matrix(data->matrix, data->row);
}

//	Funciones de cliente

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



