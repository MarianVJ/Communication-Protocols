#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001
#define TIME_MAX  5000
	
	
	//Aceasta functie primeste ca parametru ultimul sequence number 
	//pentru care s-a trimis ACK , si asteapta urmatorul pachet
	//de secventa seq + 1 
	//Functia intoarce mesajul receptionat , in caz ca
	//transmisia s-a produs cu succes, sau NULL
	//in cazul in care s-a depasit de 3 ori consecutiv
	//limita de TIMEOUT 
	msg* receiveMessage(char *	seq1){
 		int i;
 		unsigned short crc, crc_payload;
 		msg t ;
 		msg *y;
 		
 		//Creem acest mesaj cu ACK pentru un mesaj primit anterior
 		//pentru a-l trimite daca prima incercare de a receptiona
 		//in timp corespunzator mesajul esueaza
 		char seeq = *seq1;
		createPacket(&t,&seeq,ACK,0,NULL);
		
 		for(i = 0 ; i < 3 ; i++){	
			//timeout-secunde 
			y = receive_message_timeout(TIME*1000);
			//In cazul in care se produce timeout , retrimitem
			//ultimul pachet trimis (Care poate fi ori ACK la pachetul
			//de secventa seq1 sau NACK la pachetul curent 
			//(ultimul mesaj trimis emitatorului)
			
			if(y == NULL){
				send_message(&t);
				printf("[RCV] Nu s-a primit pachetul la timp \n");
			}
			else{
			   
				//Calculam crc-ul pentru bitii corspunztori
				crc = crc16_ccitt(y->payload, y->len-3);	
				//Luam crc-ul din mesaj
				memcpy(&crc_payload, y->payload + y->len - 3, 2);
				//Odata ce a fost receptionat
				//Un mesaj trebuie sa se piarda de 3 ori pe canal
				//pentru a intrerupe transmisia
				i = -1;
				if(crc == crc_payload && y->payload[2] == ((*seq1 +1)%64)){
				
					printf("[RCV] Mesajul primit cu nr scv %d de tip %c\n",
							y->payload[2],y->payload[3]);
				    char seq = y->payload[2];
				    //Actualizam numarul de secventa global
				    *seq1 = seq;
					createPacket(&t,&seq,ACK,0,NULL);	
					send_message(&t);
					break;

				}
				else{
					printf("[RCV] Mesajul Corupt cu nr scv %d de tip %c\n",
							y->payload[2],y->payload[3]);
					char seq = *seq1;
					//Zona de date este vida pt un mesaj NACK  cu nr de 
					//Secventa de la mesajul corupt
					createPacket(&t,&seq,NACK,0,NULL);
					send_message(&t);
					
				}
			}
			if(i == 2){
				printf("[RECEIVER] ->Am asteptat cele 15 secunde si nu am primit nimic \n");
				return 0;	
			}
			
		}
		return y;
 	}
 	
 	
 	//Aceasta functie are un comportament similar cu cel al functiei de mai sus
 	//doar ca este conceputa pentru a primi mesajul de tip init , in care se
 	//verifica daca parametrii de transmisie sunt buni astfel incat sa nu existe
 	//diferente care ar ingreuna transmisia (Mai multe detalii in README
 	//pentru motivul pentru care am creeat si aceasta functie)
 	msg* receiveInitMessage(char *	seq1){
 		int i;
 		unsigned short crc, crc_payload;
 		msg t ;
 		msg *y;
		
 		for(i = 0 ; i < 3 ; i++){	
	
			//timeout-secunde 
			y = receive_message_timeout(TIME*1000);
			//In cazul mesajului de tip Iinit , daca au trecut cele "timeout"
			//secunde si nu am primit mesajul , nu retransmitem nimic
			if(y == NULL){
				printf("[RCV] Nu s-a primit pachetul de INIT la timp \n");
			}
			else{
			   
				crc = crc16_ccitt(y->payload, y->len-3);	
				memcpy(&crc_payload, y->payload + y->len - 3, 2);
				i = -1;
				if(crc == crc_payload && y->payload[2] == ((*seq1 +1)%64)){
				
				    char seq = y->payload[2];
				    *seq1 = seq;
				    
				    //Verificam daca parametrii de transmisie ai transmitatorului
				    //sunt aceeasi pe care ii are si receptorul pentru a se 
				    //asigura o comunicare eficienta 
				    if((char)MAXL == y->payload[4] && y->payload[5] == TIME &&
				      y->payload[8] == MARK	){
				      printf("[RCV] Am primit mesajul de INIT cu parametrii corespunzatori\n");
						createPacket(&t,&seq,ACK,0,NULL);	
						send_message(&t);
						break;
					}
					else
					{
						 printf("[RCV] Am primit mesajul de INIT cu parametrii necorespunzatori\n");
						createPacket(&t,&seq,NACK,0,NULL);	
						send_message(&t);
						*seq1 -=1;
					}

				}
				else{
					printf("[RCV] Mesajul De Init a fost corupt\n");
					char seq = *seq1;
					createPacket(&t,&seq,NACK,0,NULL);
					send_message(&t);
				}
			}
			if(i == 2){
				printf("[RCV] ->Am asteptat cele 15 secunde si nu am primit nimic \n");
				return 0;	
			}
		}
		return y;
 	}

int main(int argc, char** argv) {

	
	msg *y = NULL;
    char seq=-1;
    init(HOST, PORT);
    
    //Asteptam de maxim 3 ori timp de TIME_OUT secunde 
    //primirea unui mesaj de tip SEND_INIT de la emitator
    //in caz ca nu am primit incheiem tranmisia
	y  = receiveInitMessage(&seq);
	if(y == NULL){
		return 0;
	}
   
    
    FILE *f = NULL;
    while(1){
		
	 	y  = receiveMessage(&seq);
		//Daca s-a incercat receptionarea pachetului 3 de ori fara
		//succes incheiem transmisia
		if(y == NULL){
			//inainte de a opri activitatea receptorului inchdem
			///fisierul de scris in caz ca nu a fost inchis
			if(f != NULL)
				fclose(f);
			return 0;
		}
		
		//Daca ultimul pachet este de tip FHEADER 
		//Atunci creem fisierul cu numele corespunzator 
		if(y->payload[3] == FHEADER){
			char file_name[250] = {0};
			strcpy(file_name,"recv_");
			memcpy(file_name + 5, y->payload + 4, y->len - 7);
			f = fopen(file_name,"wb");
		}
		
		//Daca pachetul receptionat este de ti[ DATA atunci
		//scriem datele primite in fiserul corespunzator
		if(y->payload[3] == DATA){
			fwrite(y->payload + 4, 1, y->len - 4 - 3,f);
		}
		
		//Inchidem fisierul dupa ce a fost transmis tot continutul acestuia
		if(y->payload[3] == MEOF){
			fclose(f);
			f = NULL;
		}

		//Dacaa am primit pachet de tip MEOT 
		//transmisia s-a incheiat si parasim aceasta bucla
		if(y->payload[3] == MEOT){
			break;
		}
	}
	return 0;
}


