void prelucraremesajUDP(mesajUDP msj)
{
    char topic[51];
    memcpy(topic, msj.mesaj, 50);
    topic[50] = '\0';

    int abonati_deconetati = 0;

    int foundTopic = 0;
    for (int i = 0; i < nr_topicuri; ++i)
    {
        if (strcmp(topic, topics[i].title) == 0)
        {
            foundTopic = 1;
            for (int j = 0; j < topics[i].nr_abonati; ++j) // iau toti abonatii
            {
                for (int k = 0; k < nr_perechi; ++k) // gasesc socketii abonatilor
                {
                    if (strcmp(topics[i].iduri[j], perechi[k].id) == 0)
                    {
                        if (perechi[k].socket != -1) // daca este conectat abonatul fac un send
                        {
                            int ret = send(perechi[k].socket, &msj, sizeof(msj), 0);
                            DIE((ret == 0), "Nu am putut da send la mesajul UDP\n");
                        }
                        else
                        {
                            // am gasit abonat care nu e conectat
                            // verific daca are SF-ul setat
                            // Daca are sf-ul setat, incrementez numarul de abonati care trebuie sa primeasca mesajul acesta
                            // si pun id-ul mesajului in vectorul de mesaje care trebuie primite de client cand se va conecta

                            if (topics[i].sfs[j] == 1)
                            {
                                abonati_deconetati++;
                                perechi[k].id_mesaje_stored[perechi[k].nr_mesaje_stored] = msj.idmesaj;
                                perechi[k].nr_mesaje_stored++;
                            }
                        }
                    }
                }
            }
            break;
        }
    }
    DIE((foundTopic > 1), "Am gasit mai multe topicuri cu acelasi nume\n");
    if (foundTopic == 0)
    {
        // este un topic nou, care nu are abonati deci trebuie creat
        topics[nr_topicuri].sfs = (int *)malloc(CHUNK * sizeof(int));
        topics[nr_topicuri].iduri = (char **)malloc(CHUNK * sizeof(char *));
        for (int k = 0; k < CHUNK; ++k)
        {
            topics[nr_topicuri].iduri[k] = (char *)malloc(ID_LENGTH * sizeof(char));
        }

        strcpy(topics[nr_topicuri].title, topic);

        topics[nr_topicuri].nr_abonati = 0;
        nr_topicuri++;
    }
    if (abonati_deconetati > 0) // a existat cel putin un abonat cu SF-ul setat care nu a fost conectat la momentul primirii mesajului
    {
        // trebuie sa cream o noua celula de memorie
        storage[nr_celule].nr_subscriberi = abonati_deconetati;
        memcpy(&storage[nr_celule].mesajUDP, &msj, sizeof(msj));
        nr_celule++;

        if (nr_celule % CHUNK)
        {
            storage = (celula *)realloc(storage, (CHUNK + nr_celule) * sizeof(celula));
        }
    }
    /*(printf ("=======AFISARE CELULE MEMORIE=========\n");
    for (int i=0; i < nr_celule ;++i)
    {
        printf ( "%d) ID:%d SIZE:%d NR_SUBSRIBERI:%d\n", i, storage[i].mesajUDP.idmesaj, sizeof ( storage[i].mesajUDP ) ,storage[i].nr_subscriberi);
    }
    printf ("=======AFISARE CELULE MEMORIE=========\n");*/
    return;
}

void send_stored_messages ( int index) // primesc doar index-ul din pairs al clientului care ma intereseaza
{
    int socket = perechi[index].socket;
    DIE ((socket == -1), "Trimitere mesaje stored la un client deconectat\n");
    int k=0;
    while ( k  < perechi[index].nr_mesaje_stored)
    {
       
        int idMesaj = perechi[index].id_mesaje_stored[k];
        for (int j = 0 ; j < nr_celule; ++j)
        {
            if ( storage[j].mesajUDP.idmesaj == idMesaj) // am gasit mesajul pe care trebuie sa il trimitem
            {
                
                int ret = send ( socket, &storage[j].mesajUDP, sizeof ( storage[j].mesajUDP ), 0 );
                DIE ((ret == 0) , "Eroare la trimitere de STORED MESSAGES\n");

                storage[j].nr_subscriberi--;
                if (storage[j].nr_subscriberi == 0) // nimeni nu mai e interesat de acest mesaj, deci il stergem din storage
                {
                    if ( nr_celule == 1)
                    {
                        nr_celule = 0;
                    }
                    else
                    {
                        //memcpy ( &storage[j], &storage[nr_celule], sizeof ( celula ));
                        storage[j].nr_subscriberi = storage[nr_celule-1].nr_subscriberi;
                        memcpy (&storage[j].mesajUDP , &storage[nr_celule-1].mesajUDP, sizeof( mesajUDP));
                        nr_celule--;
                        j--;
                    }
                    

                }


            }
        }
        k++;
        
        
    }
    DIE( (k > perechi[index].nr_mesaje_stored), "Am trimis prea multe mesaje STORED la un client TCP proaspat conectat\n");
    perechi[index].nr_mesaje_stored = 0;

    return;

}


