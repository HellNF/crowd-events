# CrowdEvents

Plugin C++ per Unreal Engine 5.4 che aggiunge a Dynamic Crowd Routing gli eventi semantici
come campi attrattivi e repulsivi. Espone le formule ai Blueprint come nodi della categoria
«Crowd Events», mette gli eventi nel livello come attori e aggiunge all'editor il pannello
«Crowd Events». Le distanze sono in metri.

Stato: versione 0.3.0, **scritta ma non ancora compilata**. Aggiunge alla 0.2.0 (compilata e
provata con Unreal 5.4.4 il 9 ottobre 2026) le formule della reward, il componente di reazione
del pedone e il gestore degli stati. Gli eventi influenzano i pedoni solo dopo aver aggiunto il
componente a `BP_Pawn` e i nodi di chiamata nei Blueprint di DCR (sezione «Collegamenti»).

## Moduli

| Modulo | Tipo | Contenuto |
|---|---|---|
| `CrowdEvents` | Runtime | formule, attore `ACrowdEvent`, preset, intervalli di addestramento, componente `UCrowdReactionComponent`, gestore `UCrowdEventsSubsystem` |
| `CrowdEventsEditor` | Editor | pannello «Crowd Events» e piazzamento con un clic |

## Pannello

Si apre da **Window → Crowd Events**. I testi sono in inglese.

| Sezione | Che cosa fa |
|---|---|
| Simulation | avvia e ferma la simulazione in Simulate (Play non funziona nel progetto di DCR) |
| New event | si sceglie un preset, si preme *Place in level* e si clicca sul pavimento; Esc annulla |
| Events in the level | elenco degli eventi con classe e tempo rimasto; *End event*, *Remove* |
| Selected event | classe e parametri dell'evento scelto, con il raggio dell'anello ricalcolato |
| View | cerchi di debug; punto colorato sui pedoni che reagiscono e conteggio per stato; modalità avanzata per uscire dagli intervalli di addestramento |

Le modifiche fatte a simulazione in corso si perdono quando la simulazione si ferma. A
simulazione ferma gli eventi entrano nel livello e si salvano con esso.

I preset di partenza (Street performer, Brawl, Fallen person, Fire, Armed person) sono
definiti in `CrowdEventTypes.cpp`. Altri si aggiungono dal Content Browser come Data Asset di
classe `CrowdEventPreset`.

## Contenuto

| Nodo | Che cosa fa |
|---|---|
| `Get Crowd Events Version` | restituisce `0.3.0`; serve a verificare che il plugin sia caricato |
| `Perceived Field` | campo percepito a una distanza dall'evento |
| `Ring Radius` | distanza a cui il campo ha il minimo; se non esiste, la distanza di contatto |
| `Bubble Radius` | anello del pedone tipico più lo spessore del cerchio di chi guarda |
| `Reference Radius` | distanza di riferimento per stato e ruolo; -1 negli stati 0 e 3 |
| `Avoid Reward`, `Safe Wall Reward` | stato 1: bolla, e muro alla distanza di sicurezza dopo la fuga |
| `Linger Reward`, `Linger Speed Target`, `Linger Speed Reward` | stato 2: anello o contatto, velocità bersaglio e termine di velocità |
| `Flee Reward` | stato 3: allontanamento nel passo |
| `Notice Probability` | probabilità di accorgersi dell'evento in un passo |
| `Reward Multipliers` | moltiplicatori dei cinque termini della reward di DCR per stato |

## Stati dei pedoni

`UCrowdEventsSubsystem` esiste da solo in ogni mondo di gioco. A ogni tick, per ogni pedone
con il componente `Crowd Reaction`, decide se si accorge di un evento, gli assegna stato e
ruolo e ne governa le transizioni. La policy non decide nulla di tutto questo.

| Regole | Quando | Che cosa fanno |
|---|---|---|
| dimostrazione (predefinite) | inferenza | il pedone si accorge con `Notice Probability`; poi attrattivo: sosta con probabilità `CuriosityProbability`, altrimenti schiva; malore: soccorre se i soccorritori sono meno di 2 ed è propenso, altrimenti come sopra; pericolo: fuga |
| addestramento | training | entrando nel raggio di percezione, il 20% resta nello stato 0; gli altri ricevono a caso uno degli stati ammessi dalla classe; soste di 5-20 s |

Le regole si scelgono con il nodo `Set Rule Set` del gestore o con la variabile di console
`CrowdEvents.Training` (0 o 1), che ha la precedenza. `Set Random Seed` fissa le estrazioni
del gestore.

Transizioni: la sosta dura dal momento in cui il pedone è fermo sull'anello (o a contatto);
finita, lo spettatore passa allo stato 1 e il soccorritore spegne il malore. Chi fugge passa
allo stato 1 «dopo la fuga» oltre 15 m e torna a fuggire sotto 10 m. A evento finito tutti
tornano allo stato 0.

Scelte fatte scrivendo il codice, non presenti nella specifica:

- nello stato 0 le 12 osservazioni sugli eventi valgono tutte zero, bit dello stato compreso
  (`bNeutralStateAsZeros`): sono gli ingressi con cui è stata addestrata la baseline;
- il premio di uscita dalla fuga si dà una sola volta per evento e per pedone, altrimenti
  rientrare e riuscire dalla distanza di sicurezza converrebbe;
- chi deve sostare e non raggiunge il bersaglio entro 30 s passa allo stato 1;
- il contagio conta i pedoni in fuga entro 20 m nel semipiano davanti, senza usare i raggi.

