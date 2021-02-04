/* Archivo: servidor-udp-con-select-setsockopt.c
*  Abre un socket UDP (NO ORIENTADO A CONEXIÓN). 
*  Con setsockopt(2) aplica al conector sockio la opción
*  ``SO_REUSEADDR´´ del nivel ``SOL_SOCKET´´ con el valor
*  de la variable ``yes´´ para permitir que más de una instancia
*  del proceso cliente pueda conectarse al mismo puerto en forma
*  simultánea.
*  Verifica con select(2) si existen datagramas de entrada
*  por el socket ó por STDIN.
*  Si se reciben paquetes por el socket, los envía a STDOUT.
*  Si se reciben paquetes por STDIN, los envía por el socket.
*/ 

/* ARCHIVOS DE CABECERA */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/time.h>
#include<sys/select.h>

/* DEFINICIONES */
#define PORT 5000
#define MAX(x,y) ((x)>(y) ? (x) : (y))
#define SIZE 1024
#define TIME 3600

/* SINONIMOS*/
typedef struct sockaddr *sad;

/* FUNCIONES */
void error(char *s){
	perror(s);
	exit(-1);
}

/* FUNCION PRINCIPAL MAIN */
int main(){
	const int yes = 1; //para setsockopt: SO_REUSEADDR
	int sockio, cuanto, largo, recibidos;

	struct sockaddr_in sinio;
	char linea[SIZE];
	fd_set in, in_orig;
	struct timeval tv;
	//abre un conector UDP sockio
	if((sockio=socket(PF_INET, SOCK_DGRAM, 0)) < 0 )
		error("socket");

	// Familia de direcciones de sockio
	sinio.sin_family = AF_INET;
	// Puerto, con bytes en orden de red, para sockio
	sinio.sin_port = htons(PORT);
	/*dirección de internet, con bytes en orden de red,
	 * para sockio */
	sinio.sin_addr.s_addr = INADDR_ANY; 

	/* Aplica al conector sockio la opción ``SO_REUSEADDR´´
	 * del nivel ``SOL_SOCKET´´ con el valor de ``yes´´
	 **/
	if( setsockopt(sockio, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0)
		error("setsockopt: sock");
		
	//publica dirección-puerto sinio, del conector sockio
	if( bind(sockio, (sad)&sinio, sizeof sinio) < 0 )
		error("bind");
	
	/*Lo que lea de STDIN mediante dirección sinio.sin_addr
	 *de sockio, se escribirá en sockio */
	
	/*Lo que lea por sockio desde dirección remota 
	 *sinio.sin_addr, se escribirá en STDOUT */

	/*tiene select*/
	//Limpia el conjunto de descriptores de ficheros in_orig
	FD_ZERO(&in_orig);
	//añade STDIN al conjunto in_orig
	FD_SET(0, &in_orig);
	//añade sockio al conjunto in_orig
	FD_SET(sockio, &in_orig);
		
	/*tiene 1 hora*/
	//tiempo hasta que select(2) retorne: 3600 segundos
	tv.tv_sec=TIME;
	tv.tv_usec=0;

	for(;;){
		// copia conjunto in_orig en in
		memcpy(&in, &in_orig, sizeof in);
		
		/* espera a ver si se reciben datagramas por STDIN
		 * o por sockio */
		if( (cuanto = select( MAX(0,sockio)+1, &in, NULL, NULL, &tv) ) < 0 )
			error("select: error");
		// si el tiempo de select(2) termina -> error timeout
		if(cuanto == 0)
			error("select: timeout");
				
			largo = sizeof sinio;
			
		/* averigua donde hay algo para leer*/
		
		// Si hay para leer desde el conector sockio
		if(FD_ISSET(sockio, &in)){
			
			//recibe hasta 1024 caracteres de sockio y los pone en linea
			if((recibidos = recvfrom(sockio, linea, SIZE, 0, (sad)&sinio, &largo )) < 0)
				error("recvfrom");
			else if(recibidos == 0)	//si lectura devuelve 0 -> parar ejecucion
				break;

			// marca el fin del buffer con '0'
			linea[recibidos]=0;
			
			/* Imprime en pantalla dirección de internet del cliente
			 * desde donde vienen datos */
			printf("\nDe la direccion[ %s ] : puerto[ %d ] --- llega el mensaje:\n",
					inet_ntoa(sinio.sin_addr),
					ntohs(sinio.sin_port));

			// imprime el mensaje recibido
			printf("%s \n",linea);
		}
		
		//si hay para leer desde STDIN
		if(FD_ISSET(0,&in)){
			
			//lee hasta 1024 caracteres de STDIN, los pone en linea
			fgets(linea, SIZE, stdin);
			
			//envía contenido de linea en sockio
			if(sendto(sockio, linea, sizeof linea, 0, (sad)&sinio, largo) < 0 )
				error("sendto");
		}
	}
	close(sockio);
	return 0;
}
/* Fin Archivo: servidor-udp-con-select-setsockopt.c */
