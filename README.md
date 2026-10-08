# CrowdEvents

Plugin C++ per Unreal Engine 5.4 che aggiunge a Dynamic Crowd Routing gli eventi semantici
come campi attrattivi e repulsivi. Espone le formule ai Blueprint come nodi della categoria
«Crowd Events». Le distanze sono in metri.

Stato: scheletro per la prova di compilazione (versione 0.1.0).

## Contenuto

| Nodo | Che cosa fa |
|---|---|
| `Get Crowd Events Version` | restituisce `0.1.0`; serve a verificare che il plugin sia caricato |
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