Non ancora fatto: la scaletta che fa comparire gli eventi in addestramento, il pericolo
mobile, le classi di pedone (adulto, giovane), le figure al posto del disco.

## Collegamenti nei Blueprint di DCR

Da fare a mano, dopo aver aggiunto il componente `Crowd Reaction` a `BP_Pawn`.

| Dove | Nodo del componente | Uso |
|---|---|---|
| reset del pedone (`ResetPosition`) | `Reset Reaction` | dopo l'estrazione della velocità desiderata, passandola in m/s |
| pedone fantasma del training | proprietà `Perceives Events` | falsa finché il pedone è fantasma |
| `BP_AgentInteractor`, raccolta delle osservazioni | `Get Event Observations` | i 12 valori sugli eventi, nell'ordine dello schema |
| `BP_AgentTrainer.GatherAgentReward` | `Get Reward Multipliers`, `Get Event Reward` | reward = somma dei cinque termini di DCR, ciascuno per il suo moltiplicatore, più `Get Event Reward` |
| `BP_Pawn.ComputeDesiredVelocity` | `Get Speed Target MS`, `Get Max Walk Speed Cm`, `Counts In Neighbour Average` | bersaglio di velocità e velocità massima per stato; nella media dei vicini solo chi ha il terzo nodo vero |
| `BP_Pawn.CastRays` | cast dell'attore colpito a `Crowd Event` | indicatore «colpisce un evento» del raggio; l'evento non accende l'indicatore di ostacolo e non entra nella `HitMap` |
| `BP_Pawn.SaveData` | `Get State Index`, `Get Event Id`, `Get Distance To Event M` | registro degli stati |

`Get Event Reward` va chiamato una volta per decisione: il termine di fuga confronta la
distanza con quella della chiamata precedente.

## Riferimento Python

`reference/crowd_events_reference.py` contiene le stesse formule in Python.
`reference/generate_cases.py` scrive `reference/cases.json` (1228 casi con ingressi e valori
attesi). `reference/check_in_unreal.py` va eseguito dentro l'editor, con il plugin compilato:
chiama i nodi C++ e li confronta con i casi, con tolleranza 1e-6.

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

Eseguite il 9 ottobre 2026 nel progetto di DCR, su `Classic/Map_Level_Lane`.

| Prova | Esito |
|---|---|
| 1. Compilare con `Build.bat` come per la 0.1.0 (ora i moduli sono due) | passata: nessun errore né avviso |
| 2. Aprire il progetto: nel log `CrowdEvents caricato`, in **Window** la voce **Crowd Events** | passata |
| 3. A simulazione ferma: *Street performer*, *Place in level*, clic sul pavimento. Compaiono un disco a terra e due cerchi: l'anello a 1,5 m e il raggio di percezione a 10 m. Con *Brawl* l'anello è a circa 4 m | passata dopo una correzione: nelle viste ortografiche (Top) il clic non trovava il pavimento |
| 4. Clic su un muro: l'evento non viene piazzato e compare un avviso | passata per le facce verticali; la cima di un muro è orizzontale e viene accettata |
| 5. *Start* avvia Simulate. Piazzando un evento a simulazione in corso e cambiandone i parametri, l'anello disegnato segue il valore di *Danger* | passata |
| 6. *End event* toglie cerchi e disco; *Remove* toglie l'evento dall'elenco; *Stop* ferma la simulazione | passata |
| 7. Schermata del pannello: l'aspetto è stato scritto senza vederlo | da fare |

La modalità di piazzamento (`FEdMode` registrata con `FEditorModeRegistry`), `SSegmentedControl`
e l'avvio di Simulate con `RequestPlaySession` funzionano nella 5.4.4 senza modifiche.

Non ancora provati: i preset come Data Asset, la modalità avanzata e la casella dei cerchi di
debug.

## Prove della 0.3.0 sul remoto

Da eseguire nel progetto di DCR.

| Prova | Esito |
|---|---|
| 1. Compilare con `Build.bat`; il codice è stato scritto senza compilatore, errori piccoli sono possibili | da fare |
| 2. Eseguire `reference/check_in_unreal.py` nell'editor: nel log «1228 casi su 1228 uguali al riferimento Python» | da fare |
| 3. Verificare su quale canale di collisione viaggiano i raggi delle zone con `NewVisibilityTrace` (`Visibility2`): il marcatore dell'evento oggi blocca solo `Visibility` | da fare |
| 4. Aggiungere `Crowd Reaction` a `BP_Pawn`, senza altri collegamenti; in Simulate su `Classic/Map_Level_Lane` piazzare *Street performer*: sui pedoni entro 10 m compaiono punti gialli (schiva) o blu (sosta), e il conteggio nel pannello cambia | da fare |
| 5. Con *Fire*: punti rossi; oltre 15 m diventano arancioni; a evento finito spariscono | da fare |
| 6. Con *Fallen person*: al più due punti verdi insieme | da fare |
| 7. Con `CrowdEvents.Training 1` in console: il pannello mostra «Rules: training» e circa un pedone su cinque resta senza punto | da fare |

Nelle prove 4-7 i pedoni non cambiano comportamento: la policy non riceve ancora nulla. Si
controlla solo che gli stati siano assegnati come previsto. Senza la prova 4 dei
collegamenti (`Reset Reaction` al reset), in training un pedone riportato alla partenza
conserva gli eventi già incontrati.
