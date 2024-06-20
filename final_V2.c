#include "config_bit.h"  // Inclusion des bits de configuration
#define _XTAL_FREQ 32000000 // Fréquence de l'oscillateur
#define Vitesse_son 345 // Vitesse du son en m/s
//PPS p230
// Prototypes des fonctions
void E_S(void);
void config_SMT(void);
void config_timer0(void); // Configuration du Timer0 pour 10Hz
void confg_interruption(void); // Configuration et autorisation des interruptions
void init_serie(void);
void transmettre(unsigned int mesure, unsigned char tab[]);
void __interrupt() ISR(void); // Routine d'interruption

// Variables globales
short int ecrire = 0; // Indicateur d'écriture
unsigned int mesure_d = 0; // Mesure du capteur de droite
unsigned int mesure_c = 0; // Mesure du capteur central
unsigned int mesure_g = 0; // Mesure du capteur de gauche

void main(void) 
{
  E_S();
  config_timer0();
  init_serie();
  config_SMT();
  confg_interruption(); //configuration et autorisation des interruptions
  //centre,gauche,droite = 6 lettres
  //                        0         1         2  
  //                        01234567890123456789012 3 456789012345
  unsigned char tab_d[] = {"mesure droite = 000,0cm\r\n"};
  //                        0         1         2  
  //                        01234567890123456789012 3 456789012345
  unsigned char tab_c[] = {"mesure centre = 000,0cm\r\n"};
  //                        0         1         2  
  //                        01234567890123456789012 3 456789012345
  unsigned char tab_g[] = {"mesure gauche = 000,0cm\r\n"};
  while(1==1)
  {
    if(ecrire)
    {
      ecrire=0;
      transmettre(mesure_g,tab_g);//a mettre en commentaire si on veut mesurer un seul capteur (droite)
      transmettre(mesure_c,tab_c);//a mettre en commentaire si on veut mesurer un seul capteur (droite)
      transmettre(mesure_d,tab_d);
    }
  }

}
void __interrupt() ISR ( void )
{
    static short int selec_capteur = 1;
    if ( TMR0IF ) // SI overflow du timer 0 (fin de comptage des 100ms))
    {
        // Réinitialiser le Timer0 pour la prochaine période de comptage
        TMR0H = 0x3C; // Partie haute du registre TMR0 (0x3C)
        TMR0L = 0xB0; // Partie basse du registre TMR0 (0xB0)
        TMR0IF = 0;   // RAZ du flag TMR0IF

        // Réinitialiser le Timer SMT1
        SMT1TMR = 0; // RAZ du timer SMT1
        SMT1CON1=SMT1CON1|0x80; //ob1000 0000 Déclencher le SMT1 (GO)

        // Sélectionner le capteur et générer une impulsion
        switch (selec_capteur)
        {
            case 1:
                SMT1SIGPPS=0x17; //selection de la broche RC7 0bxxx 10 111 en entree du SMT1 (trig capteur droite) p239
                //generation d une impulsion sur la broche echo du capteur droite
                RC5=1;
                __delay_us(15);
                RC5=0;
                break;
            case 2:
                SMT1SIGPPS=0x16; //selection de la broche RC6 0b0xxx 10 110 en entree du SMT1 (trig capteur centre) p239
                //generation d une impulsion sur la broche echo du capteur centre
                RC1=1;
                __delay_us(15);
                RC1=0;
                break;
            case 3:
                SMT1SIGPPS=0x14; //selection de la broche RC4 0b0xxx 10 100 en entree du SMT1 (trig capteur gauche) p239
                //generation d une impulsion sur la broche echo du capteur gauche
                RC0=1;
                __delay_us(15);
                RC0=0;
                break;
            default :
                selec_capteur=1;
                break;
                    
        }
    }
    if ( SMT1PWAIF )// Vérifier si le drapeau d'interruption de largeur d'impulsion SMT1 est levé
    {
      SMT1PWAIF=0;  // RAZ du flag d'interruption de largeur d'impulsion SMT1
      unsigned long int mesure=0;
      mesure=SMT1CPW; // Lire la mesure du SMT1
      // Calculer la distance (mm) en utilisant la mesure
      mesure=(mesure*(1/32000000.0)*Vitesse_son/2.0*1000.0);
      // Stocker la mesure en fonction du capteur sélectionné
      switch (selec_capteur)
        {
            case 1:
                mesure_d=mesure; // Stocker la mesure pour le capteur droite
                selec_capteur++; //a mettre en commentaire si on veut mesurer un seul capteur (droite)
                //ecrire=1; // a rajouter si on veut mesurer un seul capteur (droite)
                break;
            case 2:
                mesure_c=mesure; // Stocker la mesure pour le capteur centre
                selec_capteur++;
                break;
            case 3:
                mesure_g=mesure; // Stocker la mesure pour le capteur gauche
                selec_capteur=1;
                ecrire=1; // Déclencher l'envoi des mesures
                break;
            default :
                selec_capteur=1;
                break;
        }
    }
}        
void E_S (void)
{
 TRISC= TRISC & 0xDC;  //0b1101 1100  
 //RC5 en sortie Sd (trig droite)
 //RC0 en sortie Sg (trig gauche)
 //RC1 en sortie Sc (trig centre)
 TRISC= TRISC | 0xD0;  //0b1101 0000
 //RC4 en entee (echo gauche)
 //RC6 en entee (echo centre)
 //RC7 en entee (echo droite)
 TRISB= TRISB & 0xEF;  //0b1110 1111
 //RB4 en sortie (Tx)
 TRISB= TRISB | 0x40;  //0b0100 0000
 //RB6 entree (Rx)
 ANSELB=0x00; //port B en TOR
 ANSELC=0x00; //port C en TOR
}

