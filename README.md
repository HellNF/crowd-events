# CrowdEvents

Plugin C++ per Unreal Engine 5.4 che aggiunge a Dynamic Crowd Routing gli eventi semantici
come campi attrattivi e repulsivi. Espone le formule ai Blueprint come nodi della categoria
«Crowd Events», mette gli eventi nel livello come attori e aggiunge all'editor il pannello
«Crowd Events». Le distanze sono in metri.

Stato: versione 0.2.0, prima versione di eventi e pannello. **Non ancora compilata**: è stata
scritta senza Unreal a disposizione e va compilata e provata sul remoto (lista in fondo).
Gli eventi per ora non influenzano i pedoni: mancano il gestore degli stati e i collegamenti
nei Blueprint di DCR.

## Moduli

| Modulo | Tipo | Contenuto |
|---|---|---|
| `CrowdEvents` | Runtime | formule, attore `ACrowdEvent`, preset, intervalli di addestramento |
| `CrowdEventsEditor` | Editor | pannello «Crowd Events» e piazzamento con un clic |

## Pannello

Si apre da **Window → Crowd Events**. I testi sono in inglese.

| Sezione | Che cosa fa |
|---|---|
| Simulation | avvia e ferma la simulazione in Simulate (Play non funziona nel progetto di DCR) |
| New event | si sceglie un preset, si preme *Place in level* e si clicca sul pavimento; Esc annulla |
| Events in the level | elenco degli eventi con classe e tempo rimasto; *End event*, *Remove* |
| Selected event | classe e parametri dell'evento scelto, con il raggio dell'anello ricalcolato |
| View | cerchi di debug; modalità avanzata per uscire dagli intervalli di addestramento |

Le modifiche fatte a simulazione in corso si perdono quando la simulazione si ferma. A
simulazione ferma gli eventi entrano nel livello e si salvano con esso.

I preset di partenza (Street performer, Brawl, Fallen person, Fire, Armed person) sono
definiti in `CrowdEventTypes.cpp`. Altri si aggiungono dal Content Browser come Data Asset di
classe `CrowdEventPreset`.

## Contenuto

| Nodo | Che cosa fa |
|---|---|
| `Get Crowd Events Version` | restituisce `0.2.0`; serve a verificare che il plugin sia caricato |
| `Perceived Field` | campo percepito a una distanza dall'evento |
| `Ring Radius` | distanza a cui il campo ha il minimo; se non esiste, la distanza di contatto |

## Valori di controllo

Con decadimenti di 1 m (repulsivo) e 4 m (attrattivo), `Alpha = Beta = 1`:

| Evento | Intensità attrattiva | Intensità repulsiva | `Ring Radius` atteso |
|---|---|---|---|
| artista di strada | 1 | 0,77 | 1,50 m |
| colluttazione | 1 | 5,0 | 3,99 m |
| pericolo | 0 | 1 | 0,8 m (contatto: nessun anello) |

## Installazione nel progetto

Clonare questo repository in `Plugins/CrowdEvents/` dentro la cartella del progetto:
il file `CrowdEvents.uplugin` deve trovarsi in `Plugins/CrowdEvents/CrowdEvents.uplugin`.

## Prove della 0.2.0 sul remoto

1. Compilare con `Build.bat` come per la 0.1.0. Ora i moduli sono due.
2. Aprire il progetto: nel log deve comparire `CrowdEvents caricato` e in **Window** la voce
   **Crowd Events**.
3. A simulazione ferma: scegliere *Street performer*, *Place in level*, cliccare sul pavimento
   di `Classic/Map_Level_Lane`. Devono comparire un disco a terra e due cerchi: l'anello a
   1,5 m e il raggio di percezione a 10 m. Con *Brawl* l'anello è a circa 4 m.
4. Cliccare su un muro: l'evento non viene piazzato e compare un avviso.
5. Premere *Start*: parte Simulate. Piazzare un evento a simulazione in corso e cambiarne i
   parametri: l'anello disegnato deve seguire il valore di *Danger*.
6. *End event* toglie cerchi e disco; *Remove* toglie l'evento dall'elenco; *Stop* ferma la
   simulazione.
7. Mandare una schermata del pannello: l'aspetto è stato scritto senza vederlo.

Punti in cui è più probabile un errore di compilazione o di comportamento: la modalità di
piazzamento (`FEdMode` registrata con `FEditorModeRegistry`), `SSegmentedControl` e l'avvio di
Simulate con `RequestPlaySession`.
