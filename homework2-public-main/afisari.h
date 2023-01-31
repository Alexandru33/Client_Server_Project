void printmesaj ( mesajUDP* mesajulUDP)
{
    /*struct in_addr ip_addr;
    ip_addr.s_addr = mesajulUDP->ip_udp;

    printf ("%s:%d - ", inet_ntoa(ip_addr), mesajulUDP->port_udp);
    */

    char topic[51];
    memcpy(topic, mesajulUDP->mesaj, 50);
    topic[50] = '\0';

    printf("%s - ", topic);

    uint8_t tipDate;
    memcpy ( &tipDate , mesajulUDP->mesaj + 50, 1);
    if ( tipDate == 0)
    {
        printf( "INT - ");
        uint8_t signbyte;
        uint32_t numarul;

        memcpy (&signbyte, mesajulUDP->mesaj + 51, 1);
        memcpy ( &numarul, mesajulUDP->mesaj + 52, 4);
        numarul = ntohl(numarul);

        if ( signbyte == 0)
        {
            printf ( "%d\n", numarul);
        }
        else
        {
            printf( "-%d\n", numarul);
        }
        

    }
    if ( tipDate == 1)
    {
        printf( "SHORT_REAL - ");

        uint16_t numarul;
        memcpy( &numarul, mesajulUDP->mesaj + 51, 2);
        numarul = ntohs (numarul);

        float real =  (  ((float) numarul) / 100);
        printf ("%.2f\n", real);
    }
    if ( tipDate == 2)
    {
        printf( "FLOAT - ");
        uint8_t semn;
        uint32_t numar;
        uint8_t putere;

        memcpy ( &semn , mesajulUDP->mesaj + 51, 1);
        memcpy ( &numar, mesajulUDP->mesaj + 52, 4);
        memcpy ( &putere, mesajulUDP->mesaj + 56, 1);

        uint8_t copieputere = putere;

        numar = ntohl ( numar);

        float real = (float) numar;
        while ( putere > 0)
        {
            real = real / 10;
            putere --;
        }
        if( semn == 0) printf ( "%.*f\n", copieputere,  real);
        else printf ( "-%.*f\n", copieputere,  real);
    }
    if ( tipDate == 3)
    {
        printf( "STRING - ");

        char continut[1501];
        memcpy ( continut, mesajulUDP->mesaj + 51, 1500);
        continut[1500] = '\0';
        printf( "%s\n", continut);
    }

    return;
    
}