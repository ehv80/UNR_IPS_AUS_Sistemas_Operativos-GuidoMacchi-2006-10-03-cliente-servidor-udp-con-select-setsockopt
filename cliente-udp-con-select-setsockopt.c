/* Archivo: cliente-udp-con-select-setsockopt.c 
 * Abre un socket UDP (NO ORIENTADO A CONEXIÓN).
 * Con setsockopt(2) aplica al conector sockio 
 * la opción 'SO_REUSEADDR' del nivel 'SOL_SOCKET'
 * con el valor de la variable 'yes'
 * Verifica con select(2) si existen DATAGRAMAS de
 * entrada por el socket ó por STDIN.
 * 
 * Si se reciben paquetes por STDIN, los envía por el socket.
 * (sendto(2))
 *
 * Si se reciben paquetes por el socket, los envía a STDOUT.
 * (recvfrom(2))
 **/

/* ARCHIVOS DE CABECERA */
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/time.h>
#include<sys/select.h>
#include<string.h>

/* DEFINICIONES */
#define PORT 5000
#define MAX(x,y) ((x)>(y) ? (x) : (y))
#define SIZE 1024
#define TIME 3600
				   
/* SINONIMOS */
typedef struct sockaddr *sad;

/* FUNCIONES */
void error(char *s){
    perror(s);
    exit(-1);
}

/* FUNCION PRINCIPAL MAIN */
int main(int argc, char **argv){
	if(argc < 2){
		fprintf(stderr,"usa: %s ipaddr \n", argv[0]);
		exit(-1);
	}
	const int yes = 1;
	int sockio, cuanto, largo, recibidos; 
	//por sockio va a enviar-recibir paquetes datagramas
	
	//direcciones de internet para sockio
	struct sockaddr_in sini, sino;
	// sini para recibir-leer
	// sino para enviar-escribir
	
	char linea[SIZE];  //buffer de 1024 caracteres (un arreglo)
	fd_set in, in_orig;//conjuntos de descriptores de ficheros
	struct timeval tv; //tiempo limite hasta que select(2) retorne
	// abre conector UDP
	if((sockio = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		error("socket");
	//familia de direcciones a la que pertenece sino, para sockio
	sino.sin_family = AF_INET;
	sino.sin_port = htons(PORT); //puerto de sino, para sockio
    	inet_pton(AF_INET, argv[1], &sino.sin_addr);
	/* POSIX TO NETWORK, ver man 3 inet_pton : transforma 
	 * argv[1] (la direccion IP pasada como argumento al programa)
	 * desde formato ascii (puntero a cadena de caracteres)
	 * al formato network 
	 * guarda en la struct in_addr sino.sin_addr que a su vez
	 * es miembro de la struct sockaddr_in sino.  Se trata de la
	 * direccion IP del servidor al que quiere enviar datagramas,
	 * pertenece a la familia de direcciones ``AF_INET´´
	 **/
	
	/* Aplica al conector sockio la opción 'SO_REUSEADDR'
	 * del nivel 'SOL_SOCKET' con el valor de 'yes'
	 **/
	if( setsockopt(sockio, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0)
		error("setsockopt: sock");	
	
	/*tiene select*/
	//Limpia el conjunto de descriptores de fichero in_orig
	FD_ZERO(&in_orig);

	//añade al conjuto in_orig el descriptor STDIN
	FD_SET(0, &in_orig);

	//añade al conjunto in_orig el descriptor sockio
	FD_SET(sockio, &in_orig);
	
	/*tiene 1 hora*/
	//tiempo disponible hasta que select(2) regrese: 3600 segundos
	tv.tv_sec=TIME;
	tv.tv_usec=0;
	
	for(;;){
		// copia contenido del conjunto in_orig en in
		memcpy(&in, &in_orig, sizeof in);
		
		//ve si hay algún datagrama en el conjunto in
		if((cuanto = select(MAX(0,sockio)+1, &in, NULL, NULL, &tv)) < 0)
			error("select: error");

		//si el tiempo de espera de select(2) expira ==> timeout
		if(cuanto == 0)
			error("select: timeout");
		
		/*averiguamos donde hay algo para leer*/
		
		//si hay para leer desde STDIN
		if(FD_ISSET(0,&in)){
			
			//lee hasta 1024 caracteres de STDIN y pone en linea
			fgets(linea, SIZE, stdin);
			
			//envía contenido de linea en sockio
			if(sendto(sockio, linea, sizeof linea, 0, (sad)&sino, sizeof sino) < 0)
				error("sendto");
		}
		
		largo = sizeof sini;
		
		//si hay para leer desde sockio
		if(FD_ISSET(sockio, &in)){
			
			/*recibe hasta 1024 bytes desde sockio mediante dirección-puerto sini
			 * y guarda en linea */
			if((recibidos = recvfrom(sockio, linea, SIZE, 0, (sad)&sini, &largo)) < 0)
				error("recvfrom"); 
			else if(recibidos == 0)
				break; //si lectura devuelve 0  ==> parar la ejecucion
			//marcar el final del buffer con '0'
			linea[recibidos] = 0;
			
			/* Imprime en pantalla la dirección del servidor
			 * desde donde vienen datos */
			printf("\nDe la direccion[ %s ] : puerto [ %d ] --- llega el mensaje:\n",
					inet_ntoa(sini.sin_addr),
					ntohs(sini.sin_port));
			
			//Imprime en pantalla datos recibidos
			printf("%s \n",linea);
		}
	}
	close(sockio);
	return 0;
}
/* Fin Archivo: cliente-udp-con-select-setsockopt.c */
