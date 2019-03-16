#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "lib.h"

#define BUFLEN 256

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
	
    int flag_login = 0; //flag pt a verifica daca p sesiune este deja deschisa
    char last_login[7]; //Are 7 in cazul in care de la tastatura se va introduce
    					//un numar_card uc dimensiune mai mare de 6 , marcahez acest lucru
    char *tok ; //pentru a parsa datele ( pt last login mai exact);
	
    int sockfd, n;
    struct sockaddr_in serv_addr, serv_udp;
    struct hostent *server;

    char buffer[BUFLEN];
    char buffer_aux[BUFLEN];
    if (argc < 3) {
       fprintf(stderr,"Usage %s server_address server_port\n", argv[0]);
       exit(0);
    }  
    
    //Socketul tcp
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("-10 Eroare la apelul functiei socket()\n");
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr.sin_addr);
    
    
    if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("-10 Eroare la apelul functiei connect(TCP)\n");
    }
    
    //Socketul udp
    int sock_udp;
    sock_udp = socket(PF_INET, SOCK_DGRAM, 0);
    if(sock_udp < 0)
    {
    	printf("-10 : Eroare la apelul functiei socket (UDP)\n");
    }
    
    serv_udp.sin_family = AF_INET;
    serv_udp.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_udp.sin_addr);
    
    
    int l = sizeof(serv_udp);
    int quit_flag  = 0;
    FILE* f ;
   	char crt_pid[20];
   	memset(crt_pid,0,15);
   	sprintf(crt_pid,"%d",getpid()); //Am luat pid-ul si l -am convertiti in sir de caractere
   	
    char nume_fisier[50];
    memset(nume_fisier,0,50);
    strcpy(nume_fisier,"client-");
    strcat(nume_fisier,crt_pid);
    strcat(nume_fisier,".log");
    f = fopen(nume_fisier,"wt");
    
    //pentru ca utilizatorul sa stie care a fost ultimul fisierl de log creeat
    //in momentul in care porneste clientul
   	printf("Numele fisierului de log este %s\n",nume_fisier);
    //bucla principala
    while(1){
    
  		//citesc de la tastatura
    	memset(buffer, 0 , BUFLEN);
    	fgets(buffer, BUFLEN-1, stdin);
		buffer[strlen(buffer) - 1] = '\0';
	
		fprintf(f, "%s\n",buffer);
		//Apoi in functie de inceputul comenzii , intru pe o anumita ramura 
		//Pentru denumirea comenzii se aceepta mici "erori" ex loginn in loc de login
		if(strncmp(buffer,"login",5) == 0)
		{
			if(flag_login == 1)
			{
				printf("IBANK> -2 : Sesiune deja deschisa\n");
				fprintf(f, "IBANK> -2 : Sesiune deja deschisa\n");
    
			} else {
			
				memset(buffer_aux, 0, BUFLEN);
				strcpy(buffer_aux, buffer);
				
				tok = strtok(buffer, " ");
				n = 0;
				memset(last_login, 0, 7);
				//Voi retine mereu in last_login numarul ultimului card
				//pe care s-a facut login , sau pe care clientul curent a incercat
				//sa se logheze ( aceasta informatie este necesara atunci cand
				//voi face unlock
				while(tok != NULL)
				{
					if(n == 1)
					{
						strncpy(last_login,tok,6);
					}
					n++;
					tok = strtok(NULL, " ");
				}
				
				//trimit mesaj la server
    			n = send(sockfd,buffer_aux,strlen(buffer_aux), 0);
    			if (n < 0){ 
        			printf("-10 : Eroare la apel send() (TCP)\n");
        			fprintf(f,"-10 : Eroare la apel send() (TCP)\n");
        		}
        		
				memset(buffer, 0, BUFLEN);
				n = recv(sockfd, buffer, sizeof(buffer), 0);
				if(n <= 0 || strcmp(buffer, "quit") == 0)
				{
					if(n == 0 || strcmp(buffer, "quit") == 0){
						quit_flag = 1;
						break;
					}
					else						{
						printf("-10 : Eroare la apel recv (TCP)\n");
						fprintf(f,"-10 : Eroare la apel recv (TCP)\n");
					}
				}
			
				printf("%s\n",buffer);
				fprintf(f, "%s\n",buffer);
				//Daca s-a reusit logarea , atunci semnalez acest lucru si in
				//cadrul executabilului client.
				if(strncmp(buffer,"IBANK> Welcome",13) == 0){
					flag_login = 1;
				}
			}
		}else if(strncmp(buffer,"logout",5) == 0)
		{
			//Am folosit afisarea fara IBANK> in fata deorece asa era afisarea
			//si in exemplul din cerinta , si oricum mesajul nu ar fi venit dela 
			//serverul de IBANK
			if(flag_login == 0)
			{
				printf("-1 : Clientul nu este autentificat\n");
				fprintf(f,"-1 : Clientul nu este autentificat\n");
			}
			else
			{
				flag_login = 0 ; //marchez faptul ca,,clientul s-a deconectat 
				
				//trimit mesaj la server
    			n = send(sockfd,buffer,strlen(buffer), 0);
    			if (n < 0){ 
        			printf("-10 : Eroare la apel send() (TCP)\n");
        			fprintf(f,"-10 : Eroare la apel send() (TCP)\n");
        		}
        		
				memset(buffer, 0, BUFLEN);
				n = recv(sockfd, buffer, sizeof(buffer), 0);
				if(n <= 0 || strcmp(buffer, "quit") == 0)
				{
					if(n == 0 || strcmp(buffer, "quit") == 0){
						quit_flag = 1;
						break;
					}
					else						{
						printf("-10 : Eroare la apel recv (TCP)\n");
						fprintf(f,"-10 : Eroare la apel recv (TCP)\n");
					}
				}
				
				printf("%s\n",buffer);
				fprintf(f,"%s\n",buffer);
			}
		
		}else if (strncmp(buffer,"listsold",8) == 0)
		{
			if(flag_login == 0)
			{
				printf("-1 : Clientul nu este autentificat\n");
				fprintf(f, "-1 : Clientul nu este autentificat\n");
			}
			else
			{
				//trimit mesaj la server
    			n = send(sockfd, buffer, strlen(buffer), 0);
    			if (n < 0){ 
        			printf("-10 : Eroare la apel send() (TCP)\n");
        			fprintf(f, "-10 : Eroare la apel send() (TCP)\n");
        		}
        		
				memset(buffer, 0, BUFLEN);
				n = recv(sockfd, buffer, sizeof(buffer), 0);
				if(n <= 0 || strcmp(buffer, "quit") == 0)
				{
					if(n == 0 || strcmp(buffer, "quit") == 0){
						quit_flag = 1;
						break;
					}
					else						{
						printf("-10 : Eroare la apel recv (TCP)\n");
						fprintf(f,"-10 : Eroare la apel recv (TCP)\n");
					}
				}
				
				printf("%s\n",buffer);
				fprintf(f,"%s\n",buffer);
			
			}
		} else if (strncmp(buffer,"transfer",8) == 0)
		{
			if(flag_login == 0)
			{
				printf("-1 : Clientul nu este autentificat\n");
				fprintf(f,"-1 : Clientul nu este autentificat\n");
			}
			else
			{
				//trimit mesaj la server
    			n = send(sockfd,buffer,strlen(buffer), 0);
    			if (n < 0){ 
        			printf("-10 : Eroare la apel send() (TCP)\n");
        			fprintf(f,"-10 : Eroare la apel send() (TCP)\n");
        		}
        		
        		//primesc mesajul de la server , care poate fi unul de eroare sau de
        		//confirmare a transferului pe acre doresc sa il efectuez ,
        		//semn ca el este valid ( iar mesajul incepe cu Transfer)
        		memset(buffer, 0, BUFLEN);
				n = recv(sockfd, buffer, sizeof(buffer), 0);
				
				if(n <= 0 || strcmp(buffer, "quit") == 0)
				{
					if(n == 0 || strcmp(buffer, "quit") == 0){
						quit_flag = 1;
						break;
					}
					else						{
						printf("-10 : Eroare la apel recv (TCP)\n");
						fprintf(f, "-10 : Eroare la apel recv (TCP)\n");
					}
				}
				
				printf("%s\n",buffer);
				fprintf(f,"%s\n",buffer);
				//Daca datele preliminarii de la transfer au fost introduse cum trebuie
				//atunci , urmeaza confirmarea numelui utilizatorului catre care
				//se va face operatia
				if(strncmp(buffer,"IBANK> Transfer",15) == 0)
				{
					memset(buffer,0,BUFLEN);
					fgets(buffer, BUFLEN-1, stdin);
					buffer[strlen(buffer) - 1] = '\0';
					fprintf(f,"%s\n",buffer);
										
					//Trimit raspunsul la server
					n = send(sockfd,buffer,strlen(buffer), 0);
					if (n < 0){ 
        				printf("-10 : Eroare la apel send() (TCP)\n");
        				fprintf(f,"-10 : Eroare la apel send() (TCP)\n");
        			}
					memset(buffer, 0, BUFLEN);
					n = recv(sockfd, buffer, sizeof(buffer), 0);
					
					if(n <= 0 || strcmp(buffer, "quit") == 0)
					{
						if(n == 0 || strcmp(buffer, "quit") == 0){
							quit_flag = 1;
							break;
						}
						else						{
							printf("-10 : Eroare la apel recv (TCP)\n");
							fprintf(f,"-10 : Eroare la apel recv (TCP)\n");
						}
					}
				
					printf("%s\n",buffer);
					fprintf(f,"%s\n",buffer);
				}
			}
			
		}else if(strncmp(buffer,"unlock",6) == 0)
		{
			//Nu voi mai verifica daca a fost apelata cel putin o data comanda
			//login deoarece in enunt se specifica faptul ca , nu se va intampla acest lucru
		
			//Rescriu buffer-ul cu unlock in caz ca utilizaatorul nu introduce doar aceasta
			//comanda , si va mai tasta si alte simboluri , caractere, care ar putea
			//altera modul de functionare al programului(precum "unlock parola_secreta")
			//intrucat serverul se astapta la un singur format , iar last_login oricum nu 
			//este introdus de la tastatura 
			memset(buffer, 0 , BUFLEN);
			strcpy(buffer, "unlock ");
			strcat(buffer, last_login);
			
			//Trimitem mesajul la server
			n = sendto(sock_udp, buffer, BUFLEN, 0, (struct sockaddr *)&serv_udp, sizeof(serv_udp));
			if( n <= 0)
			{
				printf("-10 : Eroare la apel sendto (UDP)\n");
				fprintf(f,"-10 : Eroare la apel sendto (UDP)\n");
			}
			memset(buffer, 0 , BUFLEN);
			
			n = recvfrom(sock_udp, buffer, BUFLEN, 0, (struct sockaddr *)&serv_udp, &l);
			if( n <= 0)
			{
				printf("-10 : Eroare la apel sendto (UDP)\n");
				fprintf(f,"-10 : Eroare la apel sendto (UDP)\n");
			}
			printf("%s\n",buffer);
			fprintf(f,"%s\n",buffer);
			
			if(strncmp(buffer, "UNLOCK> Trimite parola secreta",30) == 0)
			{
				memset(buffer, 0 , BUFLEN);
				fgets(buffer, BUFLEN-1, stdin);
				buffer[strlen(buffer) - 1] = '\0';
				fprintf(f,"%s\n",buffer);
				
				memset(buffer_aux, 0 , BUFLEN);
				strcpy(buffer_aux, last_login);
				strcat(buffer_aux, " ");
				strcat(buffer_aux, buffer);
				
				//Trimit parola sub formatul precizat in enunt <numar_card> <parola_secreta>
				
				sendto(sock_udp, buffer_aux, BUFLEN, 0, (struct sockaddr *)&serv_udp, sizeof(serv_udp));
				memset(buffer, 0 , BUFLEN);
				recvfrom(sock_udp, buffer, BUFLEN, 0, (struct sockaddr *)&serv_udp, &l);
				
				printf("%s\n",buffer);
				fprintf(f,"%s\n",buffer);
			}
		
		
		} else if(strncmp(buffer,"quit",4) == 0)
		{
			//trimit mesaj la server
    		n = send(sockfd,buffer,strlen(buffer), 0);
   			if (n < 0){ 
       			printf("-10 : Eroare la apel send() (TCP)\n");
       			fprintf(f, "-10 : Eroare la apel send() (TCP)\n");
       		}				
       		//Inchid socketii
			close(sockfd);
			close(sock_udp);
			fclose(f);
			//++Inchid fisierul de scris
			break;
		}
    }
    
	if(quit_flag == 1)
    {
    	printf("IBANK> Conexiune inchisa de server\n");
    	//In log file un apare mesajul de conexiune inchisa 
    	
    	//Inchid socketii
		close(sockfd);
		close(sock_udp);
		//++Inchid fisierul de log
		fclose(f);  	
	}
    return 0;
}


