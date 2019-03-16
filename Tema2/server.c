#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "lib.h"

#define MAX_CLIENTS	100

#define BUFLEN 256

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
	//Acestea sunt diferite variabile pe care le utilizez pentru a parsa date, pentru 
	//a retine anumite informatii ,etc
	int indice = 0; //indice pt a memora pozitia din vector a unui cont 
	int flag1 = 0;  
    int flag2 = 0;
	char pin[5]; //Atunci cand parsez datele si doresc sa retin pinul 
				//Desi pinul este de 4 , atunci cand primesc un pin cu mai mult de 4
				//Caractere, atunci voi semnala acest lucru si voi pune pe poziti 5 un caracter
				
	char parola_secreta[9]; //La fel si in cazul parolei secrete si a numarului de card
	char numar_card[7];
	char *tok;
	char sold_array[15];//Suma sa fie de maxim 12 cifre (  . si 2 zecimale)
	double sold_val = 0;
	
     int sockfd, newsockfd, portno, clilen;
     char buffer[BUFLEN];
     struct sockaddr_in serv_addr, cli_addr,serv_udp,serv_udp2;
     int n, i, j;
     //Numarul clientilor din fisier	
     int nr_conturi;

	 //Pentru a citi datele din fisierul cu datele utilizatorilor
	 FILE* f;	
	 f = fopen(argv[2],"rt");
	 
	 fscanf(f,"%d",&nr_conturi);
	 printf("IBANK> In baza de date sunt inregistrate %d de conturi\n",nr_conturi);

	 //Vector de clienti, in care se retin informatiile din fisiier
	 cont_persoana cont[nr_conturi+1];
	 
	 //Vector in care se retine pt fiecare client (in caz ca s-a autentificat),
	 //indicele contului , cu care s-a autentificat 
	 int cl_cont[MAX_CLIENTS];
	 int cl_fails[MAX_CLIENTS]; //Retinem de cate ori s-a incercat (in mod nereusit) logarea pe 
	 							//contul al carui indice este in cl_fail_cont
	 							
	 int cl_fail_cont[MAX_CLIENTS]; //Retinem care a fost ultimul cont cu
	 								// care nu s-a reusit logarea (indicele)
	 										
	 
	 //Aceast vector retine pentru fiecare din clientii daca urmatorul raspuns este
	 //considerat de catre server , ca si confirmare a unei comenzi de transfer
	 //si este 1 semn , ca mesajul trimis de client nu este o comanda ci doar un raspuns
	 int transfer[MAX_CLIENTS];
	 //Pt a nu tine 2 vectori , voi folosi vectorul transfer si pentru cazurile
	 //in care urmeaza sa primesc parola in cazul unui unlock , deoarece socketii , oricum
	 //au valori diferite chiar daca vin de la acelasi client ( udp si tcp)
	 double bani[MAX_CLIENTS];
	 
	 //initializez toti vectorii auxiliari 
	 for(i = 0 ; i < MAX_CLIENTS ;i++)
	 {
	 	bani[i] = 0;
	 	transfer[i] = 0;
	 	cl_cont[i] = 0;
	 	cl_fails[i] = 0;
	 	cl_fail_cont[i] = 0;
	 }

	 for(i = 1 ; i <= nr_conturi ; i++)
	 {
	 	fscanf(f,"%s", cont[i].nume);
	 	fscanf(f,"%s", cont[i].prenume);
	 	fscanf(f,"%s", cont[i].numar_card);
	 	fscanf(f,"%s", cont[i].pin);
	 	fscanf(f,"%s", cont[i].parola);
	 	fscanf(f,"%lf", &(cont[i].sold));
	 	cont[i].cont_status = 0;
	 	cont[i].clientSocket = 0;
	 }
	 //Inchidem fisierul
	 fclose(f);
	 
     fd_set read_fds;	//multimea de citire folosita in select()
     fd_set tmp_fds;	//multime folosita temporar 
     int fdmax;		//valoare maxima file descriptor din multimea read_fds

     if (argc < 2) {
         fprintf(stderr,"Usage : %s port\n", argv[0]);
         exit(1);
     }

     //golim multimea de descriptori de citire (read_fds) si multimea tmp_fds 
     FD_ZERO(&read_fds);
     FD_ZERO(&tmp_fds);
     
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        printf("-10 : Eroare la apelul functiei socket (TCP)\n");
     
     
     portno = atoi(argv[1]);
     memset((char *) &serv_addr, 0, sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
     serv_addr.sin_port = htons(portno);
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
     {
              printf("-10 : Eroare la apelul functiei bind (TCP)\n");
  	 }
     listen(sockfd, MAX_CLIENTS);

     //adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
     FD_SET(sockfd, &read_fds);
     FD_SET(0, &read_fds); //am adaugat socketul cu care ascult de la tastatura (o pt stdin)

	 //Sokcetul udp , pe care voi primi comenzi de unlock de la clienti
	 //(toti clinetii vor trimite pe acest socket)
	 int sock_udp;
	 int len = sizeof(serv_udp);
	 sock_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	 if(sock_udp < 0)
	 {
	 	printf("-10 : Eroare la apelul functiei socket (UDP)\n");
	 }
	 memset((char *) &serv_udp, 0, sizeof(serv_udp));
     serv_udp.sin_family = AF_INET;
     serv_udp.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
     serv_udp.sin_port = htons(portno);
	 
	 if (bind(sock_udp, (struct sockaddr *) &serv_udp, sizeof(struct sockaddr)) < 0) 
     {
     	printf("-10 : Eroare la apelul functiei bind() (UDP)\n");
     }	
     //Adaugam socketul udp in multimea read_fs
     FD_SET(sock_udp, &read_fds);
     //Se schimba maximul 
     fdmax = sock_udp;
	int k;
	int quit_flag = 0; //pentru a parasi inchide programul cand se tasteaza
						//quit de la tastatura
    // bucla principala
	while (quit_flag == 0) {
		tmp_fds = read_fds; 
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
		{
			printf("-10 : Eroare la apelul functiei select\n");
		}
	
		for(i = 0; i <= fdmax; i++) {
			if(quit_flag == 1)
			{
				break;
			}
			if (FD_ISSET(i, &tmp_fds)) {
				if(i == 0)
				{
					memset(buffer, 0 , BUFLEN);
					fgets(buffer, BUFLEN-1, stdin);
					buffer[strlen(buffer) - 1] = '\0';
					//Daca s-a introdus de la tastatutra , din server
					//"quit" , atunci serverul isi va incheia trnsmisia si 
					//trebuie sa anunt toti clinetii activi de acest lucru
					if(strcmp(buffer,"quit") == 0)
					{
						//inchid socketul udp
						close(sock_udp);
						
						//Inchid socketul de tcp 
						close(sockfd);
						
						//Ii scot din multimea de scoketi
						FD_CLR(sock_udp, &read_fds);
						FD_CLR(sockfd, &read_fds);
						
						tmp_fds = read_fds;
						for(k = 4; k <= fdmax; k++)
						{
							//mesajul trebuie trimis catre fiecare clinetii
							//iar fiecare client are asocia un unic socket tcp
						    if (FD_ISSET(k, &tmp_fds))
							{
								n = send(k, buffer, BUFLEN, 0);
								if (n < 0){ 
        							printf("-10 : Eroare la apel send() (TCP)\n");
        						}
        						close(k);
							}	
						}
						
						quit_flag = 1; //pt a parasi si bucla while
						break;
					}
				}
				else if(i == sock_udp)
				{
					//Pentru inceput receptionez mesajul
					memset(buffer, 0, BUFLEN);
					n = recvfrom(sock_udp, buffer, BUFLEN, 0, (struct sockaddr *)&serv_udp2, &len);
					if (n < 0)
					{ 
		        			printf("-10 : Eroare la apel recvfrom() (UDP)\n");
		        	}
					printf ("Am primit de la un client pe socketul UDP %d, mesajul: %s\n", i, buffer);
					//Daca transfer[i] este nenul , atunci asteptam de la 
					//client o parola , pentru deblocarea ultimului cont accesat 
					//Acest fiind "valid" de deblocare
					if(transfer[i] != 0)
					{
						//Parsam datele , pentru a lua din mesajul primit numarul carudlui
						//si parola secreta
						memset(numar_card,0,7);
						memset(parola_secreta, 0, 9);	
						tok = strtok(buffer," ");
						n = 0;
						while(tok != NULL)
						{
							if(n == 0)
							{
								strncpy(numar_card,tok,6);	
								if(strlen(tok) > 6)
								{
									numar_card[6] = 23;
								}						
							}
							//Parola poate varia in functie de utilizator , asa ca nu o
							//mai putem lua cu strncpy (si lungimea variaza)
							if(n == 1)
							{
								strcpy(parola_secreta,tok);
							}
							tok = strtok(NULL," ");
							n++;
						}
						flag1 = 0;
						flag2 = 0;
						indice = 0;
						for(j = 1 ; j <= nr_conturi ; j++)
						{
							if(strncmp(numar_card,cont[j].numar_card,6) == 0 && strlen(numar_card) == 6)
							{
								indice = j;
								flag1 = 1;
								if(strcmp(parola_secreta,cont[j].parola) == 0)
								{
									flag2 = 1;
								}
							}
						}
						
						
						//Refacem "flag-ul" pentru socket-ul udp
						transfer[i] =0;
						memset(buffer, 0 , BUFLEN);
						//Daca parola trimisa este cea asteptata atunci cardul este deblocat
						if(flag2 == 1)
						{
							strcpy(buffer, "UNLOCK> Card deblocat");
							cont[indice].cont_status = 0; //Deblocat cardul de tot
							
						}
						else //Altfel nu putem debloca
						{
							strcpy(buffer, "UNLOCK> -7 : Deblocare esuata");
						}
						//Trimitem inapoi clientului mesajul
						sendto(sock_udp, buffer, BUFLEN, 0, (struct sockaddr *)&serv_udp2, sizeof(serv_udp));			
						if (n < 0){ 
		        			printf("-10 : Eroare la apel sendto() (UDP)\n");
		        		}		
					}
					
					else
					{		
						//Atlfel daca transfer[sock_udp] == 0
						//Atunci am primit o cerere de deblocare
						memset(numar_card,0,7);	
						tok = strtok(buffer," ");
						n = 0;
						//Verific daca lungimea numarului de card este corespunzatoare
						//la pasul urmator, ( daca si primele 6 corespuns)
						while(tok != NULL)
						{
							if(n == 1)
							{
								strncpy(numar_card,tok,6);							
							}
							tok = strtok(NULL," ");
							n++;
						}
						flag1 = 0;
						flag2 = 0;
						indice = 0;
						//Verificam validitatea cardului introdus
						for(j = 1 ; j <= nr_conturi ; j++)
						{
							if(strncmp(numar_card,cont[j].numar_card,6) == 0)
							{
								indice = j;
								flag1 = 1;
							}
						}
						memset(buffer, 0 , BUFLEN);
						if(flag1 == 0)
						{
							strcpy(buffer, "UNLOCK> -4 : Numar Card inexistent");
						} else if(cont[indice].cont_status != 2) //daca nu este blocat 
						{										//operatia este esuata	
							strcpy(buffer, "UNLOCK> -6 : Operatie esuata");
						}else
						{
							strcpy(buffer, "UNLOCK> Trimite parola secreta");
							transfer[i] = indice;
						}
						n = sendto(sock_udp, buffer, BUFLEN, 0, (struct sockaddr *)&serv_udp2, sizeof(serv_udp));
						if (n < 0){ 
        			printf("-10 : Eroare la apel sendto() (UDP)\n");
        		}
						
					}
				//Daca am o conexiune noua pe socket TCP				
				}else if (i == sockfd)
			    {
					// a venit ceva pe socketul inactiv(cel cu listen) = o noua conexiune
					// actiunea serverului: accept()
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
						printf("-10 : Eroare la apelul functiei accept (TCP)\n");
					} 
					else {
						//adaug noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}
					}
					printf("Noua conexiune de la %s, port %d, socket_client %d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);
				}
					
				else {
					// am primit date pe unul din socketii cu care vorbesc cu clientii
					//actiunea serverului: recv()
					memset(buffer, 0, BUFLEN);
					if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
						if (n == 0) {
							//conexiunea s-a inchis
							printf("Server: Conexiunea de pe socketul %d s-a inchis\n", i);
							//Acum in caz ca era logat pe un cont , acela este
							//din nou disponibil
							cont[cl_cont[i]].clientSocket = 0;
							cl_fails[i] = 0;
							cl_fail_cont[i] = 0;
							cl_cont[i] = 0;
						} else {
							printf("-10 : Eroare la apelul functiei recv (TCP)\n");
						}
						close(i); 
						FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul pe care 
					} 
					
					else { //recv intoarce >0
						printf ("Am primit de la clientul de pe socketul %d, mesajul: %s\n", i, buffer);
					}
					//Cazul de noua conexiune si UDP SI INPUT a fost testat anterior 
					//acum sunt doar comunicari cu clienti prin tcp
					if(strncmp(buffer,"login",5) == 0 && transfer[i] == 0){
						
						memset(pin, 0, 5);
						memset(numar_card, 0, 7);		
						tok = strtok(buffer," ");
						n = 0;
						while(tok != NULL)
						{
							if(n == 1)
							{
								if(strlen(tok) == 6)
									strncpy(numar_card,tok,6);							
								else
								{
									strncpy(numar_card,tok,6);	
									numar_card[6] = 23;//acest lucru semnaleaza faptul
														//ca numarul de card are lungimea prea mare
								}
							}
							if(n == 2)
							{
								if(strlen(tok) == 4)	
									strncpy(pin,tok,4);
								else
								{
									strncpy(pin,tok,4);
									pin[4] = 23;	
								}	
							}
							tok = strtok(NULL," ");
							n++;
						}
						
						flag1 = 0;
						flag2 = 0;
					    indice = 0;
					    //Verific daca exista cardul intrudos flag1 == 1 si parola
					    //este cea corecta flag2 == 1
						for(j = 1 ; j <= nr_conturi ; j++)
						{
							if(strncmp(cont[j].numar_card,numar_card,6) == 0 
								&& strlen(numar_card) == 6)
							{
								flag1 = 1;
								indice = j;
								if(strncmp(cont[j].pin,pin,4) == 0 && strlen(pin) == 4)
									flag2 = 1;
							}
						}
						
						memset(buffer, 0, BUFLEN);
						if(flag1 == 0) //Daca numarul cardului este invalid
						{
							strcpy(buffer, "IBANK> -4 : Numar card inexistent");
							//resetam si contorul pt nr de pinuri consecutive gresite
							//pentru acelasi card ( asa am intesls si gandit eu ca ar
							//trebuit sa se produca blocarea)
							cl_fails[i] = 0;
							cl_fail_cont[i] = 0;
						}
						else if (cont[indice].clientSocket > 0) //Daca cineva este deja conectat
						{										//pe acel cont
							strcpy(buffer,"IBANK> -2 : Sesiune deja deschisa");
						}
						else if (cont[indice].cont_status == 2) //Daca cardul este blocat 
						{
							strcpy(buffer, "IBANK> -5: Card blocat"); 
						}
						else if(flag2 == 0) //Daca pinul a fost introdus gresit 
						{
							strcpy(buffer, "IBANK> -3 : Pin gresit");
							
							//Verificam daca cardul curent cu care a incercat
							//sa se logheze clientul trebuie blocat 
							
							
							//Daca cardul pt care s-a introdus parola gresita 
							//a fost introdus ,si la utlima logare , atunci contorul
							//continua sa numere, altfel este initializat la 0
							if(cl_fail_cont[i] != indice)
							{
								cl_fails[i] = 0;
								cl_fail_cont[i] = indice;
							}
							
							
							cl_fails[i]++;
							if(cl_fails[i] == 3){
								cl_cont[i] = 0; //nu este logat pe niciun cont 
								cont[indice].cont_status = 2; //blocat 
								strcpy(buffer, "IBANK> -2 : Card blocat"); 	
							}
						}
						else
						{
							//In acest caz inregistrarea a avut succes, se reseteaza
							//toti contorii 
							cl_fails[i] = 0;
							cl_fail_cont[i] = 0;
							cl_cont[i] = indice; //Indicele contului pe care 
												//clientul de pe socketul i s-a logat
							
							cont[indice].clientSocket = i;
							strcpy(buffer, "IBANK> Welcome ");
							strcat(buffer, cont[indice].nume);
							strcat(buffer, " ");
							strcat(buffer, cont[indice].prenume);
						}
						send(i,buffer,BUFLEN,0);	
						if (n < 0){ 
        					printf("-10 : Eroare la apel send() (TCP)\n");
        				}
										
					} else if (strncmp(buffer,"logout",5) == 0 && transfer[i] == 0)
					{
						//Nu verific daca clientul era deja logat , deoarece 
						//clientul , de pe care s-a trimmis acest mesaj , nu permite
						//trimiterea sa , daca aceasta nu s-a logat inainte
						cont[cl_cont[i]].clientSocket = 0;
						cl_cont[i]= 0;
						memset(buffer,0,BUFLEN);
						strcpy(buffer,"IBANK> Clientul a fost deconectat");
						
						send(i,buffer,BUFLEN,0);
						if (n < 0){ 
        					printf("-10 : Eroare la apel send() (TCP)\n");
        				}
						
					}else if (strncmp(buffer,"listsold",8) == 0 && transfer[i] == 0)
					{
						//Se verifica lungimea comenzii deoarece ea vine fara parametrii
						if(strlen(buffer) > 8)
						{
							strcpy(buffer,"IBANK> -10 : Eroare la listsold, comanda nu a fost ");
							strcat(buffer,"introdus corespunzator");
						
						}
						else
						{
							memset(buffer,0,BUFLEN);
							memset(sold_array,0,15);
							sprintf(sold_array,"%.2lf",cont[cl_cont[i]].sold);
							strcpy(buffer,"IBANK> ");
							strcat(buffer,sold_array);
						}
						n = send(i, buffer, BUFLEN, 0);
						if (n < 0){ 
        					printf("-10 : Eroare la apel send() (TCP)\n");
        				}
					
					}else if(strncmp(buffer,"transfer",8) == 0 || transfer[i] != 0)
					{
						
						//Daca transfer[i] este nenul , atunci acest mesaj , este un
						//raspuns la o cerere din confirmare (trimis de catre server)
						if(transfer[i] != 0)
						{
							//Daca prima litera din mesaj este 'y' atunci banii 
							//se vor transfera , altfel nu 
							if(buffer[0] == 'y')
							{
								//transfer[i] contine indicele contului in care
								//urmeaza sa fie transferati banii
								memset(buffer,0,BUFLEN);
								strcpy(buffer, "IBANK> Transfer realizat cu succes");
								cont[cl_cont[i]].sold -= bani[i];
								cont[transfer[i]].sold += bani[i];
							}
							else
							{
								memset(buffer, 0, BUFLEN);
								strcpy(buffer, "IBANK> -9 : Operatie anulata");
								
							}
							
							transfer[i] = 0; //marcam faptul ca am termiant
								//de realizat operatia de transfer
							bani[i] = 0;
							n = send(i, buffer, BUFLEN, 0);
							if (n < 0){ 
        						printf("-10 : Eroare la apel send() (TCP)\n");
        					}	
						}
						else
						{
							n = 0;
							memset(numar_card,0,6);
							memset(sold_array,0,15);
							tok = strtok(buffer," "); 
							while(tok != NULL)
							{
								if(n == 1)
								{
									strncpy(numar_card,tok,6);							
									if(strlen(tok) > 6)
										numar_card[6] = 23;//Daca lungimea numarului de card depaseste 6
								}
								if(n == 2)
								{
									strcpy(sold_array,tok);
								}
								tok = strtok(NULL," ");
								n++;
							}
						
							sold_val = atof(sold_array);
							flag1 = 0; //pt a verifica daca cardul exista
							flag2 = 0; //pt a verifica daca suma introdusa este transferabila
							
							indice = 0;
							for(j = 1 ; j <= nr_conturi ; j++)
							{
								if(strncmp(cont[j].numar_card,numar_card,6) == 0
									 && strlen(numar_card) == 6) 
								{
									flag1 = 1;
									indice = j;
									
								}
							}
							if(sold_val <= cont[cl_cont[i]].sold)
									flag2 = 1;
						
							memset(buffer,0,BUFLEN);
							if(flag1 == 0)
							{
								strcpy(buffer,"IBANK> -4 : Numar card inexistent");
							} else if (flag2 == 0)
							{
								strcpy(buffer,"IBANK> -8 : Fonduri insuficiente");
							}
							else
							{
								bani[i] = sold_val;
								transfer[i] = indice; //Catre ce cont se face
													//trnsferul
								strcpy(buffer,"IBANK> Transfer ");
								//Conversia cu 2 zecimale
								memset(sold_array,0,15);
								sprintf(sold_array,"%.2lf",sold_val);
								
								//Compunem mesajul 
								strcat(buffer, sold_array);
								strcat(buffer, " catre ");
								strcat(buffer,cont[indice].nume);
								strcat(buffer," ");
								strcat(buffer,cont[indice].prenume);
								strcat(buffer,"? [y/n]");
							}
							
							n = send(i,buffer,BUFLEN,0);
							if (n < 0){ 
        						printf("-10 : Eroare la apel send() (TCP)\n");
        					}
						}
					}else if (strncmp(buffer,"quit",4) == 0 && transfer[i] == 0)
					{
						//Daca un client a dat quit , atunci contul in cazul in care era
						//logat pe un cont , acum acesta este disponibil 
						cont[cl_cont[i]].clientSocket = 0;
						cl_cont[i] = 0;
						cl_fails[i] = 0;
						cl_fail_cont[i] = 0;
						//inchid socketul , si il scot din multimea de read_fds
						close(i); 
						FD_CLR(i, &read_fds);
						
					}
					
				} 
			}
		}
     }

     return 0; 
}


