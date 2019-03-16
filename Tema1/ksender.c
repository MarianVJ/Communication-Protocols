#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000
#define TIME_MAX 5000

	//Aceasta functie initializeaza frame-ul pentru un pachet de tipul
	//Send-Init
	void initializeFirstFrame(frame* ft, char * seq){
	
		init_frame inf;
		//Initializam zona de "DATE" dintr-un pachet de tip S
		initializeSendInitData(&inf);
		(*ft).soh = SOH;
		memset(ft, 0 , sizeof(frame));
		(*ft).soh = SOH;
		(*ft).seq = *seq;
		*seq = (*seq + 1 ) % 64;
		(*ft).type = SINIT;
		memcpy((*ft).data,&inf,sizeof(init_frame));
		(*ft).mark = 0x0D;
	}
	
			
	//Aceasta functie initializeaza un pachet de tip SEND-INIT
	//Ce va avea numarul de secv seq
	void initializeSendInitPacket(msg* t,  char * seq){
	
	 	//Acest frame este folosit doar pentru a creea un pachet
		//de tip SEND-INIT
		frame ft;
		//si este initializat cu ajutorul functiei corespunzatoare
		initializeFirstFrame(&ft,seq);
		//Copiem in payload-ul nostru in ordine headerul si zona de date din frame
		memset((*t).payload,0,sizeof((*t).payload));
		memcpy((*t).payload,&ft,4);
		memcpy((*t).payload + 4,&ft.data,11);
		memcpy((*t).payload + 4 + 11 + 2 ,&ft.mark, 1);
		//Setam campul de LEN Din pachet corespunzator
		(*t).payload[1] = 2  + 3 + sizeof(init_frame);
	   	//Calculam crc-ul pt primii 15 octeti ( header de 4 biti
	   	//+ zona de date
		unsigned short crc = crc16_ccitt((*t).payload, 15);
		unsigned short aux;
		//Scriem in pachet payload-ul
		memcpy((*t).payload + 15,&crc,2);
		memcpy(&aux,((*t).payload+15),2);
		(*t).len = (*t).payload[1] + 2;
	}
	
	
	//Aceasta functie intoarce 0 sau 1 
	//1 Daca a reusit sa transmita pachetul t la receptor
	// si 0 daca au fost 3 transmisii succesive fara niciun raspuns 
	//Caz in care transmitatorul isi va incheia activitatea
    //Pachetul t primit ca parametru este deja creat cu toate campurile 
    //complete
	char sendPacket(msg *t){
		int i;
	 	msg *y;
	 	printf("[SND] Trimis mesaj cu Nr Secv: %d TIP: %c\n"
	 		   ,(*t).payload[2],(*t).payload[3]);
			 	
		send_message(t);
		for(i = 0 ; i < 3 ; i++){
			//timeout-secunde 
			y = receive_message_timeout(TIME*1000);
			
			//In caz ca nu am primit niciun raspuns in limita
			//stabilita de timp retransmitem mesajul 
			if(y == NULL){
				printf("[SND] Depasire limita la msj cu Nr Secv: %d TIP: %c\n",
				(*t).payload[2],(*t).payload[3]);
				send_message(t);
			}
			else{
				//Daca am primit ACK , pachetul corespunzator a fost
				//transmis cu succes si parasi functia 
				if(y->payload[3] == ACK && y->payload[2] == t->payload[2]){
					printf("[SND] ACK la mesaj cu Nr Secv: %d TIP: %c\n"
	 		  				 ,(*y).payload[2],(*t).payload[3]);
					break;
				}
				else{
					//Daca am primit NACK retransmitem mesaju 
					//incepem din nou retransmisia pachetului curent
					i = -1;
					send_message(t);
					printf("[SND] NACK la mesaj cu Nr Secv: %d TIP: %c\n"
	 		  				 ,(*y).payload[2],(*t).payload[3]);
				}
			}
			
			//Daca au fost 3 incercari eusaate de a receptiona un raspuns
			//Din partea receptorului de ACK/NACK , returnam 0
			if(i == 2){
				printf("[SND] Pachetul a fost retransmis de 3 fara a primi raspusn\n");
				return 0;
			}	
	}
	//Daca receptia pachetului curent a avut succes intoarcem 1
	return 1;
	}
	

int main(int argc, char** argv) {

	char seq = 0;   
	//Campul maxim de date al unui pachet poate avea maxim MAXL bytes
	char message[MAXL];
	int i;
	char verify ;
    msg t;
    
    //Initializam pachetul de tip SEND-INIT
    initializeSendInitPacket(&t,&seq);
    
	init(HOST, PORT);
    printf("[SND] Am trimis mesaj de SEND-INIT(cu param emitatorului) \n");
 	//sendPacket va intoarce 0 daca nu am reusit sa transmitem pachetul
 	//si 1 in caz afirmativ  
    verify = sendPacket(&t);
	
	//In caz de esec se intrerupe transmisia iar emitatorul isi incheie executia
	if(verify == 0){
		return 0;
	}
    
    //Transmitem in ordinea data fisierele dorite
    for(i = 1 ; i < argc ; i++){		
		//Creeam pachet de FILE Header ce va avea in zona 
		//de date numele fisierului transmis la pasul curent
		createPacket(&t,&seq,FHEADER,strlen(argv[i]),argv[i]);
		//Trimitem pachetul
		verify = sendPacket(&t);
		
		//Verificam daca transmisia a eusat 
		if(verify == 0){
			return 0;
		}
    
	    //Deschidem fisierul i pentru citire 
    	FILE *f  = fopen(argv[i],"rb");
    	//Trimitem pachet cu pachet datele la receptor
    	//pana cand am terminat de transmis fisierul curent 
    	//Sau va exista o eroare pe canalul de transmisie ( de timeout)
		while(1){	
			//Setam toti bitii pe 0 
			memset(message, 0 ,sizeof(message));
		
			unsigned char octetiCititi = fread(&message,1,MAXL,f);
			
			//Daca am terminat de citit din fisierul curent 
			//paramsim bucla de transmisie a datelor
			if((unsigned char)octetiCititi <= 0){
				break;
			}
			//Altfel pachetul de date cu nr de secventa actual
			//tipul DATA  si care are in zona de date octetii 
			//cititi la pasul actual
			createPacket(&t,&seq,DATA,octetiCititi ,message);
			
			int verify = sendPacket(&t);
			if(verify == 0){
				return 0;
			}
		}
		
   
        //Creeam un cadru prin care sa anuntam faptul ca am terminat 
        //transmisia fisierului curent, iar functia ajutatoare primeste
        //ca ultim argument NULL semn ca pt acest tip de pachet
        //campul de date este VID
		createPacket(&t,&seq,MEOF,12,NULL);
		
	    verify = sendPacket(&t);
		if(verify == 0){
			return 0;
		}
	}
		
	//Creeam un cadru prin care sa anuntam faptul ca am terminat 
    //transmisia tuturor fisiereloor si in acest caz
    // functia ajutatoare primeste
    //ca ultim argument NULL semn ca pt acest tip de pachet
    //campul de date este VID
	createPacket(&t,&seq,MEOT,12,NULL);
	
    verify = sendPacket(&t);
	if(verify == 0){
		return 0;
	}
	return 0;

}