void config_SMT(void)
{
 // Configuration du registre SMT1CON0
 SMT1CON0 = 0x80; // 0b1000 0000
 /* 
 Bits de SMT1CON0 :
 1 : SMT enable (activation du SMT)
 x : Non utilisé (toujours à 0)
 0 : Le compteur se réinitialise à l'interruption SMT1PR
 0x : La fenêtre se déclenche sur le front montant
 0 : SMT actif-haut / front montant activé
 0 : SMT incrémente sur le front montant du signal d'horloge sélectionné
 00 : Diviseur de fréquence (prescaler) 1:1
 */

 // Configuration du registre SMT1CON1
 SMT1CON1 = 0x01; // 0b0000 0001
 /* 
 Bits de SMT1CON1 :
 0 : GO (Incrémentation et acquisition de données désactivées)
 0 : single acquisition
 x : Non utilisé (toujours à 0)
 x : Non utilisé (toujours à 0)
 0001 : gated timer mode
 */

 // Configuration du registre SMT1CLK
 SMT1CLK = 0x01; // 0b0000 0001
 /* 
 Bits de SMT1CLK :
 xxxxx : Non utilisé (toujours à 0)
 001 : Source d'horloge Fosc
 */

 // Configuration du registre SMT1SIG
 SMT1SIG = 0x00; // Pin sélectionnée par SMT1SIGPPS p328
 /* 
 Bits de SMT1SIG :
 Tout à 0 : Utilise la broche sélectionnée par SMT1SIGPPS
 */
}
void config_timer0(void)// 10HZ
{
 // Configuration du registre T0CON0
 // 0x90 = 0b1001 0000
 // 1 (bit 7) : Timer0 activé
 // 0 (bit 6) : Non utilisé (toujours à 0)
 // 1 (bit 5) : Mode 16 bits
 // 0000 (bits 4-0) : Post-diviseur à 1:1
 T0CON0 = 0x90;

 // Configuration du registre T0CON1
 // 0x54 = 0b0101 0100
 // 010 (bits 7-5) : Source d'horloge Fosc/4
 // 1 (bit 4) : Synchronisation désactivée
 // 0100 (bits 3-0) : Pré-diviseur à 1:16
 T0CON1 = 0x54;

 // Initialisation du registre TMR0 pour générer l'interruption à 10Hz
 // Calcul du préchargement : 65536 - (50000) = 15536
 // En hexadécimal : 15536 = 0x3CB0
 TMR0H = 0x3C; // Partie haute du registre TMR0 (0x3C)
 TMR0L = 0xB0; // Partie basse du registre TMR0 (0xB0)
    
 // Commentaire sur le calcul de la fréquence
 // Fréquence du Timer = Fosc / (Pré-diviseur * Post-diviseur * (65536 - TMR0))
 // 32 MHz / (4*16 * 50000) = 10 Hz
}
void init_serie(void)
{
    SP1BRG=16; //  p413
    /*CSRC : '0' sans importance car on est en mode asynchrone
    TX9 : '0' transmission de 8 bits
    TXEN : '1' Autorisation de la transmission
    SYNC : '0' Mode de fonctionnement asynchrone5 (8 bits )
    SENDB : '0'
    BRGH : '1' (8 bits )
    TRMT : '0' c'est un bit en lecture seule, donc sans importance !
    TX9D : '0' le neuvi me bit sert de bit de STOP*/
    TX1STA=0x24;//0b 0010 0100
    /*SPEN : '1' validation de l'affectation au port s rie de RC6 et RC7
    RX9 : '0' r ception de 8 bits
    SREN : '0' Sans importance pour le mode asynchrone
    CREN : '1' Validation de la r ception
    ADDEN : '0' invalidation de la d tection d'adresse 
    FERR : '0' c'est un bit en lecture seule, donc sans importance !?
    OERR : '0' c'est un bit en lecture seule, donc sans importance ! 
    RX9D : '0' c'est un bit en lecture seule, donc sans importance !*/
    RC1STA=0x90; //1000 0000
    BAUD1CON = 0x00 ;//0b xxxx 0(8 BRG16=0)xxx
    RB6PPS=0x0E;//xxx 01 110 rb6 on cable notre RX qui vient de CDCTX
    RB4PPS=0x0F;//xx 00 1111 rb4 on cable notre TX qui va sur CDCRX p231(pour RB4PPS ou CK1PPS) et page 233 pour la valeur 00 1111
}

void confg_interruption(void)
{
  TMR0IE=1; // autorisation des interruptions liees au timer0
  SMT1PWAIE=1;  //autorisation des interruptions SMT1 pour la mesure de largeur d'impulsion (Pulse Width Acquisition)
  PEIE=1; // autorisation des interruptions périphériques
  GIE=1; // autorisation des interruptions globales
}

void transmettre(unsigned int mesure,unsigned char tab[])
{
    short int i;
    // Extraction des milliers, centaines, dizaines et unités de la mesure (mesure en mm)
    unsigned int m = mesure/1000;
    unsigned int dm = mesure%1000/100;
    unsigned int cm = mesure%100/10;
    unsigned int mm = mesure%10;
    // Conversion des valeurs numériques en caractères ASCII et stockage dans le tableau
    tab[16] = (m + '0');
    tab[17] = (dm + '0');
    tab[18] = (cm + '0');
    tab[20] = (mm + '0');
 
    // Envoi de la chaine de caracteres
        for (i = 0; tab[i] != '\0'; i++) 
        {
            TX1REG = tab[i];  // Envoi du caractere contenu dans le tab a la position i
            while (TRMT==0); // Attente de la fin de la transmission
        }
}